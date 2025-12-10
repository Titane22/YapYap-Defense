// Fill out your copyright notice in the Description page of Project Settings.

#include "Gameplay/Data/Ability.h"

#include "Gameplay/Abilities/Projectile_Base.h"
#include "Gameplay/Abilities/AOE_Base.h"
#include "Gameplay/Components/AbilityComponent.h"
#include "Gameplay/Data/AbilityData.h"
#include "Gameplay/Data/AbilityEffect.h"
#include "Gameplay/Data/AbilityTypes.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AIController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Gameplay/Components/CharacterStatComponent.h"
#include "Kismet/GameplayStatics.h"

void UAbility::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UAbility, CurrentLevel);
	DOREPLIFETIME(UAbility, RemainingCooldown);
	DOREPLIFETIME(UAbility, CurrentCharges);
	DOREPLIFETIME(UAbility, bIsCasting);
}

void UAbility::Execute(const FAbilityTargetData& TargetData)
{
	// 1. Check if ability can be cast
	if (!CanCast())
		return;

	// 2. Validate targeting BEFORE executing
	if (!ValidateTargetData(TargetData))
	{
		// Check if it's ONLY a range issue and target is otherwise valid
		if (TargetData.TargetActor && TargetingStrategy &&
			(AbilityData->TargetingConfig.TargetingType == ETargetingType::Unit ||
			 AbilityData->TargetingConfig.TargetingType == ETargetingType::UnitAOE))
		{
			// Check if target is valid except for range
			float Distance = FVector::Dist(OwningActor->GetActorLocation(), TargetData.TargetActor->GetActorLocation());
			float Range = AbilityData->TargetingConfig.Range;

			if (Distance > Range)
			{
				// Target is valid but out of range - move closer
				UE_LOG(LogTemp, Warning, TEXT("Target out of range (%.1f > %.1f). Moving closer..."), Distance, Range);

				// Store pending execution
				bHasPendingExecution = true;
				PendingExecutionTargetData = TargetData;

				// Command character to move toward target
				if (APawn* PawnOwner = Cast<APawn>(OwningActor))
				{
					if (AController* Controller = PawnOwner->GetController())
					{
						if (AAIController* AIController = Cast<AAIController>(Controller))
						{
							UAIBlueprintHelperLibrary::SimpleMoveToActor(AIController, TargetData.TargetActor);
						}
						else if (APlayerController* PC = Cast<APlayerController>(Controller))
						{
							// For player, use navigation
							UAIBlueprintHelperLibrary::SimpleMoveToActor(PC, TargetData.TargetActor);
						}
					}
				}
				return;
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("Execute failed: Invalid target for ability %s"),
			*AbilityData->AbilityName.ToString());
		return;
	}

	// 3. Start Casting or Execute immediately
	if (AbilityData->CastTime > 0.f)
		StartCasting(TargetData);
	else
	{
		ConsumeResources();
		ExecuteDelivery(TargetData);
		StartCooldown();

		OnAbilityExecuted.Broadcast(this, TargetData);
	}
}

bool UAbility::ValidateTargetData(const FAbilityTargetData& TargetData) const
{
	if (!TargetingStrategy || !AbilityData)
		return false;

	const FTargetingConfig& Config = AbilityData->TargetingConfig;

	// Validate based on TargetingType
	switch (Config.TargetingType)
	{
	case ETargetingType::Unit:
	case ETargetingType::UnitAOE:
		{
			// Unit targeting requires a valid target actor
			if (!TargetData.TargetActor)
			{
				UE_LOG(LogTemp, Warning, TEXT("Unit targeting requires a target actor"));
				return false;
			}

			// Validate the target against filters
			if (!TargetingStrategy->ValidateTarget(TargetData.TargetActor))
			{
				UE_LOG(LogTemp, Warning, TEXT("Target %s failed validation (not matching filters)"),
					*TargetData.TargetActor->GetName());
				return false;
			}

			UE_LOG(LogTemp, Log, TEXT("Target %s passed validation"), *TargetData.TargetActor->GetName());
			return true;
		}

	case ETargetingType::Ground:
	case ETargetingType::GroundAOE:
	case ETargetingType::Direction:
	case ETargetingType::Cone:
		{
			// These types don't require target validation
			// Just check if we have a valid location
			if (TargetData.TargetLocation.IsNearlyZero() && TargetData.Direction.IsNearlyZero())
			{
				UE_LOG(LogTemp, Warning, TEXT("Ground/Direction targeting requires a valid location or direction"));
				return false;
			}
			return true;
		}

	case ETargetingType::Auto:
		{
			// Auto targeting will find targets automatically
			return true;
		}

	default:
		return false;
	}
}

void UAbility::Initialize(UAbilityData* InData, AActor* Owner, EAbilitySlot Slot)
{
	AbilityData = InData;
	OwningActor = Owner;
	AbilitySlot = Slot;
	OwningComponent = Owner->FindComponentByClass<UAbilityComponent>();

	CurrentCharges = AbilityData->MaxCharges;

	CreateTargetingStrategy();
	CreateRuntimeEffects();
}

void UAbility::LevelUp()
{
	CurrentLevel++;
}

void UAbility::CreateTargetingStrategy()
{
	if (!AbilityData)
		return;

	if (AbilityData->CustomTargetingClass)
	{
		TargetingStrategy = NewObject<UTargetingStrategy>(this, AbilityData->CustomTargetingClass);
	}
	else
	{
		TargetingStrategy = NewObject<UTargetingStrategy>(this);
	}

	// Config 초기화
	if (TargetingStrategy)
	{
		TargetingStrategy->Initialize(AbilityData->TargetingConfig, OwningActor);
	}
}

void UAbility::CreateRuntimeEffects()
{
	if (!AbilityData)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateRuntimeEffects: AbilityData is null"));
		return;
	}

	RuntimeEffects.Empty();

	UE_LOG(LogTemp, Log, TEXT("Creating Runtime Effects for %s - Effects count in data: %d"),
		*AbilityData->AbilityName.ToString(), AbilityData->Effects.Num());

	for (int32 i = 0; i < AbilityData->Effects.Num(); i++)
	{
		const FAbilityEffectData& EffectData = AbilityData->Effects[i];
		if (EffectData.EffectClass)
		{
			UAbilityEffect* NewEffect = NewObject<UAbilityEffect>(this, EffectData.EffectClass);
			NewEffect->InitializeFromData(EffectData, this);
			RuntimeEffects.Add(NewEffect);
			UE_LOG(LogTemp, Log, TEXT("  Created Effect %d: %s (BaseValue: %.1f)"),
				i, *EffectData.EffectClass->GetName(), EffectData.BaseValue);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("  Effect %d has no EffectClass set!"), i);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Total RuntimeEffects created: %d"), RuntimeEffects.Num());
}

void UAbility::StartCasting(const FAbilityTargetData& TargetData)
{
	bIsCasting = true;
	PendingTargetData = TargetData;

	OnAbilityCastStarted.Broadcast(this, AbilityData->CastTime);

	GetWorld()->GetTimerManager().SetTimer(
		CastTimerHandle,
		this,
		&UAbility::OnCastComplete,
		AbilityData->CastTime,
		false
	);

	if (AbilityData->CastAnimation && OwningActor)
	{
		// TODO: Play animation montage on character
	}
}

void UAbility::OnCastComplete()
{
	if (!bIsCasting)
		return;
	bIsCasting = false;
	ConsumeResources();
	ExecuteDelivery(PendingTargetData);
	StartCooldown();

	OnAbilityExecuted.Broadcast(this, PendingTargetData);
}

void UAbility::CancelCast()
{
	if (!bIsCasting)
		return;

	bIsCasting = false;

	GetWorld()->GetTimerManager().ClearTimer(CastTimerHandle);

	// 캔슬 시 리소스 환불 (선택사항)
	// RefundResources();

	OnAbilityCastCancelled.Broadcast(this);
}

void UAbility::ExecuteDelivery(const FAbilityTargetData& TargetData)
{
	if (!AbilityData)
		return;

	// Rotate character to face target before executing
	RotateOwnerToTarget(TargetData);

	switch (AbilityData->DeliveryConfig.DeliveryType)
	{
	case EAbilityDeliveryType::Instant:
		ExecuteInstant(TargetData);
		break;
	case EAbilityDeliveryType::Projectile:
		ExecuteProjectile(TargetData);
		break;
	case EAbilityDeliveryType::GroundAOE:
	case EAbilityDeliveryType::DelayedAOE:
		ExecuteAOE(TargetData);
		break;
	}
}

void UAbility::RotateOwnerToTarget(const FAbilityTargetData& TargetData)
{
	if (!OwningActor)
		return;

	FVector TargetLocation;

	// Determine target location based on TargetingType
	if (TargetData.TargetActor)
	{
		TargetLocation = TargetData.TargetActor->GetActorLocation();
	}
	else if (!TargetData.TargetLocation.IsNearlyZero())
	{
		TargetLocation = TargetData.TargetLocation;
	}
	else if (!TargetData.Direction.IsNearlyZero())
	{
		// Use direction to calculate a point ahead
		TargetLocation = OwningActor->GetActorLocation() + TargetData.Direction * 1000.f;
	}
	else
	{
		return; // No valid target to rotate toward
	}

	// Calculate rotation toward target (only Yaw, ignore pitch/roll)
	FVector Direction = TargetLocation - OwningActor->GetActorLocation();
	Direction.Z = 0.f; // Keep rotation horizontal

	if (!Direction.IsNearlyZero())
	{
		FRotator TargetRotation = Direction.Rotation();
		FRotator CurrentRotation = OwningActor->GetActorRotation();

		// Only change Yaw, keep current Pitch and Roll
		FRotator NewRotation(CurrentRotation.Pitch, TargetRotation.Yaw, CurrentRotation.Roll);

		OwningActor->SetActorRotation(NewRotation);

		UE_LOG(LogTemp, Log, TEXT("Rotated %s to face target (Yaw: %.1f)"),
			*OwningActor->GetName(), TargetRotation.Yaw);
	}
}

void UAbility::ExecuteInstant(const FAbilityTargetData& TargetData)
{
	TArray<AActor*> Targets = GetValidTargetsFromData(TargetData);

	for (AActor* Actor : Targets)
	{
		ApplyEffectsToActor(Actor);
	}
}

void UAbility::ExecuteProjectile(const FAbilityTargetData& TargetData)
{
	if (!AbilityData || !OwningActor)
	{
		UE_LOG(LogTemp, Error, TEXT("ExecuteProjectile: AbilityData or OwningActor is null"));
		return;
	}

	const FAbilityDeliveryConfig& DeliveryConfig = AbilityData->DeliveryConfig;

	if (!DeliveryConfig.ProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("ExecuteProjectile: No ProjectileClass set in DeliveryConfig"));
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwningActor;
	SpawnParams.Instigator = Cast<APawn>(OwningActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn ahead of actor to avoid collision with owner
	FVector ForwardOffset = OwningActor->GetActorForwardVector() * 100.f; // 100 units forward
	FVector UpOffset = FVector(0, 0, 80.f); // 80 units up
	FVector SpawnLocation = OwningActor->GetActorLocation() + ForwardOffset + UpOffset;
	FRotator SpawnRotation = OwningActor->GetActorRotation();

	UE_LOG(LogTemp, Warning, TEXT("=== Spawning Projectile ==="));
	UE_LOG(LogTemp, Log, TEXT("Owner: %s at location: %s"), *OwningActor->GetName(), *OwningActor->GetActorLocation().ToString());
	UE_LOG(LogTemp, Log, TEXT("Spawn Location: %s (Forward: %s)"), *SpawnLocation.ToString(), *ForwardOffset.ToString());

	// Choose aim target based on TargetingType
	ETargetingType TargetingType = AbilityData->TargetingConfig.TargetingType;
	FVector AimTarget;

	if (TargetingType == ETargetingType::Unit || TargetingType == ETargetingType::UnitAOE)
	{
		// Unit targeting: Aim at the actor's location
		if (TargetData.TargetActor)
		{
			AimTarget = TargetData.TargetActor->GetActorLocation();
			UE_LOG(LogTemp, Log, TEXT("Unit Targeting: Aiming at actor %s at %s"),
				*TargetData.TargetActor->GetName(), *AimTarget.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Unit Targeting but no TargetActor! Using TargetLocation"));
			AimTarget = TargetData.TargetLocation;
		}
	}
	else
	{
		// Direction/Ground targeting: Use cursor hit location, NOT actor location
		AimTarget = TargetData.TargetLocation;
		UE_LOG(LogTemp, Log, TEXT("Direction/Ground Targeting: Aiming at location %s"), *AimTarget.ToString());
	}

	// Calculate direction and flatten Z
	FVector Direction = AimTarget - SpawnLocation;
	Direction.Z = 0.f; // Flatten for horizontal projectiles
	SpawnRotation = Direction.Rotation();

	// Clamp pitch to prevent extreme angles (optional additional safety)
	SpawnRotation.Pitch = FMath::Clamp(SpawnRotation.Pitch, -30.f, 30.f);

	UE_LOG(LogTemp, Log, TEXT("Spawn Rotation: %s"), *SpawnRotation.ToString());

	if (UWorld* World = GetWorld())
	{
		AProjectile_Base* Projectile = World->SpawnActor<AProjectile_Base>(
			DeliveryConfig.ProjectileClass,
			SpawnLocation,
			SpawnRotation,
			SpawnParams
		);

		if (Projectile)
		{
			// 투사체 충돌 이벤트 바인딩
			Projectile->OnProjectileImpact.AddDynamic(this, &UAbility::OnProjectileHit);

			// 속도, 호밍 등 설정
			if (Projectile->ProjectileMovement)
			{
				Projectile->ProjectileMovement->InitialSpeed = DeliveryConfig.ProjectileSpeed;
				Projectile->ProjectileMovement->MaxSpeed = DeliveryConfig.ProjectileSpeed;

				if (DeliveryConfig.bIsHoming && TargetData.TargetActor)
				{
					Projectile->ProjectileMovement->bIsHomingProjectile = true;
					Projectile->ProjectileMovement->HomingAccelerationMagnitude = DeliveryConfig.HomingAcceleration;
					Projectile->ProjectileMovement->HomingTargetComponent = TargetData.TargetActor->GetRootComponent();
				}
			}

			UE_LOG(LogTemp, Warning, TEXT("Projectile spawned with speed: %.1f"), DeliveryConfig.ProjectileSpeed);
		}
	}

	PlayPresentation(SpawnLocation);
}

void UAbility::ExecuteAOE(const FAbilityTargetData& TargetData)
{
	if (!AbilityData || !OwningActor)
		return;

	const FAbilityDeliveryConfig& DeliveryConfig = AbilityData->DeliveryConfig;

	if (!DeliveryConfig.AOEIndicatorClass)
		return;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwningActor;
	SpawnParams.Instigator = Cast<APawn>(OwningActor);

	FVector SpawnLocation = TargetData.bIsValid ? TargetData.TargetLocation : OwningActor->GetActorLocation();

	if (UWorld* World = GetWorld())
	{
		AAOE_Base* AOE = World->SpawnActor<AAOE_Base>(
			DeliveryConfig.AOEIndicatorClass,
			SpawnLocation,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (AOE)
		{
			// AOE 오버랩 이벤트 바인딩
			AOE->OnOverlapActor.AddDynamic(this, &UAbility::OnAOEOverlap);

			if (DeliveryConfig.DeliveryType == EAbilityDeliveryType::DelayedAOE)
			{
				// 지연 AOE는 일정 시간 후 발동
				FTimerHandle DelayHandle;
				World->GetTimerManager().SetTimer(
					DelayHandle,
					AOE,
					&AAOE_Base::Trigger,
					DeliveryConfig.DelayTime,
					false
				);
			}
			else
			{
				// 즉시 발동
				AOE->Trigger();
			}
		}
	}

	PlayPresentation(SpawnLocation);
}

void UAbility::ApplyEffectsToActor(AActor* Target)
{
	if (!Target || !OwningActor)
	{
		UE_LOG(LogTemp, Error, TEXT("ApplyEffectsToActor: Target or OwningActor is null"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("=== Applying %d Effects to %s ==="), RuntimeEffects.Num(), *Target->GetName());

	for (int32 i = 0; i < RuntimeEffects.Num(); i++)
	{
		UAbilityEffect* Effect = RuntimeEffects[i];
		if (Effect)
		{
			UE_LOG(LogTemp, Log, TEXT("Applying Effect %d: %s"), i, *Effect->GetClass()->GetName());
			Effect->Apply(Target, OwningActor);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Effect %d is null"), i);
		}
	}
}

// ============================================
// Callbacks
// ============================================

void UAbility::OnProjectileHit(AActor* HitActor, FHitResult Hit)
{
	UE_LOG(LogTemp, Warning, TEXT("=== OnProjectileHit Callback ==="));
	UE_LOG(LogTemp, Log, TEXT("Hit Actor: %s"), HitActor ? *HitActor->GetName() : TEXT("None"));
	UE_LOG(LogTemp, Log, TEXT("RuntimeEffects count: %d"), RuntimeEffects.Num());

	if (HitActor)
	{
		ApplyEffectsToActor(HitActor);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("HitActor is null!"));
	}
}

void UAbility::OnAOEOverlap(AActor* OverlappedActor)
{
	if (OverlappedActor)
	{
		ApplyEffectsToActor(OverlappedActor);
	}
}

// ============================================
// Resource Management
// ============================================

void UAbility::ConsumeResources()
{
	if (!AbilityData)
		return;

	// 마나 소비 구현
	if (UCharacterStatComponent* Resource = OwningActor->FindComponentByClass<UCharacterStatComponent>())
	{
	     Resource->UseMana(GetManaCost());
	}

	// 충전 소비
	if (AbilityData->MaxCharges > 1)
	{
		CurrentCharges = FMath::Max(0, CurrentCharges - 1);
	}
}

void UAbility::RefundResources()
{
	// 마나 환불 구현
	if (UCharacterStatComponent* Resource = OwningActor->FindComponentByClass<UCharacterStatComponent>())
	{
		Resource->RestoreMana(GetManaCost());
	}
	// 충전 환불
	if (AbilityData->MaxCharges > 1)
	{
		CurrentCharges = FMath::Min(AbilityData->MaxCharges, CurrentCharges + 1);
	}
}

void UAbility::StartCooldown()
{
	if (!AbilityData)
		return;

	// 충전 시스템이 있으면 충전당 쿨다운
	if (AbilityData->MaxCharges > 1)
	{
		// 충전이 최대가 아니면 쿨다운 시작
		if (CurrentCharges < AbilityData->MaxCharges)
		{
			RemainingCooldown = GetCooldown();
		}
	}
	else
	{
		// 일반 쿨다운
		RemainingCooldown = GetCooldown();
	}
}

// ============================================
// Queries
// ============================================

bool UAbility::CanCast() const
{
	if (!AbilityData || !OwningComponent)
		return false;

	// 레벨 체크
	if (CurrentLevel <= 0)
		return false;

	// 캐스팅 중
	if (bIsCasting)
		return false;

	// 쿨다운 체크
	if (IsOnCooldown())
		return false;

	// 전역 상태 체크 (침묵, 스턴)
	if (OwningComponent->bIsSilenced)
		return false;

	if (OwningComponent->bIsStunned)
		return false;

	// 마나 체크
	if (UCharacterStatComponent* Resource = OwningActor->FindComponentByClass<UCharacterStatComponent>())
	{
		if (GetManaCost() > Resource->CurrentMana)
			return false;
	}

	return true;
}

bool UAbility::IsMaxLevel() const
{
	if (!AbilityData)
		return false;

	// LevelScaling 배열의 크기가 최대 레벨
	return CurrentLevel >= AbilityData->LevelScaling.Num();
}

float UAbility::GetManaCost() const
{
	if (!AbilityData)
		return 0.0f;

	float Cost = AbilityData->ManaCost;

	// 레벨별 증가
	if (CurrentLevel > 0 && AbilityData->LevelScaling.IsValidIndex(CurrentLevel - 1))
	{
		Cost += AbilityData->LevelScaling[CurrentLevel - 1].ManaCostIncrease;
	}

	return Cost;
}

float UAbility::GetCooldown() const
{
	if (!AbilityData)
		return 0.0f;

	float CD = AbilityData->Cooldown;

	// 레벨별 감소
	if (CurrentLevel > 0 && AbilityData->LevelScaling.IsValidIndex(CurrentLevel - 1))
	{
		CD -= AbilityData->LevelScaling[CurrentLevel - 1].CooldownReduction;
	}

	return FMath::Max(0.0f, CD);
}

float UAbility::GetRange() const
{
	if (!AbilityData)
		return 0.0f;

	float Range = AbilityData->TargetingConfig.Range;

	// 레벨별 증가
	if (CurrentLevel > 0 && AbilityData->LevelScaling.IsValidIndex(CurrentLevel - 1))
	{
		Range += AbilityData->LevelScaling[CurrentLevel - 1].RangeIncrease;
	}

	return Range;
}

float UAbility::GetCooldownPercent() const
{
	float TotalCooldown = GetCooldown();
	if (TotalCooldown <= 0.0f)
		return 0.0f;

	return 1.0f - (RemainingCooldown / TotalCooldown);
}

void UAbility::UpdateCooldown(float DeltaTime)
{
	if (RemainingCooldown > 0.0f)
	{
		RemainingCooldown -= DeltaTime;

		// 충전 시스템: 쿨다운 완료 시 충전 회복
		if (RemainingCooldown <= 0.0f && AbilityData && AbilityData->MaxCharges > 1)
		{
			CurrentCharges = FMath::Min(AbilityData->MaxCharges, CurrentCharges + 1);

			// 아직 충전이 최대가 아니면 다음 충전 쿨다운 시작
			if (CurrentCharges < AbilityData->MaxCharges)
			{
				RemainingCooldown = GetCooldown();
			}
			else
			{
				RemainingCooldown = 0.0f;
			}
		}
		else if (RemainingCooldown <= 0.0f)
		{
			RemainingCooldown = 0.0f;
		}
	}
}

void UAbility::CheckPendingExecution()
{
	if (!bHasPendingExecution || !PendingExecutionTargetData.TargetActor)
		return;

	// Check if target is now in range
	float Distance = FVector::Dist(OwningActor->GetActorLocation(), PendingExecutionTargetData.TargetActor->GetActorLocation());
	float Range = AbilityData->TargetingConfig.Range;

	if (Distance <= Range)
	{
		UE_LOG(LogTemp, Log, TEXT("Now in range (%.1f <= %.1f). Executing ability!"), Distance, Range);

		// Clear pending state
		bHasPendingExecution = false;

		// Execute the ability
		Execute(PendingExecutionTargetData);
	}
}

void UAbility::CancelPendingExecution()
{
	if (bHasPendingExecution)
	{
		UE_LOG(LogTemp, Log, TEXT("Cancelled pending ability execution"));
		bHasPendingExecution = false;
		PendingExecutionTargetData = FAbilityTargetData();
	}
}

// ============================================
// Helpers
// ============================================

TArray<AActor*> UAbility::GetValidTargetsFromData(const FAbilityTargetData& TargetData)
{
	if (!TargetingStrategy)
		return TArray<AActor*>();

	// TargetingStrategy를 사용하여 유효한 타겟 수집
	return TargetingStrategy->GetValidTargets(TargetData);
}

void UAbility::PlayPresentation(const FVector& Location)
{
	if (!AbilityData || !OwningActor)
		return;

	UWorld* World = GetWorld();
	if (!World)
		return;

	// VFX 재생
	if (AbilityData->CastVFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			World,
			AbilityData->CastVFX,
			Location
		);
	}

	// SFX 재생
	if (AbilityData->CastSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			World,
			AbilityData->CastSound,
			Location
		);
	}
}
