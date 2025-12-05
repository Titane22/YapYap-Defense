// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "YDCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UCharacterStatComponent;
class UCombatComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class AYDCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AYDCharacter();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	UCharacterStatComponent* CharacterStatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UCombatComponent* CombatComponent;

protected:
	// APawn interface
	virtual void BeginPlay();

};

