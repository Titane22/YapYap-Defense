// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Gameplay/Data/AbilityData.h"
#include "Gameplay/Data/AbilityTypes.h"
#include "AbilityComponent.generated.h"

class UCharacter_Data;
class UAbility;
class UAbilityEffect;
enum class EAbilitySlot : uint8;
struct FAbilityTargetData;

/**
 * Component for managing character abilities (QWER)
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class YD_API UAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAbilityComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	void InitializeAbilities();
	
public:
	// ============ Data Assets (에디터에서 할당) ============
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TArray<UAbilityData*> AbilityDataAssets;

	// ============ Runtime Instances ============
	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	TArray<UAbility*> Abilities;

	UPROPERTY()
	TMap<FGameplayTag, UAbilityEffect*> ActiveBuffs;

	// ============ Global State ============
	UPROPERTY(Replicated)
	bool bIsSilenced = false;

	UPROPERTY(Replicated)
	bool bIsStunned = false;

	UPROPERTY(Replicated)
	bool bIsRooted = false;

	// ============ Public Interface ============
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	UAbility* GetAbility(EAbilitySlot Slot) const;

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void LearnAbility(UAbilityData* AbilityData, EAbilitySlot Slot);

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void ExecuteAbility(EAbilitySlot Slot, const FAbilityTargetData& TargetData);

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
