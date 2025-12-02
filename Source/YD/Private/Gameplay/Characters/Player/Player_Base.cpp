// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/Characters/Player/Player_Base.h"
#include "Gameplay/Components/InventoryComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

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
}
