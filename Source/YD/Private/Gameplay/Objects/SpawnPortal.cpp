// Fill out your copyright notice in the Description page of Project Settings.

#include "Gameplay/Objects/SpawnPortal.h"
#include "Components/BoxComponent.h"

// Sets default values
ASpawnPortal::ASpawnPortal()
{
	// SpawnPortal doesn't need to tick
	PrimaryActorTick.bCanEverTick = false;

	// Create root component
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;

	// Create spawn box
	SpawnBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnBox"));
	SpawnBox->SetupAttachment(RootScene);
	SpawnBox->SetBoxExtent(FVector(500.f, 500.f, 100.f)); // 10m x 10m x 2m box
	SpawnBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnBox->SetHiddenInGame(true); // Visible in editor, hidden in game
}

// Called when the game starts or when spawned
void ASpawnPortal::BeginPlay()
{
	Super::BeginPlay();
}

TArray<FVector> ASpawnPortal::GetGridSpawnPositions(int32 Count, int32 ColumnsPerRow) const
{
	TArray<FVector> Positions;

	if (Count <= 0 || !SpawnBox)
		return Positions;

	// Get box bounds in world space
	FVector BoxExtent = SpawnBox->GetScaledBoxExtent();
	FVector BoxCenter = SpawnBox->GetComponentLocation();
	FRotator BoxRotation = SpawnBox->GetComponentRotation();

	// Calculate grid dimensions
	int32 Rows = FMath::CeilToInt((float)Count / ColumnsPerRow);

	// Calculate spacing
	float SpacingX = (BoxExtent.X * 2.0f) / FMath::Max(1, ColumnsPerRow - 1);
	float SpacingY = (BoxExtent.Y * 2.0f) / FMath::Max(1, Rows - 1);

	// Generate grid positions
	for (int32 i = 0; i < Count; i++)
	{
		int32 Row = i / ColumnsPerRow;
		int32 Col = i % ColumnsPerRow;

		// Calculate local position (relative to box center)
		FVector LocalPos;
		LocalPos.X = -BoxExtent.X + (Col * SpacingX);
		LocalPos.Y = -BoxExtent.Y + (Row * SpacingY);
		LocalPos.Z = 0.f;

		// Transform to world space
		FVector WorldPos = BoxCenter + BoxRotation.RotateVector(LocalPos);
		Positions.Add(WorldPos);
	}

	return Positions;
}

