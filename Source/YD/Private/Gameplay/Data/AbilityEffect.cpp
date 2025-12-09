// Fill out your copyright notice in the Description page of Project Settings.

#include "Gameplay/Data/AbilityEffect.h"

#include "Gameplay/Components/CharacterStatComponent.h"
#include "Gameplay/Data/Ability.h"

void UAbilityEffect::InitializeFromData(const FAbilityEffectData& InData, UAbility* Ability)
{
	EffectData = InData;
	SourceAbility = Ability;
	Duration = InData.Duration;
	RemainingDuration = Duration;
}

void UAbilityEffect::Apply(AActor* Target, AActor* Instigator)
{
	// Base implementation - override in subclasses
}

void UAbilityEffect::OnApplied()
{
	// Base implementation - override in subclasses
}

void UAbilityEffect::OnRemoved()
{
	// Base implementation - override in subclasses
}

void UAbilityEffect::UpdateDuration(float DeltaTime)
{
	if (Duration > 0.0f)
	{
		RemainingDuration -= DeltaTime;
	}
}

bool UAbilityEffect::IsExpired() const
{
	return Duration > 0.0f && RemainingDuration <= 0.0f;
}

float UAbilityEffect::CalculateFinalValue(AActor* Instigator) const
{
	if (!Instigator)
		return EffectData.BaseValue;

	float FinalValue = EffectData.BaseValue;

	// Apply AD/AP scaling from character stats
	if (UCharacterStatComponent* Stats = Instigator->FindComponentByClass<UCharacterStatComponent>())
	{
		FinalValue += Stats->GetCurrentAttackDamage() * EffectData.ADScaling;
		FinalValue += Stats->GetCurrentAbilityPower() * EffectData.APScaling;
	}

	return FinalValue;
}
