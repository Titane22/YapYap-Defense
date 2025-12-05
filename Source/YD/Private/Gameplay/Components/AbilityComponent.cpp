// Fill out your copyright notice in the Description page of Project Settings.

#include "Gameplay/Components/AbilityComponent.h"
#include "Gameplay/Data/AbilityData.h"
#include "Gameplay/Data/Character_Data.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/WindDirectionalSourceComponent.h"
#include "Sound/SoundBase.h"

UAbilityComponent::UAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAbilityComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize cooldown map
	AbilityCooldowns.Add(EAbilitySlot::Q, 0.0f);
	AbilityCooldowns.Add(EAbilitySlot::W, 0.0f);
	AbilityCooldowns.Add(EAbilitySlot::E, 0.0f);
	AbilityCooldowns.Add(EAbilitySlot::R, 0.0f);
}

void UAbilityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateCooldowns(DeltaTime);
}

void UAbilityComponent::InitializeAbilities(UCharacter_Data* CharacterData)
{
	if (!CharacterData)
	{
		UE_LOG(LogTemp, Warning, TEXT("AbilityComponent: Cannot initialize abilities - CharacterData is null"));
		return;
	}

	// Set abilities from character data
	SetAbility(EAbilitySlot::Q, CharacterData->QAbility);
	SetAbility(EAbilitySlot::W, CharacterData->WAbility);
	SetAbility(EAbilitySlot::E, CharacterData->EAbility);
	SetAbility(EAbilitySlot::R, CharacterData->RAbility);

	UE_LOG(LogTemp, Log, TEXT("AbilityComponent: Initialized abilities for %s"), *CharacterData->CharacterName.ToString());
}

void UAbilityComponent::SetAbility(EAbilitySlot Slot, UAbilityData* AbilityData)
{
	Abilities.Add(Slot, AbilityData);

	if (AbilityData)
	{
		UE_LOG(LogTemp, Log, TEXT("AbilityComponent: Set ability '%s' to slot %d"),
			*AbilityData->AbilityName.ToString(), static_cast<int32>(Slot));
	}
}

UAbilityData* UAbilityComponent::GetAbility(EAbilitySlot Slot) const
{
	if (auto FoundAbility = Abilities.Find(Slot))
	{
		return *FoundAbility;
	}
	return nullptr;
}

bool UAbilityComponent::TryUseAbility(EAbilitySlot Slot, AActor* Target)
{
	UAbilityData* AbilityData = GetAbility(Slot);

	if (!AbilityData)
	{
		UE_LOG(LogTemp, Warning, TEXT("AbilityComponent: No ability in slot %d"), static_cast<int32>(Slot));
		return false;
	}

	if (!CanUseAbility(Slot, AbilityData))
	{
		return false;
	}

	// Execute ability
	ExecuteAbility(Slot, AbilityData, Target);

	// Start cooldown
	StartCooldown(Slot, AbilityData->Cooldown);

	// Broadcast event
	OnAbilityUsed.Broadcast(Slot, AbilityData);

	return true;
}

bool UAbilityComponent::UseQAbility(AActor* Target)
{
	return TryUseAbility(EAbilitySlot::Q, Target);
}

bool UAbilityComponent::UseWAbility(AActor* Target)
{
	return TryUseAbility(EAbilitySlot::W, Target);
}

bool UAbilityComponent::UseEAbility(AActor* Target)
{
	return TryUseAbility(EAbilitySlot::E, Target);
}

bool UAbilityComponent::UseRAbility(AActor* Target)
{
	return TryUseAbility(EAbilitySlot::R, Target);
}

bool UAbilityComponent::IsAbilityOnCooldown(EAbilitySlot Slot) const
{
	const float* Cooldown = AbilityCooldowns.Find(Slot);
	return Cooldown && *Cooldown > 0.0f;
}

float UAbilityComponent::GetAbilityCooldownRemaining(EAbilitySlot Slot) const
{
	const float* Cooldown = AbilityCooldowns.Find(Slot);
	return Cooldown ? *Cooldown : 0.0f;
}

float UAbilityComponent::GetAbilityCooldownPercent(EAbilitySlot Slot) const
{
	const UAbilityData* AbilityData = GetAbility(Slot);
	if (!AbilityData || AbilityData->Cooldown <= 0.0f)
	{
		return 0.0f;
	}

	const float Remaining = GetAbilityCooldownRemaining(Slot);
	return FMath::Clamp(Remaining / AbilityData->Cooldown, 0.0f, 1.0f);
}

