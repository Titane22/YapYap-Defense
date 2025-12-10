// Copyright Epic Games, Inc. All Rights Reserved.

#include "GamePlay/Characters/Player/YDPlayerController.h"
#include "GamePlay/Characters/Player/YDCharacter.h"
#include "Gameplay/Components/CombatComponent.h"
#include "Gameplay/Components/AbilityComponent.h"
#include "Gameplay/Data/Ability.h"
#include "Gameplay/Data/AbilityTypes.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Camera/CameraComponent.h"
#include "Gameplay/Components/AbilityComponent.h"

AYDPlayerController::AYDPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CachedDestination = FVector::ZeroVector;
	FollowTime = 0.f;
	bIsToDestination = false;
	CurrentTarget = nullptr;

	// Camera settings
	CameraScrollSpeed = 3000.f;
	CameraScrollBorderThickness = 50.f;
	MaxCameraDistanceFromCharacter = 3000.f;
	IndependentCamera = nullptr;
	CameraTargetLocation = FVector::ZeroVector;

	// Skill cooldowns (example values)
	QCooldown = 5.f;
	WCooldown = 8.f;
	ECooldown = 12.f;
	RCooldown = 60.f;

	QLastUsedTime = -1000.f;
	WLastUsedTime = -1000.f;
	ELastUsedTime = -1000.f;
	RLastUsedTime = -1000.f;
}

void AYDPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Add Input Mapping Context
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}

	// Create independent camera actor for LoL-style camera
	if (GetWorld())
	{
		// Spawn a camera actor
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		IndependentCamera = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (IndependentCamera)
		{
			// Add camera component to the actor
			UCameraComponent* CameraComponent = NewObject<UCameraComponent>(IndependentCamera, TEXT("IndependentCamera"));
			CameraComponent->RegisterComponent();
			IndependentCamera->SetRootComponent(CameraComponent);

			// Set camera to view target
			SetViewTarget(IndependentCamera);

			// Position camera above character with LoL-style angle
			CenterCameraOnCharacter();
		}
	}
}

void AYDPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Set up action bindings with Enhanced Input
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Bind right mouse button
		EnhancedInputComponent->BindAction(RightMouseButtonAction, ETriggerEvent::Started, this, &AYDPlayerController::OnRightMouseButtonStarted);
		EnhancedInputComponent->BindAction(RightMouseButtonAction, ETriggerEvent::Completed, this, &AYDPlayerController::OnRightMouseButtonCompleted);

		// Bind skill keys
		EnhancedInputComponent->BindAction(QSkillAction, ETriggerEvent::Triggered, this, &AYDPlayerController::OnQSkillTriggered);
		EnhancedInputComponent->BindAction(WSkillAction, ETriggerEvent::Triggered, this, &AYDPlayerController::OnWSkillTriggered);
		EnhancedInputComponent->BindAction(ESkillAction, ETriggerEvent::Triggered, this, &AYDPlayerController::OnESkillTriggered);
		EnhancedInputComponent->BindAction(RSkillAction, ETriggerEvent::Triggered, this, &AYDPlayerController::OnRSkillTriggered);
	}
}

void AYDPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// Handle camera edge scrolling
	HandleCameraEdgeScrolling(DeltaTime);

	// Movement is now handled by NavMesh AI (SimpleMoveToLocation)
	// No manual AddMovementInput needed
}

void AYDPlayerController::OnRightMouseButtonStarted()
{
	// Perform raycast to find what we clicked on
	FHitResult Hit;
	bool bHitSuccessful = GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);

	if (bHitSuccessful)
	{
		// Check if we clicked on an enemy
		AActor* HitActor = Hit.GetActor();
		if (HitActor && IsEnemy(HitActor))
		{
			// Attack the enemy
			CurrentTarget = HitActor;
			AttackTarget(CurrentTarget);
		}
		else
		{
			// Move to the location
			MoveToMouseCursor();
		}
	}
}

void AYDPlayerController::OnRightMouseButtonCompleted()
{
	FollowTime = 0.f;
}

