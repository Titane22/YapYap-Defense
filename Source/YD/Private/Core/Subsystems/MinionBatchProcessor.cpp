// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/Subsystems/MinionBatchProcessor.h"
#include "Gameplay/Characters/Enemy/Enemy_Base.h"
#include "Gameplay/Components/CharacterStatComponent.h"
#include "Gameplay/Components/CombatComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"

void UMinionBatchProcessor::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	TargetUpdateTimer = 0.f;
	TargetUpdateInterval = 0.5f; // Update targets every 0.5 seconds

	UE_LOG(LogTemp, Log, TEXT("MinionBatchProcessor: Initialized"));
}

void UMinionBatchProcessor::Deinitialize()
{
	RegisteredMinions.Empty();
	Super::Deinitialize();
}

void UMinionBatchProcessor::Tick(float DeltaTime)
{
	if (RegisteredMinions.Num() == 0)
		return;

	// Update target search timer
	TargetUpdateTimer += DeltaTime;
	if (TargetUpdateTimer >= TargetUpdateInterval)
	{
		TargetUpdateTimer = 0.f;
		BatchUpdateTargets();
	}
}

void UMinionBatchProcessor::RegisterMinion(AEnemy_Base* Minion)
{
	if (!Minion || RegisteredMinions.Contains(Minion))
		return;

	RegisteredMinions.Add(Minion);
	UE_LOG(LogTemp, Log, TEXT("MinionBatchProcessor: Registered minion. Total: %d"), RegisteredMinions.Num());
}

void UMinionBatchProcessor::UnregisterMinion(AEnemy_Base* Minion)
{
	if (!Minion)
		return;

	RegisteredMinions.Remove(Minion);
	UE_LOG(LogTemp, Log, TEXT("MinionBatchProcessor: Unregistered minion. Total: %d"), RegisteredMinions.Num());
}

void UMinionBatchProcessor::BatchUpdateTargets()
{
	if (RegisteredMinions.Num() == 0)
		return;

	UWorld* World = GetWorld();
	if (!World)
		return;

	// Single sphere overlap for all minions
	// Use a large sphere that encompasses all minion detection ranges
	FVector CenterPoint = FVector::ZeroVector;
	int32 ValidMinions = 0;

	// Calculate center of all minions
	for (AEnemy_Base* Minion : RegisteredMinions)
	{
		if (Minion && IsValid(Minion))
		{
			CenterPoint += Minion->GetActorLocation();
			ValidMinions++;
		}
	}

	if (ValidMinions == 0)
		return;

	CenterPoint /= ValidMinions;

	// Perform large overlap to find all potential targets
	TArray<FOverlapResult> OverlapResults;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(5000.f); // Large radius
	FCollisionQueryParams QueryParams;
	for (AEnemy_Base* Minion : RegisteredMinions)
	{
		QueryParams.AddIgnoredActor(Minion);
	}

	World->OverlapMultiByChannel(
		OverlapResults,
		CenterPoint,
		FQuat::Identity,
		ECC_Pawn,
		Sphere,
		QueryParams
	);

	// Assign closest enemy to each minion
	for (AEnemy_Base* Minion : RegisteredMinions)
	{
		if (!Minion || !IsValid(Minion))
			continue;

		UCombatComponent* Combat = Minion->FindComponentByClass<UCombatComponent>();
		if (!Combat)
			continue;

		// Skip if already has a valid target
		if (Combat->GetTarget() && IsValid(Combat->GetTarget()))
		{
			UCharacterStatComponent* TargetStats = Combat->GetTarget()->FindComponentByClass<UCharacterStatComponent>();
			if (TargetStats && TargetStats->IsAlive())
				continue; // Keep current target
		}

		// Find new closest enemy
		AActor* ClosestEnemy = FindClosestEnemy(Minion, OverlapResults);
		if (ClosestEnemy && Combat)
		{
			Combat->SetTarget(ClosestEnemy);

			// Move towards target
			if (AController* Controller = Minion->GetController())
			{
				UAIBlueprintHelperLibrary::SimpleMoveToActor(Controller, ClosestEnemy);
			}
		}
	}
}

AActor* UMinionBatchProcessor::FindClosestEnemy(AEnemy_Base* Minion, const TArray<FOverlapResult>& PotentialTargets) const
{
	if (!Minion)
		return nullptr;

	float DetectionRange = 800.f; // Same as Enemy_Base
	AActor* ClosestEnemy = nullptr;
	float ClosestDistance = DetectionRange;

	FVector MinionLocation = Minion->GetActorLocation();

	for (const FOverlapResult& Result : PotentialTargets)
	{
		AActor* Target = Result.GetActor();
		if (!Target || Target == Minion)
			continue;

		// Check if enemy
		if (!Minion->IsEnemy(Target))
			continue;

		// Check if alive
		UCharacterStatComponent* TargetStats = Target->FindComponentByClass<UCharacterStatComponent>();
		if (!TargetStats || !TargetStats->IsAlive())
			continue;

		// Check distance
		float Distance = FVector::Dist(MinionLocation, Target->GetActorLocation());
		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosestEnemy = Target;
		}
	}

	return ClosestEnemy;
}

