// Fill out your copyright notice in the Description page of Project Settings.

#include "Gameplay/Data/DamageEffect.h"
#include "Gameplay/Components/CharacterStatComponent.h"
#include "Gameplay/Data/Ability.h"

void UDamageEffect::Apply(AActor* Target, AActor* Instigator)
{
	if (!Target || !Instigator)
	{
		UE_LOG(LogTemp, Warning, TEXT("DamageEffect::Apply - Invalid Target or Instigator"));
		return;
	}

	// Get target's stat component
	UCharacterStatComponent* TargetStats = Target->FindComponentByClass<UCharacterStatComponent>();
	if (!TargetStats)
	{
		UE_LOG(LogTemp, Warning, TEXT("DamageEffect::Apply - Target %s has no CharacterStatComponent"), *Target->GetName());
		return;
	}

	// Calculate final damage value (BaseValue + AD/AP scaling)
	float FinalDamage = CalculateFinalValue(Instigator);

	// Apply damage
	TargetStats->TakeDamage(FinalDamage, Instigator);

	UE_LOG(LogTemp, Log, TEXT("DamageEffect applied %.1f damage from %s to %s"),
		FinalDamage, *Instigator->GetName(), *Target->GetName());

	// Call base implementation for VFX/SFX
	Super::Apply(Target, Instigator);
}