void AYDPlayerController::MoveToMouseCursor()
{
	FHitResult Hit;
	bool bHitSuccessful = GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);

	if (bHitSuccessful)
	{
		CachedDestination = Hit.Location;
		CurrentTarget = nullptr; // Clear attack target when moving

		// Clear combat target
		APawn* ControlledPawn = GetPawn();
		if (ControlledPawn)
		{
			UCombatComponent* CombatComponent = ControlledPawn->FindComponentByClass<UCombatComponent>();
			if (CombatComponent)
			{
				CombatComponent->ClearTarget();
			}
		}

		// Move the character using NavMesh AI
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, CachedDestination);

		// Spawn click effect
		SpawnClickEffect(CachedDestination);
	}
}

void AYDPlayerController::AttackTarget(AActor* Target)
{
	if (Target == nullptr)
		return;

	// Move towards the target
	UAIBlueprintHelperLibrary::SimpleMoveToActor(this, Target);

	// Get combat component from controlled pawn
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		UCombatComponent* CombatComponent = ControlledPawn->FindComponentByClass<UCombatComponent>();
		if (CombatComponent)
		{
			CombatComponent->SetTarget(Target);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Attacking target: %s"), *Target->GetName());
}

bool AYDPlayerController::IsEnemy(AActor* Actor)
{
	// TODO: Implement proper enemy detection
	// For now, check if actor has "Enemy" tag
	if (Actor)
	{
		return Actor->ActorHasTag(FName("Enemy"));
	}
	return false;
}

void AYDPlayerController::HandleCameraEdgeScrolling(float DeltaTime)
{
	if (!IndependentCamera)
		return;

	// Get mouse position
	float MouseX, MouseY;
	if (GetMousePosition(MouseX, MouseY))
	{
		int32 ViewportSizeX, ViewportSizeY;
		GetViewportSize(ViewportSizeX, ViewportSizeY);

		FVector CameraMovement = FVector::ZeroVector;

		// Check edges
		if (MouseX <= CameraScrollBorderThickness)
			CameraMovement.Y = -1.f;
		else if (MouseX >= ViewportSizeX - CameraScrollBorderThickness)
			CameraMovement.Y = 1.f;

		if (MouseY <= CameraScrollBorderThickness)
			CameraMovement.X = 1.f;
		else if (MouseY >= ViewportSizeY - CameraScrollBorderThickness)
			CameraMovement.X = -1.f;

		// Apply camera movement to independent camera
		if (!CameraMovement.IsZero())
		{
			FVector NewLocation = IndependentCamera->GetActorLocation() + (CameraMovement * CameraScrollSpeed * DeltaTime);

			// Limit camera distance from character
			APawn* ControlledPawn = GetPawn();
			if (ControlledPawn)
			{
				FVector CharacterLocation = ControlledPawn->GetActorLocation();
				FVector ToCamera = NewLocation - CharacterLocation;
				ToCamera.Z = 0; // Ignore vertical distance

				if (ToCamera.Size() > MaxCameraDistanceFromCharacter)
				{
					ToCamera = ToCamera.GetSafeNormal() * MaxCameraDistanceFromCharacter;
					NewLocation = CharacterLocation + ToCamera;
					NewLocation.Z = IndependentCamera->GetActorLocation().Z; // Keep original height
				}
			}

			IndependentCamera->SetActorLocation(NewLocation);
		}
	}
}

void AYDPlayerController::CenterCameraOnCharacter()
{
	if (!IndependentCamera)
		return;

	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		// Position camera above and behind character with LoL-style angle
		FVector CharacterLocation = ControlledPawn->GetActorLocation();
		FVector CameraLocation = CharacterLocation + FVector(-700.f, 0.f, 1400.f); // Offset for top-down view
		FRotator CameraRotation = FRotator(-60.0f, 0.0f, 0.0f); // LoL-style angle

		IndependentCamera->SetActorLocation(CameraLocation);
		IndependentCamera->SetActorRotation(CameraRotation);

		CameraTargetLocation = CameraLocation;
	}
}

void AYDPlayerController::SpawnClickEffect(const FVector& Location)
{
	if (ClickEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ClickEffect, Location, FRotator::ZeroRotator, FVector(1.f), true, true, ENCPoolMethod::None, true);
	}
}

