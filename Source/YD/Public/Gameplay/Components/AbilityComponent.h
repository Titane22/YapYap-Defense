// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Gameplay/Data/AbilityData.h"
#include "AbilityComponent.generated.h"

class UCharacter_Data;

/** Enum for ability slots */
UENUM(BlueprintType)
enum class EAbilitySlot : uint8
{
	Q UMETA(DisplayName = "Q Ability"),
	W UMETA(DisplayName = "W Ability"),
	E UMETA(DisplayName = "E Ability"),
	R UMETA(DisplayName = "R Ability")
};

/** Delegate for ability events */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAbilityUsed, EAbilitySlot, Slot, UAbilityData*, AbilityData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAbilityCooldownStarted, EAbilitySlot, Slot, float, Cooldown);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityCooldownFinished, EAbilitySlot, Slot);

/**
 * Component for managing character abilities (QWER)
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class YD_API UAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAbilityComponent();

protected:
	virtual void BeginPlay() override;

	// ========== Ability Base Logic ==========
	
	/** Execute the actual ability logic */
	void ExecuteAbility(EAbilitySlot Slot, UAbilityData* AbilityData, AActor* Target);

	/** Check if ability can be used */
	bool CanUseAbility(EAbilitySlot Slot, UAbilityData* AbilityData) const;

	/** Start cooldown for ability */
	void StartCooldown(EAbilitySlot Slot, float Duration);

	/** Update all cooldowns */
	void UpdateCooldowns(float DeltaTime);

	// ========== Ability Perform Func ==========

	bool ResolveTargetData(UAbilityData* AbilityData, AActor* Target, FVector& OutTargetLocation, FVector& OutLookDirection);

	/** Helper: Check if target is within ability range */
	bool IsTargetInRange(const FVector& TargetLocation, float Range) const;

	/** Helper: Play common ability visuals (animation, VFX, sound) */
	void PlayAbilityVisuals(UAbilityData* AbilityData, const FRotator& SpawnRotation);

	/** Perform ability by target type */
	void PerformSelfAbility(UAbilityData* AbilityData);

	void PerformSingleAbility(UAbilityData* AbilityData, AActor* Target);

	void PerformGorundAbility(UAbilityData* AbilityData, FVector TargetLocation);

	void PerformDirecitonalAbility(UAbilityData* AbilityData, FVector TargetDirection);

	// ========== Ability Type Apply Func ==========
	void ApplyDamage(AActor* Target, float Damage);

	void ApplyHeal(AActor* Target, float HealAmount);

	void ApplyBuff(AActor* Target, UAbilityData* AbilityData);

	void ApplyDebuff(AActor* Target, UAbilityData* AbilityData);
	
public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ========== Ability Setup ==========

	/** Initialize abilities from character data */
	UFUNCTION(BlueprintCallable, Category = "Ability")
	void InitializeAbilities(UCharacter_Data* CharacterData);

	/** Set ability for a specific slot */
	UFUNCTION(BlueprintCallable, Category = "Ability")
	void SetAbility(EAbilitySlot Slot, UAbilityData* AbilityData);

	/** Get ability for a specific slot */
	UFUNCTION(BlueprintPure, Category = "Ability")
	UAbilityData* GetAbility(EAbilitySlot Slot) const;

	// ========== Ability Execution ==========

	/** Try to use ability in specified slot */
	UFUNCTION(BlueprintCallable, Category = "Ability")
	bool TryUseAbility(EAbilitySlot Slot, AActor* Target = nullptr);

	/** Use Q ability */
	UFUNCTION(BlueprintCallable, Category = "Ability")
	bool UseQAbility(AActor* Target = nullptr);

	/** Use W ability */
	UFUNCTION(BlueprintCallable, Category = "Ability")
	bool UseWAbility(AActor* Target = nullptr);

	/** Use E ability */
	UFUNCTION(BlueprintCallable, Category = "Ability")
	bool UseEAbility(AActor* Target = nullptr);

	/** Use R ability */
	UFUNCTION(BlueprintCallable, Category = "Ability")
	bool UseRAbility(AActor* Target = nullptr);

	// ========== Cooldown Management ==========

	/** Check if ability is on cooldown */
	UFUNCTION(BlueprintPure, Category = "Ability")
	bool IsAbilityOnCooldown(EAbilitySlot Slot) const;

	/** Get remaining cooldown time for ability */
	UFUNCTION(BlueprintPure, Category = "Ability")
	float GetAbilityCooldownRemaining(EAbilitySlot Slot) const;

	/** Get cooldown percentage (0.0 = ready, 1.0 = just used) */
	UFUNCTION(BlueprintPure, Category = "Ability")
	float GetAbilityCooldownPercent(EAbilitySlot Slot) const;

	// ========== Events ==========

	/** Called when an ability is used */
	UPROPERTY(BlueprintAssignable, Category = "Ability")
	FOnAbilityUsed OnAbilityUsed;

	/** Called when ability cooldown starts */
	UPROPERTY(BlueprintAssignable, Category = "Ability")
	FOnAbilityCooldownStarted OnAbilityCooldownStarted;

	/** Called when ability cooldown finishes */
	UPROPERTY(BlueprintAssignable, Category = "Ability")
	FOnAbilityCooldownFinished OnAbilityCooldownFinished;

protected:
	/** Abilities mapped to QWER slots */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability")
	TMap<EAbilitySlot, UAbilityData*> Abilities;

	/** Cooldown timers for each ability */
	UPROPERTY()
	TMap<EAbilitySlot, float> AbilityCooldowns;

};
