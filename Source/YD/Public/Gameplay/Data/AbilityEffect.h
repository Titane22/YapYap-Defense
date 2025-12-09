// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AbilityTypes.h"
#include "AbilityEffect.generated.h"

class UAbility;
/**
 * 
 */
UCLASS()
class YD_API UAbilityEffect : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	FAbilityEffectData EffectData;
    
	UPROPERTY()
	UAbility* SourceAbility;
    
	UPROPERTY()
	float Duration = 0.0f;
    
	UPROPERTY()
	float RemainingDuration = 0.0f;
    
	void InitializeFromData(const FAbilityEffectData& InData, UAbility* Ability);
	virtual void Apply(AActor* Target, AActor* Instigator);
	virtual void OnApplied();
	virtual void OnRemoved();
	virtual void UpdateDuration(float DeltaTime);
	bool IsExpired() const;
    
protected:
	float CalculateFinalValue(AActor* Instigator) const;
};
