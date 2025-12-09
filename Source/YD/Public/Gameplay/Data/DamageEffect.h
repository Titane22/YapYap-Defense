// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilityEffect.h"
#include "DamageEffect.generated.h"

/**
 * Damage effect that applies damage to target's CharacterStatComponent
 */
UCLASS()
class YD_API UDamageEffect : public UAbilityEffect
{
	GENERATED_BODY()

public:
	virtual void Apply(AActor* Target, AActor* Instigator) override;
};
