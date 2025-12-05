// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AbilityData.generated.h"

/**
 * Enum for ability types
 */
UENUM(BlueprintType)
enum class EAbilityType : uint8
{
	Damage		UMETA(DisplayName = "Damage"),
	Heal		UMETA(DisplayName = "Heal"),
	Buff		UMETA(DisplayName = "Buff"),
	Debuff		UMETA(DisplayName = "Debuff"),
	Utility		UMETA(DisplayName = "Utility"),
	Summon		UMETA(DisplayName = "Summon")
};

/**
 * Enum for ability targeting mode
 * Note: AOE is not a target type - use SpawnedActorClass with AOE_Base instead
 */
UENUM(BlueprintType)
enum class EAbilityTargetType : uint8
{
	Self			UMETA(DisplayName = "Self"),
	SingleTarget	UMETA(DisplayName = "Single Target"),
	GroundTarget	UMETA(DisplayName = "Ground Target"),
	Directional		UMETA(DisplayName = "Directional"),
	None			UMETA(DisplayName = "None")
};

/**
 * Data asset for ability configuration
 */
UCLASS(BlueprintType)
class YD_API UAbilityData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Ability display name */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Basic")
	FText AbilityName;

	/** Ability description */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Basic", meta = (MultiLine = true))
	FText Description;

	/** Ability icon */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Basic")
	UTexture2D* Icon;

	/** Ability type */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Basic")
	EAbilityType AbilityType;

	/** Targeting mode */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Targeting")
	EAbilityTargetType TargetType;

	/** Maximum targeting range */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Targeting", meta = (ClampMin = "0.0"))
	float Range = 1000.0f;

	/** AOE radius (if applicable) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Targeting", meta = (ClampMin = "0.0"))
	float AOERadius = 0.0f;

	/** Cooldown duration in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Stats", meta = (ClampMin = "0.0"))
	float Cooldown = 1.0f;

	/** Mana or resource cost */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Stats", meta = (ClampMin = "0.0"))
	float ManaCost = 0.0f;

	/** Base damage/heal amount */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Stats")
	float BasePower = 0.0f;

	/** Duration of effect (for buffs/debuffs) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Stats", meta = (ClampMin = "0.0"))
	float Duration = 0.0f;

	/** Cast time in seconds */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Animation", meta = (ClampMin = "0.0"))
	float CastTime = 0.0f;

	/** Animation montage to play when using ability */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Animation")
	UAnimMontage* AbilityMontage;

	/** Particle effect to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|VFX")
	UParticleSystem* AbilityEffect;

	/** Niagara effect to spawn (alternative to particle system) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|VFX")
	class UNiagaraSystem* NiagaraEffect;

	/** Sound to play when ability is used */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Audio")
	USoundBase* AbilitySound;

	/** Blueprint class to spawn (for projectiles, summons, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Advanced")
	TSubclassOf<AActor> SpawnedActorClass;

	/** Can this ability be interrupted? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Advanced")
	bool bCanBeInterrupted = true;

	/** Does this ability require a target? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability|Advanced")
	bool bRequiresTarget = false;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("Ability", GetFName());
	}
};
