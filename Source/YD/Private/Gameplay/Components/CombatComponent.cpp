// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Components/CombatComponent.h"
#include "Gameplay/Components/CharacterStatComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	CurrentTarget = nullptr;
	TimeSinceLastAttack = 0.f;
	AttackCooldown = 1.f;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	// Get reference to stat component
	AActor* Owner = GetOwner();
	if (Owner)
	{
		StatComponent = Owner->FindComponentByClass<UCharacterStatComponent>();
		if (StatComponent)
		{
			UpdateAttackCooldown();
		}
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update attack cooldown based on current attack speed
	UpdateAttackCooldown();

	// If we have a target, try to attack
	if (CurrentTarget)
	{
		// Check if target is still valid
		if (!IsTargetValid())
		{
			ClearTarget();
			return;
		}

		// Increment attack timer
		TimeSinceLastAttack += DeltaTime;

		// Check if we can attack (cooldown ready and in range)
		if (TimeSinceLastAttack >= AttackCooldown)
		{
			if (IsInAttackRange(CurrentTarget))
			{
				PerformAttack();
				TimeSinceLastAttack = 0.f;
			}
			else
			{
				// Debug: Not in range
				float Distance = FVector::Dist(GetOwner()->GetActorLocation(), CurrentTarget->GetActorLocation());
				float AttackRange = StatComponent ? StatComponent->GetCurrentAttackRange() : 0.f;
				UE_LOG(LogTemp, Warning, TEXT("%s: Target out of range. Distance: %.1f, AttackRange: %.1f"),
					*GetOwner()->GetName(), Distance, AttackRange);
			}
		}
		else
		{
			// Debug: Cooldown not ready
			UE_LOG(LogTemp, Log, TEXT("%s: Attack on cooldown. Time: %.2f / %.2f"),
				*GetOwner()->GetName(), TimeSinceLastAttack, AttackCooldown);
		}
	}
}

void UCombatComponent::SetTarget(AActor* NewTarget)
{
	if (NewTarget == CurrentTarget)
		return;

	CurrentTarget = NewTarget;
	TimeSinceLastAttack = 0.f;

	if (CurrentTarget)
	{
		UE_LOG(LogTemp, Log, TEXT("%s targeting %s"), *GetOwner()->GetName(), *CurrentTarget->GetName());
	}
}

void UCombatComponent::ClearTarget()
{
	CurrentTarget = nullptr;
	UE_LOG(LogTemp, Log, TEXT("%s cleared target"), *GetOwner()->GetName());
}

bool UCombatComponent::IsInAttackRange(AActor* Target) const
{
	if (!Target || !StatComponent)
		return false;

	AActor* Owner = GetOwner();
	if (!Owner)
		return false;

	float Distance = FVector::Dist(Owner->GetActorLocation(), Target->GetActorLocation());
	float AttackRange = StatComponent->GetCurrentAttackRange();

	return Distance <= AttackRange;
}

void UCombatComponent::Attack(AActor* Target)
{
	if (!Target)
		return;

	SetTarget(Target);

	// If already in range, attack immediately
	if (IsInAttackRange(Target) && TimeSinceLastAttack >= AttackCooldown)
	{
		PerformAttack();
		TimeSinceLastAttack = 0.f;
	}
}

void UCombatComponent::PerformAttack()
{
	if (!CurrentTarget || !StatComponent)
		return;

	// Broadcast attack started event (for animation)
	OnAttackStarted.Broadcast(CurrentTarget);

	// Get attack damage from stat component
	float AttackDamage = StatComponent->GetCurrentAttackDamage();

	// Find target's stat component and deal damage
	UCharacterStatComponent* TargetStatComponent = CurrentTarget->FindComponentByClass<UCharacterStatComponent>();
	if (TargetStatComponent)
	{
		TargetStatComponent->TakeDamage(AttackDamage, GetOwner());
	}

	// Broadcast attack hit event (for effects/sounds)
	OnAttackHit.Broadcast(CurrentTarget);

	UE_LOG(LogTemp, Log, TEXT("%s attacked %s for %.1f damage"),
		*GetOwner()->GetName(),
		*CurrentTarget->GetName(),
		AttackDamage);
}

void UCombatComponent::UpdateAttackCooldown()
{
	if (StatComponent)
	{
		float AttackSpeed = StatComponent->GetCurrentAttackSpeed();
		if (AttackSpeed > 0.f)
		{
			AttackCooldown = 1.f / AttackSpeed;
		}
	}
}

bool UCombatComponent::IsTargetValid() const
{
	if (!CurrentTarget)
		return false;

	// Check if target is destroyed or pending kill
	if (!IsValid(CurrentTarget))
		return false;

	// Check if target has stat component and is alive
	UCharacterStatComponent* TargetStatComponent = CurrentTarget->FindComponentByClass<UCharacterStatComponent>();
	if (TargetStatComponent)
	{
		return TargetStatComponent->IsAlive();
	}

	// If no stat component, target is invalid (can't attack objects without health)
	return false;
}
