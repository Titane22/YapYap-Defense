// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/Components/CombatComponent.h"
#include "Gameplay/Components/CharacterStatComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

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
			CancelAttackMontage();
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
			// During attack animation (cooldown not ready)
			// Check if target moved out of range and cancel montage if needed
			if (!IsInAttackRange(CurrentTarget))
			{
				UE_LOG(LogTemp, Warning, TEXT("%s: Target escaped during attack animation, cancelling"), *GetOwner()->GetName());
				CancelAttackMontage();
				// Don't clear target, just reset cooldown to try again
				TimeSinceLastAttack = AttackCooldown;
			}
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

	// Only broadcast attack started event (for animation)
	// Damage will be applied later via AnimNotify_MeleeAttack
	OnAttackStarted.Broadcast(CurrentTarget);

	UE_LOG(LogTemp, Log, TEXT("%s started attack animation on %s"),
		*GetOwner()->GetName(),
		*CurrentTarget->GetName());
}

void UCombatComponent::ApplyMeleeDamage()
{
	if (!CurrentTarget || !StatComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: ApplyMeleeDamage called but no valid target"), *GetOwner()->GetName());
		return;
	}

	// Check if target is still in range
	if (!IsInAttackRange(CurrentTarget))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Target %s out of range, damage not applied"),
			*GetOwner()->GetName(), *CurrentTarget->GetName());
		return;
	}

	// Get attack damage from stat component
	float AttackDamage = StatComponent->GetCurrentAttackDamage();

	// Find target's stat component and deal damage
	UCharacterStatComponent* TargetStatComponent = CurrentTarget->FindComponentByClass<UCharacterStatComponent>();
	if (TargetStatComponent)
	{
		TargetStatComponent->TakeDamage(AttackDamage, GetOwner());

		// Broadcast attack hit event (for effects/sounds)
		OnAttackHit.Broadcast(CurrentTarget);

		UE_LOG(LogTemp, Log, TEXT("%s dealt %.1f damage to %s"),
			*GetOwner()->GetName(),
			AttackDamage,
			*CurrentTarget->GetName());
	}
}

void UCombatComponent::CancelAttackMontage()
{
	AActor* Owner = GetOwner();
	if (!Owner)
		return;

	// Get the character's mesh and anim instance
	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		USkeletalMeshComponent* Mesh = Character->GetMesh();
		if (Mesh)
		{
			UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
			if (AnimInstance && AnimInstance->IsAnyMontagePlaying())
			{
				AnimInstance->Montage_Stop(0.2f);
				UE_LOG(LogTemp, Log, TEXT("%s: Attack montage cancelled"), *Owner->GetName());
			}
		}
	}
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
