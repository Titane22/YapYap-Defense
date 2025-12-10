// Fill out your copyright notice in the Description page of Project Settings.

#include "Gameplay/Components/CharacterStatComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UCharacterStatComponent::UCharacterStatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UCharacterStatComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize current stats from base stats
	CurrentHealth = BaseMaxHealth;
	CurrentMana = BaseMaxMana;
	RecalculateStats();
}

void UCharacterStatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update timed modifiers (buffs/debuffs with duration)
	UpdateTimedModifiers(DeltaTime);
}

void UCharacterStatComponent::TakeDamage(float Damage, AActor* DamageDealer)
{
	if (!IsAlive())
		return;

	// Calculate damage reduction from armor
	// Formula: Damage Reduction = Armor / (Armor + 100)
	float DamageReduction = CurrentArmor / (CurrentArmor + 100.f);
	float ActualDamage = Damage * (1.f - DamageReduction);

	CurrentHealth = FMath::Max(0.f, CurrentHealth - ActualDamage);

	// Broadcast health changed event
	OnHealthChanged.Broadcast(CurrentHealth, CurrentMaxHealth);

	// Check for death
	if (CurrentHealth <= 0.f)
	{
		Die();
	}

	UE_LOG(LogTemp, Log, TEXT("%s took %.1f damage (%.1f after armor). Health: %.1f/%.1f"),
		*GetOwner()->GetName(), Damage, ActualDamage, CurrentHealth, CurrentMaxHealth);
}

void UCharacterStatComponent::UseMana(float Amount)
{
	if (!IsAlive())
		return;

	CurrentMana = FMath::Max(0.f, CurrentMana - Amount);

	OnManaChanged.Broadcast(CurrentMana, CurrentMaxMana);
	UE_LOG(LogTemp, Log, TEXT("%s healed %.1f. Health: %.1f/%.1f"),
		*GetOwner()->GetName(), Amount, CurrentMana, CurrentMaxMana);
}

void UCharacterStatComponent::Heal(float Amount)
{
	if (!IsAlive())
		return;

	CurrentHealth = FMath::Min(CurrentMaxHealth, CurrentHealth + Amount);
	OnHealthChanged.Broadcast(CurrentHealth, CurrentMaxHealth);

	UE_LOG(LogTemp, Log, TEXT("%s healed %.1f. Health: %.1f/%.1f"),
		*GetOwner()->GetName(), Amount, CurrentHealth, CurrentMaxHealth);
}

void UCharacterStatComponent::RestoreMana(float Amount)
{
	if (!IsAlive())
		return;

	CurrentMana = FMath::Min(CurrentMaxMana, CurrentMana + Amount);
	OnManaChanged.Broadcast(CurrentMana, CurrentMaxMana);

	UE_LOG(LogTemp, Log, TEXT("%s healed %.1f. Health: %.1f/%.1f"),
		*GetOwner()->GetName(), Amount, CurrentMana, CurrentMaxMana);
}

void UCharacterStatComponent::AddStatModifier(FStatModifier Modifier)
{
	// Check if modifier with same name already exists
	int32 ExistingIndex = StatModifiers.IndexOfByPredicate([&](const FStatModifier& Mod) {
		return Mod.ModifierName == Modifier.ModifierName;
	});

	if (ExistingIndex != INDEX_NONE)
	{
		// Replace existing modifier (refresh duration)
		StatModifiers[ExistingIndex] = Modifier;
		StatModifiers[ExistingIndex].RemainingTime = Modifier.Duration;
	}
	else
	{
		// Add new modifier
		Modifier.RemainingTime = Modifier.Duration;
		StatModifiers.Add(Modifier);
	}

	RecalculateStats();
	OnStatsUpdated.Broadcast(Modifier);

	UE_LOG(LogTemp, Log, TEXT("%s added stat modifier: %s (Duration: %.1f)"),
		*GetOwner()->GetName(), *Modifier.ModifierName.ToString(), Modifier.Duration);
}

void UCharacterStatComponent::RemoveStatModifier(FName ModifierName)
{
	int32 RemovedCount = StatModifiers.RemoveAll([&](const FStatModifier& Mod) {
		return Mod.ModifierName == ModifierName;
	});

	if (RemovedCount > 0)
	{
		RecalculateStats();
		UE_LOG(LogTemp, Log, TEXT("%s removed stat modifier: %s"),
			*GetOwner()->GetName(), *ModifierName.ToString());
	}
}

void UCharacterStatComponent::RecalculateStats()
{
	// Start with base stats
	CurrentMaxHealth = BaseMaxHealth;
	CurrentMaxMana = BaseMaxMana;
	CurrentAttackDamage = BaseAttackDamage;
	CurrentArmor = BaseArmor;
	CurrentMoveSpeed = BaseMoveSpeed;
	CurrentAttackSpeed = BaseAttackSpeed;
	CurrentAttackRange = BaseAttackRange;

	// Apply all active modifiers
	for (const FStatModifier& Modifier : StatModifiers)
	{
		CurrentMaxHealth += Modifier.HealthModifier;
		CurrentMaxMana += Modifier.ManaModifier;
		CurrentAttackDamage += Modifier.AttackDamageModifier;
		CurrentArmor += Modifier.ArmorModifier;
		CurrentMoveSpeed += Modifier.MoveSpeedModifier;
		CurrentAttackSpeed += Modifier.AttackSpeedModifier;
	}

	// Clamp current health to new max health & Mana
	CurrentHealth = FMath::Min(CurrentHealth, CurrentMaxHealth);
	CurrentMana = FMath::Min(CurrentMaxMana, CurrentMaxMana);
	
	// Update character movement speed if owner is a character
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->MaxWalkSpeed = CurrentMoveSpeed;
		}
	}

	OnHealthChanged.Broadcast(CurrentHealth, CurrentMaxHealth);
	OnManaChanged.Broadcast(CurrentMana, CurrentMaxMana);
}

void UCharacterStatComponent::ResetStats()
{
}

void UCharacterStatComponent::UpdateTimedModifiers(float DeltaTime)
{
	bool bNeedsRecalculation = false;

	// Update all timed modifiers
	for (int32 i = StatModifiers.Num() - 1; i >= 0; --i)
	{
		FStatModifier& Modifier = StatModifiers[i];

		// Skip permanent modifiers (Duration < 0)
		if (Modifier.Duration < 0.f)
			continue;

		Modifier.RemainingTime -= DeltaTime;

		// Remove expired modifiers
		if (Modifier.RemainingTime <= 0.f)
		{
			UE_LOG(LogTemp, Log, TEXT("%s modifier expired: %s"),
				*GetOwner()->GetName(), *Modifier.ModifierName.ToString());

			StatModifiers.RemoveAt(i);
			bNeedsRecalculation = true;
		}
	}

	// Recalculate stats if any modifier expired
	if (bNeedsRecalculation)
	{
		RecalculateStats();
	}
}

void UCharacterStatComponent::Die()
{
	OnDeath.Broadcast();
	UE_LOG(LogTemp, Warning, TEXT("%s has died!"), *GetOwner()->GetName());

	// Disable tick to prevent further updates
	SetComponentTickEnabled(false);

	// TODO: Add death logic (ragdoll, destroy actor, etc.)
}

