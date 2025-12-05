// Copyright Epic Games, Inc. All Rights Reserved.

#include "GamePlay/Characters/Player/YDCharacter.h"
#include "Gameplay/Components/CharacterStatComponent.h"
#include "Gameplay/Components/CombatComponent.h"
#include "Engine/LocalPlayer.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Animation/AnimInstance.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AYDCharacter

AYDCharacter::AYDCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement - LOL style
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f); // Fast rotation for responsive feel
	GetCharacterMovement()->MaxWalkSpeed = 350.f; // LoL-style movement speed
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2048.f;

	// Create stat component
	CharacterStatComponent = CreateDefaultSubobject<UCharacterStatComponent>(TEXT("CharacterStatComponent"));

	// Create combat component
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character)
	// can now be set via CharacterData asset for a fully data-driven approach
}

void AYDCharacter::BeginPlay()
{
	// Call the base class
	Super::BeginPlay();

}
