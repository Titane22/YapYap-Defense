// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Characters/Player/YDCharacter.h"
#include "Player_Base.generated.h"

class UInventoryComponent;
class UAbilityComponent;
class UCharacter_Data;

/**
 * 
 */
UCLASS()
class YD_API APlayer_Base : public AYDCharacter
{
	GENERATED_BODY()

protected:
	/** Camera boom positioning the camera above the character (top-down view) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UInventoryComponent* InventoryComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	UAbilityComponent* AbilityComponent;
	
	/** Current character data asset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
	UCharacter_Data* CurrentCharacterData;

	UPROPERTY()
	UAnimMontage* AttackMontage;
	
public:
	APlayer_Base();

	/** Initialize character from data asset */
	UFUNCTION(BlueprintCallable, Category = "Character")
	virtual void InitializeFromCharacterData(UCharacter_Data* CharacterData);

	virtual void BeginPlay();
	
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	
	/** Get the current character data */
	UFUNCTION(BlueprintPure, Category = "Character")
	UCharacter_Data* GetCharacterData() const { return CurrentCharacterData; }

	/** Get ability component */
	UFUNCTION(BlueprintPure, Category = "Character")
	UAbilityComponent* GetAbilityComponent() const { return AbilityComponent; }

	/** Apply visual settings from character data */
	void ApplyVisualSettings(UCharacter_Data* CharacterData);

	/** Apply stats from character data */
	void ApplyStats(UCharacter_Data* CharacterData);

protected:
	void Attack(AActor* Target);
};