bool UAbilityComponent::ResolveTargetData(UAbilityData* AbilityData, AActor* Target, FVector& OutTargetLocation,
	FVector& OutLookDirection)
{
	if (!AbilityData || !GetOwner())
	{
		return false;
	}

	AActor* Owner = GetOwner();
	FVector OwnerLocation = Owner->GetActorLocation();

	switch (AbilityData->TargetType)
	{
		case EAbilityTargetType::Self:
			// Self: No location/rotation needed
			OutTargetLocation = OwnerLocation;
			OutLookDirection = Owner->GetActorForwardVector();
			return true;

		case EAbilityTargetType::SingleTarget:
			if (!Target)
			{
				UE_LOG(LogTemp, Warning, TEXT("ResolveTargetData: SingleTarget ability requires a target"));
				return false;
			}

			OutTargetLocation = Target->GetActorLocation();

			// Range check
			if (AbilityData->Range > 0.0f && !IsTargetInRange(OutTargetLocation, AbilityData->Range))
			{
				UE_LOG(LogTemp, Warning, TEXT("ResolveTargetData: Target out of range for ability '%s'"),
					*AbilityData->AbilityName.ToString());
				return false;
			}

			// Calculate and normalize direction
			OutLookDirection = (OutTargetLocation - OwnerLocation).GetSafeNormal();
			if (OutLookDirection.IsNearlyZero())
			{
				OutLookDirection = Owner->GetActorForwardVector();
			}
			return true;

		case EAbilityTargetType::GroundTarget:
		{
			// Get cursor location
			APlayerController* PC = GetWorld()->GetFirstPlayerController();
			if (!PC)
			{
				UE_LOG(LogTemp, Warning, TEXT("ResolveTargetData: Cannot get cursor location - no PlayerController"));
				return false;
			}

			FHitResult Hit;
			if (!PC->GetHitResultUnderCursor(ECC_Visibility, true, Hit))
			{
				UE_LOG(LogTemp, Warning, TEXT("ResolveTargetData: Failed to get ground target - cursor not over valid surface"));
				return false;
			}

			OutTargetLocation = Hit.Location;

			// Range check
			if (AbilityData->Range > 0.0f && !IsTargetInRange(OutTargetLocation, AbilityData->Range))
			{
				UE_LOG(LogTemp, Warning, TEXT("ResolveTargetData: Target out of range for ability '%s'"),
					*AbilityData->AbilityName.ToString());
				return false;
			}

			// Calculate and normalize direction
			OutLookDirection = (OutTargetLocation - OwnerLocation).GetSafeNormal();
			if (OutLookDirection.IsNearlyZero())
			{
				OutLookDirection = Owner->GetActorForwardVector();
			}
			return true;
		}

		case EAbilityTargetType::Directional:
			// Direction: Use forward vector directly (already normalized), no range check
			OutLookDirection = Owner->GetActorForwardVector();
			OutTargetLocation = OwnerLocation + (OutLookDirection * AbilityData->Range);
			return true;

		case EAbilityTargetType::None:
		default:
			UE_LOG(LogTemp, Warning, TEXT("ResolveTargetData: Unknown or None target type for ability '%s'"),
				*AbilityData->AbilityName.ToString());
			return false;
	}
}

void UAbilityComponent::ExecuteAbility(EAbilitySlot Slot, UAbilityData* AbilityData, AActor* Target)
{
	// 1. Validation
	if (!AbilityData || !GetOwner())
	{
		return;
	}

	// 2. Validate target requirement
	if (AbilityData->bRequiresTarget && !Target)
	{
		UE_LOG(LogTemp, Warning, TEXT("AbilityComponent: Ability '%s' requires a target but none was provided"),
			*AbilityData->AbilityName.ToString());
		return;
	}

	// 3. Resolve target data (location, direction, range check, etc.)
	FVector TargetLocation = FVector::ZeroVector;
	FVector LookDirection = FVector::ForwardVector; 

	if (!ResolveTargetData(AbilityData, Target, TargetLocation, LookDirection))
	{
		// Failed to resolve target data (out of range, invalid target, etc.)
		UE_LOG(LogTemp, Warning, TEXT("AbilityComponent: Failed to resolve target for ability '%s'"),
		*AbilityData->AbilityName.ToString());
		return;
	}

	// 4. Play common visuals
	FRotator SpawnRotation = LookDirection.Rotation();
	PlayAbilityVisuals(AbilityData, SpawnRotation);

	// 5. Execute ability based on target type
	switch (AbilityData->TargetType)
	{
		case EAbilityTargetType::Self:
			PerformSelfAbility(AbilityData);
			break;

		case EAbilityTargetType::SingleTarget:
			PerformSingleAbility(AbilityData, Target);
			break;

		case EAbilityTargetType::GroundTarget:
			PerformGorundAbility(AbilityData, TargetLocation);
			break;

		case EAbilityTargetType::Directional:
			PerformDirecitonalAbility(AbilityData, LookDirection);
			break;
	}

	UE_LOG(LogTemp, Log, TEXT("AbilityComponent: Used ability '%s' from slot %d"),
		*AbilityData->AbilityName.ToString(), static_cast<int32>(Slot));
}

