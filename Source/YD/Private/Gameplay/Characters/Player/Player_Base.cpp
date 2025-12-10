// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/Characters/Player/Player_Base.h"
#include "Gameplay/Components/InventoryComponent.h"
#include "Gameplay/Components/CharacterStatComponent.h"
#include "Gameplay/Components/AbilityComponent.h"
#include "Gameplay/Data/Character_Data.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Gameplay/Components/CombatComponent.h"

APlayer_Base::APlayer_Base()
{
	// Create a camera boom for top-down view
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1400.0f; // Distance for top-down view
	CameraBoom->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f)); // Angle for LoL-style camera
	CameraBoom->bDoCollisionTest = false; // Don't want camera to pull in
	CameraBoom->bUsePawnControlRotation = false; // Fixed camera angle
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate

	// Create inventory component
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	
	// Create ability component
	AbilityComponent = CreateDefaultSubobject<UAbilityComponent>(TEXT("AbilityComponent"));

	// Add "Player" tag for team identification
	Tags.Add(FName("Player"));
}

void APlayer_Base::InitializeFromCharacterData(UCharacter_Data* CharacterData)
{
	if (!CharacterData)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("InitializeFromCharacterData: CharacterData is null"));
		return;
	}

	CurrentCharacterData = CharacterData;

	// Apply visual settings
	ApplyVisualSettings(CharacterData);

	// Apply stats
	ApplyStats(CharacterData);

	// Note: Abilities are automatically initialized in AbilityComponent::BeginPlay()
	// AbilityDataAssets should be set in the component's defaults

	UE_LOG(LogTemplateCharacter, Log, TEXT("Character initialized from data: %s"), *CharacterData->CharacterName.ToString());
}

void APlayer_Base::BeginPlay()
{
	Super::BeginPlay();
	
	// Initialize from character data if set
	if (CurrentCharacterData)
	{
		InitializeFromCharacterData(CurrentCharacterData);
	}

	if (CombatComponent)
	{
		CombatComponent->OnAttackStarted.AddDynamic(this, &APlayer_Base::Attack);
	}
}

void APlayer_Base::ApplyVisualSettings(UCharacter_Data* CharacterData)
{
	if (!CharacterData)
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("ApplyVisualSettings: Mesh component is null"));
		return;
	}

	// Set skeletal mesh
	if (CharacterData->SkeletalMesh)
	{
		MeshComp->SetSkeletalMesh(CharacterData->SkeletalMesh);
		UE_LOG(LogTemplateCharacter, Log, TEXT("Applied skeletal mesh: %s"), *CharacterData->SkeletalMesh->GetName());
	}

	// Set animation blueprint
	if (CharacterData->AnimationBlueprint)
	{
		MeshComp->SetAnimInstanceClass(CharacterData->AnimationBlueprint);
		UE_LOG(LogTemplateCharacter, Log, TEXT("Applied animation blueprint: %s"), *CharacterData->AnimationBlueprint->GetName());
	}

	// Apply materials
	if (CharacterData->Materials.Num() > 0)
	{
		for (int32 i = 0; i < CharacterData->Materials.Num(); i++)
		{
			if (CharacterData->Materials[i])
			{
				MeshComp->SetMaterial(i, CharacterData->Materials[i]);
			}
		}
		UE_LOG(LogTemplateCharacter, Log, TEXT("Applied %d materials"), CharacterData->Materials.Num());
	}

	if (CharacterData->AttackMontage)
	{
		AttackMontage = CharacterData->AttackMontage;
	}
	
	// Apply transform settings
	MeshComp->SetRelativeScale3D(CharacterData->MeshScale);
	MeshComp->SetRelativeLocation(CharacterData->MeshLocationOffset);
	MeshComp->SetRelativeRotation(CharacterData->MeshRotationOffset);
}

void APlayer_Base::ApplyStats(UCharacter_Data* CharacterData)
{
	if (!CharacterData)
	{
		return;
	}

	// Apply movement speed
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->MaxWalkSpeed = CharacterData->BaseMovementSpeed;
		UE_LOG(LogTemplateCharacter, Log, TEXT("Set movement speed to %.1f"), CharacterData->BaseMovementSpeed);
	}

	// Apply stats to StatComponent
	if (CharacterStatComponent)
	{
		CharacterStatComponent->BaseMaxHealth = CharacterData->BaseHealth;
		CharacterStatComponent->BaseMaxMana = CharacterData->BaseMana;
		CharacterStatComponent->BaseAttackDamage = CharacterData->BaseAttackDamage;
		CharacterStatComponent->BaseAttackSpeed = CharacterData->BaseAttackSpeed;
		CharacterStatComponent->BaseArmor = CharacterData->BaseArmor;
		CharacterStatComponent->BaseMoveSpeed = CharacterData->BaseMovementSpeed;

		// Recalculate all stats
		CharacterStatComponent->RecalculateStats();

		// Initialize to full health after applying new stats
		CharacterStatComponent->CurrentHealth = CharacterStatComponent->CurrentMaxHealth;
		CharacterStatComponent->CurrentMana = CharacterStatComponent->CurrentMaxMana;

		UE_LOG(LogTemplateCharacter, Log, TEXT("Applied stats - Health: %.1f/%.1f, Damage: %.1f, Speed: %.1f"),
			CharacterStatComponent->CurrentHealth, CharacterStatComponent->CurrentMaxHealth,
			CharacterData->BaseAttackDamage, CharacterData->BaseAttackSpeed);
	}
}

void APlayer_Base::Attack(AActor* Target)
{
	// Play attack montage if set
	if (AttackMontage)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			// Stop current montage and play new one for responsive attacks
			if (AnimInstance->IsAnyMontagePlaying())
			{
				AnimInstance->Montage_Stop(0.2f);
			}
			AnimInstance->Montage_Play(AttackMontage, 1.0f);
			UE_LOG(LogTemp, Log, TEXT("%s playing attack montage on %s"), *GetName(), Target ? *Target->GetName() : TEXT("None"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("%s has no AnimInstance!"), *GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s has no AttackMontage set!"), *GetName());
	}
}
