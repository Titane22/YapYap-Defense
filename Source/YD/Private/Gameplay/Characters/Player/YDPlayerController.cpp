// Copyright Epic Games, Inc. All Rights Reserved.

#include "GamePlay/Characters/Player/YDPlayerController.h"
#include "GamePlay/Characters/Player/YDCharacter.h"
#include "Gameplay/Components/CombatComponent.h"
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
	if (CanUseSkill(QLastUsedTime, QCooldown))
	{
		QLastUsedTime = GetWorld()->GetTimeSeconds();
		UE_LOG(LogTemp, Log, TEXT("Q Skill Used"));

		// TODO: Implement Q skill logic
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Q Skill on cooldown"));
	}
}

void AYDPlayerController::OnWSkillTriggered()
{
	if (CanUseSkill(WLastUsedTime, WCooldown))
	{
		WLastUsedTime = GetWorld()->GetTimeSeconds();
		UE_LOG(LogTemp, Log, TEXT("W Skill Used"));

		// TODO: Implement W skill logic
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("W Skill on cooldown"));
	}
}

void AYDPlayerController::OnESkillTriggered()
{
	if (CanUseSkill(ELastUsedTime, ECooldown))
	{
		ELastUsedTime = GetWorld()->GetTimeSeconds();
		UE_LOG(LogTemp, Log, TEXT("E Skill Used"));

		// TODO: Implement E skill logic
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("E Skill on cooldown"));
	}
}

void AYDPlayerController::OnRSkillTriggered()
{
	if (CanUseSkill(RLastUsedTime, RCooldown))
	{
		RLastUsedTime = GetWorld()->GetTimeSeconds();
		UE_LOG(LogTemp, Log, TEXT("R Skill Used"));

		// TODO: Implement R skill logic
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("R Skill on cooldown"));
	}
}

bool AYDPlayerController::CanUseSkill(float LastUsedTime, float Cooldown)
{
	float CurrentTime = GetWorld()->GetTimeSeconds();
	return (CurrentTime - LastUsedTime) >= Cooldown;
}