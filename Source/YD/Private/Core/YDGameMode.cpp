// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/YDGameMode.h"
#include "GamePlay/Characters/Player/YDCharacter.h"
#include "GamePlay/Characters/Player/YDPlayerController.h"
#include "Core/Subsystems/MinionBatchProcessor.h"
#include "Core/Subsystems/MinionPoolManager.h"
#include "Gameplay/Characters/Enemy/Enemy_Base.h"
#include "Gameplay/Objects/SpawnPortal.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AYDGameMode::AYDGameMode()
{
	// Set player controller to C++ class
	PlayerControllerClass = AYDPlayerController::StaticClass();

	// Default values
	MinionsPerPortal = 3;
	InitialPoolSize = 20;

	// DefaultPawnClass is set in Blueprint or World Settings
	// This allows using BP_YDCharacter with mesh setup
}

void AYDGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Initialize Minion Pool
	MinionPool = GetWorld()->GetSubsystem<UMinionPoolManager>();
	if (MinionPool && MinionClass)
	{
		MinionPool->InitializePool(MinionClass, InitialPoolSize);
		UE_LOG(LogTemp, Log, TEXT("GameMode: Initialized MinionPool with %d minions"), InitialPoolSize);
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, TEXT("MinionPool initialized!"));
	}
	else if (!MinionClass)
	{
		UE_LOG(LogTemp, Error, TEXT("GameMode: MinionClass not set! Set it in Blueprint."));
	}

	// Initialize Batch Processor
	BatchProcessor = GetWorld()->GetSubsystem<UMinionBatchProcessor>();

	SpawnMinionWave();
}

void AYDGameMode::SpawnMinionWave()
{
	if (!MinionPool)
	{
		UE_LOG(LogTemp, Error, TEXT("GameMode: MinionPool is null!"));
		return;
	}

	// Find all active spawn portals
	TArray<ASpawnPortal*> ActivePortals = GetActiveSpawnPortals();

	if (ActivePortals.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("GameMode: No active spawn portals found!"));
		return;
	}

	// Spawn minions at each portal
	int32 TotalSpawned = 0;
	for (ASpawnPortal* Portal : ActivePortals)
	{
		if (!Portal)
			continue;

		FRotator SpawnRotation = Portal->GetActorRotation();

		// Get grid spawn positions from portal (5 columns per row)
		TArray<FVector> GridPositions = Portal->GetGridSpawnPositions(MinionsPerPortal, 5);

		UE_LOG(LogTemp, Log, TEXT("GameMode: Spawning %d minions at portal: %s (Grid: 5x%d)"),
			MinionsPerPortal, *Portal->GetName(), FMath::CeilToInt((float)MinionsPerPortal / 5));

		// Spawn minions at grid positions
		for (int32 i = 0; i < GridPositions.Num(); i++)
		{
			FVector SpawnLocation = GridPositions[i];
			AEnemy_Base* Minion = MinionPool->GetMinion(SpawnLocation, SpawnRotation);

			if (Minion)
			{
				TotalSpawned++;

				// Register with batch processor
				if (BatchProcessor)
				{
					BatchProcessor->RegisterMinion(Minion);
				}

				// Verify spawned location
				FVector ActualLocation = Minion->GetActorLocation();
				bool bHasController = Minion->GetController() != nullptr;
				UE_LOG(LogTemp, Log, TEXT("  Minion %d: Pos %s, Controller: %s, Hidden: %s"),
					i, *ActualLocation.ToString(),
					bHasController ? TEXT("YES") : TEXT("NO"),
					Minion->IsHidden() ? TEXT("YES") : TEXT("NO"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("GameMode: Failed to get minion from pool!"));
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("GameMode: Spawned %d minions at %d portals."),
		TotalSpawned, ActivePortals.Num());
}

TArray<ASpawnPortal*> AYDGameMode::GetActiveSpawnPortals() const
{
	TArray<ASpawnPortal*> ActivePortals;

	// Find all SpawnPortal actors in the level
	TArray<AActor*> FoundPortals;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnPortal::StaticClass(), FoundPortals);

	// Filter only active portals
	for (AActor* Actor : FoundPortals)
	{
		ASpawnPortal* Portal = Cast<ASpawnPortal>(Actor);
		if (Portal && Portal->GetEnabled())
		{
			ActivePortals.Add(Portal);
		}
	}

	return ActivePortals;
}