bool UAbilityComponent::CanUseAbility(EAbilitySlot Slot, UAbilityData* AbilityData) const
{
	if (!AbilityData)
	{
		return false;
	}

	// Check cooldown
	if (IsAbilityOnCooldown(Slot))
	{
		UE_LOG(LogTemp, Warning, TEXT("AbilityComponent: Ability on cooldown (%.1fs remaining)"),
			GetAbilityCooldownRemaining(Slot));
		return false;
	}

	// TODO: Check mana/resources
	// TODO: Check if target is required but missing

	return true;
}

void UAbilityComponent::StartCooldown(EAbilitySlot Slot, float Duration)
{
	if (Duration <= 0.0f)
	{
		return;
	}

	AbilityCooldowns.Add(Slot, Duration);
	OnAbilityCooldownStarted.Broadcast(Slot, Duration);

	UE_LOG(LogTemp, Log, TEXT("AbilityComponent: Started cooldown for slot %d (%.1fs)"),
		static_cast<int32>(Slot), Duration);
}

void UAbilityComponent::UpdateCooldowns(float DeltaTime)
{
	TArray<EAbilitySlot> FinishedCooldowns;

	for (auto& Pair : AbilityCooldowns)
	{
		if (Pair.Value > 0.0f)
		{
			Pair.Value -= DeltaTime;

			if (Pair.Value <= 0.0f)
			{
				Pair.Value = 0.0f;
				FinishedCooldowns.Add(Pair.Key);
			}
		}
	}

	// Broadcast finished cooldowns
	for (EAbilitySlot Slot : FinishedCooldowns)
	{
		OnAbilityCooldownFinished.Broadcast(Slot);
	}
}

bool UAbilityComponent::IsTargetInRange(const FVector& TargetLocation, float Range) const
{
	if (!GetOwner())
	{
		return false;
	}

	float Distance = FVector::Dist(GetOwner()->GetActorLocation(), TargetLocation);
	return Distance <= Range;
}

void UAbilityComponent::PlayAbilityVisuals(UAbilityData* AbilityData, const FRotator& SpawnRotation)
{
	if (!AbilityData || !GetOwner())
	{
		return;
	}

	AActor* Owner = GetOwner();
	FVector SpawnLocation = Owner->GetActorLocation();

	// Play animation
	if (AbilityData->AbilityMontage)
	{
		if (ACharacter* Character = Cast<ACharacter>(Owner))
		{
			if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
			{
				AnimInstance->Montage_Play(AbilityData->AbilityMontage);
			}
		}
	}

	// Spawn particle effect
	if (AbilityData->AbilityEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), AbilityData->AbilityEffect, SpawnLocation, SpawnRotation);
	}

	// Spawn Niagara effect
	if (AbilityData->NiagaraEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), AbilityData->NiagaraEffect, SpawnLocation, SpawnRotation);
	}

	// Play sound
	if (AbilityData->AbilitySound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), AbilityData->AbilitySound, SpawnLocation);
	}
}

void UAbilityComponent::PerformSelfAbility(UAbilityData* AbilityData)
{
	AActor* Owner = GetOwner();
	if (!Owner)
		return;

	// TODO: Spawn actor as Ability Attack Type(etc. Projectile, AOE, None)
	
	switch (AbilityData->AbilityType)
	{
	case EAbilityType::Buff:
		ApplyBuff(Owner, AbilityData);
		break;
	case EAbilityType::Damage:
		ApplyDamage(Owner, AbilityData->BasePower);
		break;
	case EAbilityType::Heal:
		ApplyHeal(Owner, AbilityData->BasePower);
		break;
	case EAbilityType::Debuff:
		ApplyDebuff(Owner, AbilityData);
		break;
	}
}

void UAbilityComponent::PerformSingleAbility(UAbilityData* AbilityData, AActor* Target)
{
	if (!Target)
		return;
	
	// TODO: Spawn actor as Ability Attack Type(etc. Projectile, AOE, None)
	
	switch (AbilityData->AbilityType)
	{
	case EAbilityType::Buff:
		ApplyBuff(Target, AbilityData);
		break;
	case EAbilityType::Damage:
		ApplyDamage(Target, AbilityData->BasePower);
		break;
	case EAbilityType::Heal:
		ApplyHeal(Target, AbilityData->BasePower);
		break;
	case EAbilityType::Debuff:
		ApplyDebuff(Target, AbilityData);
		break;
	}
}

void UAbilityComponent::PerformGorundAbility(UAbilityData* AbilityData, FVector TargetLocation)
{
	
}

void UAbilityComponent::PerformDirecitonalAbility(UAbilityData* AbilityData, FVector TargetDirection)
{
}

void UAbilityComponent::ApplyDamage(AActor* Target, float Damage)
{
}

void UAbilityComponent::ApplyHeal(AActor* Target, float HealAmount)
{
}

void UAbilityComponent::ApplyBuff(AActor* Target, UAbilityData* AbilityData)
{
}

void UAbilityComponent::ApplyDebuff(AActor* Target, UAbilityData* AbilityData)
{
}
