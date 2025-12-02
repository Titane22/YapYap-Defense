// Fill out your copyright notice in the Description page of Project Settings.

#include "Gameplay/Characters/Enemy/Enemy_Base.h"
#include "Gameplay/Components/CharacterStatComponent.h"
#include "Gameplay/Components/CombatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"
#include "AIController.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"

AEnemy_Base::AEnemy_Base()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set default values
	GoldReward = 20;
	ExperienceReward = 30;
	DetectionRange = 800.f;
	DetectionTimer = 0.f;
	MovementTarget = nullptr;

	// Add "Enemy" tag for identification
	Tags.Add(FName("Enemy"));

	// Configure character movement for minion
	GetCharacterMovement()->MaxWalkSpeed = 325.f;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f);

	// Auto possess AI controller
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = nullptr; // Use default or set in Blueprint
}

void AEnemy_Base::BeginPlay()
{
	Super::BeginPlay();

	// Bind to stat component death event
	if (CharacterStatComponent)
	{
		CharacterStatComponent->OnDeath.AddDynamic(this, &AEnemy_Base::HandleDeath);
	}

	// Bind to combat component attack event for animation
	if (CombatComponent)
	{
		CombatComponent->OnAttackStarted.AddDynamic(this, &AEnemy_Base::OnAttackStartedHandler);
	}

	// Ensure AI controller is assigned
	if (!GetController())
	{
		SpawnDefaultController();
		UE_LOG(LogTemp, Warning, TEXT("Enemy_Base: No controller! Spawned default controller."));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Enemy_Base: Has controller: %s"), *GetController()->GetName());
	}

	// Get references to components (inherited from YDCharacter)
	// StatComponent and CombatComponent are already created in parent class
}

void AEnemy_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update detection timer
	DetectionTimer += DeltaTime;
	if (DetectionTimer >= 0.5f) // Check every 0.5 seconds
	{
		DetectionTimer = 0.f;
		SearchForEnmies();
	}

	if (!CombatComponent || !GetController())
		return;

	// Get current target (either combat target or movement target)
	AActor* CurrentTargetActor = CombatComponent->GetTarget();
	if (!CurrentTargetActor && MovementTarget)
	{
		CurrentTargetActor = MovementTarget;
	}

	// If we have a target, handle movement and combat
	if (CurrentTargetActor)
	{
		// Check if we're in attack range
		float DistanceToTarget = FVector::Dist(GetActorLocation(), CurrentTargetActor->GetActorLocation());
		UCharacterStatComponent* Stats = FindComponentByClass<UCharacterStatComponent>();
		float AttackRange = Stats ? Stats->GetCurrentAttackRange() : 150.f;

		if (DistanceToTarget <= AttackRange)
		{
			// In range - set as combat target if not already
			if (!CombatComponent->GetTarget())
			{
				CombatComponent->SetTarget(CurrentTargetActor);
			}
			// Stop moving when in attack range
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(GetController(), GetActorLocation());
		}
		else
		{
			// Not in range - keep moving towards target
			UAIBlueprintHelperLibrary::SimpleMoveToActor(GetController(), CurrentTargetActor);
		}
	}
}

void AEnemy_Base::OnDeath(AActor* Killer)
{
	// Broadcast death event for reward handling
	OnEnemyDeath.Broadcast(this, Killer);

	// Stop all movement
	if (AController* MyController = GetController())
	{
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(MyController, GetActorLocation());
	}

	// Clear combat target
	if (CombatComponent)
	{
		CombatComponent->ClearTarget();
	}

	// TODO: Play death animation, spawn death effects, etc.
	UE_LOG(LogTemp, Log, TEXT("Enemy %s killed by %s"), *GetName(), Killer ? *Killer->GetName() : TEXT("Unknown"));

	// Destroy after delay (for death animation)
	SetLifeSpan(3.0f);
}

void AEnemy_Base::SetMovementTarget(AActor* NewTarget)
{
	MovementTarget = NewTarget;
	UE_LOG(LogTemp, Log, TEXT("Enemy %s movement target set to %s"), *GetName(), NewTarget ? *NewTarget->GetName() : TEXT("None"));
}

void AEnemy_Base::SearchForEnmies()
{
	// If already attacking something, don't search
	if (CombatComponent && CombatComponent->GetTarget())
		return;

	// Sphere overlap to find enemies in detection range
	TArray<FOverlapResult> OverlapResults;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(DetectionRange);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	// Overlap with Pawn channel
	GetWorld()->OverlapMultiByChannel(
		OverlapResults,
		GetActorLocation(),
		FQuat::Identity,
		ECC_Pawn,
		Sphere,
		QueryParams
	);

	AActor* ClosestEnemy = nullptr;
	float ClosestDistance = DetectionRange;

	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* Actor = Result.GetActor();
		if (!Actor || !IsEnemy(Actor))
			continue;

		// Check if target is alive
		UCharacterStatComponent* TargetStats = Actor->FindComponentByClass<UCharacterStatComponent>();
		if (!TargetStats || !TargetStats->IsAlive())
			continue;

		// Find closest enemy
		float Distance = FVector::Dist(GetActorLocation(), Actor->GetActorLocation());
		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosestEnemy = Actor;
		}
	}

	// Attack closest enemy if found
	if (ClosestEnemy && CombatComponent)
	{
		CombatComponent->SetTarget(ClosestEnemy);
		UE_LOG(LogTemp, Log, TEXT("%s found enemy target: %s"), *GetName(), *ClosestEnemy->GetName());
	}
}

bool AEnemy_Base::IsEnemy(AActor* Actor) const
{
	if (!Actor)
		return false;

	// Enemies attack players (anything without "Enemy" tag)
	return !Actor->ActorHasTag(FName("Enemy"));
}

void AEnemy_Base::HandleDeath()
{
	// Get killer from stat component's last damage dealer
	AActor* Killer = nullptr;
	// You might want to track last damage dealer in CharacterStatComponent
	// For now, use nullptr

	OnDeath(Killer);
}

void AEnemy_Base::OnAttackStartedHandler(AActor* Target)
{
	// Play attack montage if set
	if (AttackMontage)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			// Stop current montage and play new one for responsive attacks
			if (AnimInstance->IsAnyMontagePlaying())
			{
				AnimInstance->Montage_Stop(0.2f);
			}
			AnimInstance->Montage_Play(AttackMontage, 1.0f);
			UE_LOG(LogTemp, Log, TEXT("%s playing attack montage on %s"), *GetName(), Target ? *Target->GetName() : TEXT("None"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("%s has no AnimInstance!"), *GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s has no AttackMontage set!"), *GetName());
	}
}