void AYDPlayerController::OnQSkillTriggered()
{
	UE_LOG(LogTemp, Warning, TEXT("=== Q Key Pressed ==="));

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		UE_LOG(LogTemp, Error, TEXT("No controlled pawn"));
		return;
	}

	UAbilityComponent* AbilityComponent = ControlledPawn->FindComponentByClass<UAbilityComponent>();
	if (!AbilityComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("No AbilityComponent found on %s"), *ControlledPawn->GetName());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("AbilityComponent found - Abilities count: %d"), AbilityComponent->Abilities.Num());

	// Use GetAbility() instead of direct array access to avoid crash
	UAbility* QAbility = AbilityComponent->GetAbility(EAbilitySlot::Q);
	if (!QAbility)
	{
		UE_LOG(LogTemp, Error, TEXT("Q Ability not found! You need to:"));
		UE_LOG(LogTemp, Error, TEXT("  1. Create DA_Fireball DataAsset"));
		UE_LOG(LogTemp, Error, TEXT("  2. Call AbilityComponent->LearnAbility(DA_Fireball, EAbilitySlot::Q)"));
		UE_LOG(LogTemp, Error, TEXT("  3. Call QAbility->LevelUp() at least once"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Q Ability found - Level: %d, Cooldown: %.1f"),
		QAbility->CurrentLevel, QAbility->RemainingCooldown);

	if (!QAbility->CanCast())
	{
		UE_LOG(LogTemp, Warning, TEXT("Q Ability cannot be cast:"));
		UE_LOG(LogTemp, Warning, TEXT("  - Level: %d (needs > 0)"), QAbility->CurrentLevel);
		UE_LOG(LogTemp, Warning, TEXT("  - Cooldown: %.1fs"), QAbility->RemainingCooldown);
		UE_LOG(LogTemp, Warning, TEXT("  - IsCasting: %s"), QAbility->bIsCasting ? TEXT("Yes") : TEXT("No"));
		return;
	}

	// PlayerController's ONLY job: Get cursor input data
	// Let TargetingStrategy decide what's valid!

	// Try to hit Pawn first
	FHitResult HitResult;
	bool bHitPawn = GetHitResultUnderCursor(ECC_Pawn, false, HitResult);

	// If no pawn, try visibility (ground/objects)
	if (!bHitPawn || !HitResult.GetActor())
	{
		GetHitResultUnderCursor(ECC_Visibility, false, HitResult);
	}

	// Build TargetData with ALL available information
	FAbilityTargetData TargetData;
	TargetData.bIsValid = HitResult.bBlockingHit;
	TargetData.TargetLocation = HitResult.Location;
	TargetData.Direction = (HitResult.Location - ControlledPawn->GetActorLocation()).GetSafeNormal();

	// Include actor only if it has gameplay-relevant tags
	AActor* HitActor = HitResult.GetActor();
	if (HitActor)
	{
		bool bHasGameplayTag = HitActor->Tags.Contains(FName("Enemy")) ||
		                       HitActor->Tags.Contains(FName("Structure")) ||
		                       HitActor->Tags.Contains(FName("Character")) ||
		                       HitActor->Tags.Contains(FName("Minion")) ||
		                       HitActor->Tags.Contains(FName("Champion")) ||
		                       HitActor->Tags.Contains(FName("Neutral")) ||
		                       HitActor->Tags.Contains(FName("Player"));

		if (bHasGameplayTag)
		{
			TargetData.TargetActor = HitActor;
			UE_LOG(LogTemp, Log, TEXT("Cursor hit valid target: %s at %s"), *HitActor->GetName(), *HitResult.Location.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("Cursor hit background object (ignored): %s"), *HitActor->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Cursor hit location: %s"), *HitResult.Location.ToString());
	}

	UE_LOG(LogTemp, Warning, TEXT(">>> Executing Q Ability <<<"));
	QAbility->Execute(TargetData);
}

void AYDPlayerController::OnWSkillTriggered()
{
	
}

void AYDPlayerController::OnESkillTriggered()
{
	
}

void AYDPlayerController::OnRSkillTriggered()
{
	
}

bool AYDPlayerController::CanUseSkill(float LastUsedTime, float Cooldown)
{
	float CurrentTime = GetWorld()->GetTimeSeconds();
	return (CurrentTime - LastUsedTime) >= Cooldown;
}