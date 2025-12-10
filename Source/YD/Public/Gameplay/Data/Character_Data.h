// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Character_Data.generated.h"

class UAbilityData;

/**
 * Data asset for character configuration including abilities
 */
UCLASS(BlueprintType)
class YD_API UCharacter_Data : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Character display name */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Basic")
	FText CharacterName;

	/** Character description */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Basic", meta = (MultiLine = true))
	FText Description;

	/** Character portrait */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Basic")
	UTexture2D* Portrait;

	// ========== Visual Assets ==========

	/** Character skeletal mesh */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Visual")
	USkeletalMesh* SkeletalMesh;

	/** Animation Blueprint class */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Visual")
	TSubclassOf<UAnimInstance> AnimationBlueprint;

	/** Default materials for the mesh (optional override) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Visual")
	TArray<UMaterialInterface*> Materials;

	/** Mesh scale */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Visual")
	FVector MeshScale = FVector(1.0f, 1.0f, 1.0f);

	/** Mesh relative location offset */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Visual")
	FVector MeshLocationOffset = FVector(0.0f, 0.0f, -90.0f);

	/** Mesh relative rotation offset */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Visual")
	FRotator MeshRotationOffset = FRotator(0.0f, -90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Visual")
	UAnimMontage* AttackMontage;
		
	// ========== QWER Abilities ==========

	/** Q Ability */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Abilities")
	UAbilityData* QAbility;

	/** W Ability */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Abilities")
	UAbilityData* WAbility;

	/** E Ability */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Abilities")
	UAbilityData* EAbility;

	/** R Ability (Ultimate) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Abilities")
	UAbilityData* RAbility;

	// ========== Base Stats ==========

	/** Base health */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Stats", meta = (ClampMin = "1.0"))
	float BaseHealth = 100.0f;

	/** Base mana */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Stats", meta = (ClampMin = "0.0"))
	float BaseMana = 100.0f;

	/** Base attack damage */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Stats", meta = (ClampMin = "0.0"))
	float BaseAttackDamage = 10.0f;

	/** Base attack speed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Stats", meta = (ClampMin = "0.1"))
	float BaseAttackSpeed = 1.0f;

	/** Base movement speed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Stats", meta = (ClampMin = "0.0"))
	float BaseMovementSpeed = 600.0f;

	/** Base armor/defense */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Stats", meta = (ClampMin = "0.0"))
	float BaseArmor = 0.0f;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("Character", GetFName());
	}
};
