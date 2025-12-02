// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/Subsystems/MinionPoolManager.h"
#include "Gameplay/Characters/Enemy/Enemy_Base.h"
#include "Gameplay/Components/CharacterStatComponent.h"
#include "Gameplay/Components/CombatComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

void UMinionPoolManager::InitializePool(TSubclassOf<AEnemy_Base> MinionClass, int32 InitialPoolSize)
{
	if (!MinionClass)
	{
		UE_LOG(LogTemp, Error, TEXT("MinionPoolManager: MinionClass is null! Cannot initialize pool."));
		return;
	}

	// Validate that MinionClass is actually a valid Actor class
	if (!MinionClass->IsChildOf(AActor::StaticClass()))
	{
		UE_LOG(LogTemp, Error, TEXT("MinionPoolManager: MinionClass is not an Actor! Class: %s"),
			*MinionClass->GetName());
		return;
	}

	MinionClassToSpawn = MinionClass;

	UE_LOG(LogTemp, Log, TEXT("MinionPoolManager: Initializing pool with class: %s, count: %d"),
		*MinionClass->GetName(), InitialPoolSize);

	// Pre-spawn minions for the pool
	for (int32 i = 0; i < InitialPoolSize; i++)
	{
		AEnemy_Base* Minion = SpawnNewMinion();
		if (Minion)
		{
			DeactivateMinion(Minion);
			InactiveMinions.Add(Minion);
			UE_LOG(LogTemp, Log, TEXT("  Pooled minion %d: %s"), i, *Minion->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("  Failed to spawn minion %d!"), i);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("MinionPoolManager: Initialized pool with %d minions"), InactiveMinions.Num());
}

AEnemy_Base* UMinionPoolManager::GetMinion(const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
	AEnemy_Base* Minion = nullptr;

	// Try to reuse from pool
	if (InactiveMinions.Num() > 0)
	{
		Minion = InactiveMinions.Pop();
		ActivateMinion(Minion, SpawnLocation, SpawnRotation);
	}
	else
	{
		// Pool is empty, spawn new minion
		Minion = SpawnNewMinion();
		if (Minion)
		{
			Minion->SetActorLocation(SpawnLocation);
			Minion->SetActorRotation(SpawnRotation);
		}
	}

	if (Minion)
	{
		ActiveMinions.Add(Minion);
	}

	return Minion;
}

void UMinionPoolManager::ReturnMinion(AEnemy_Base* Minion)
{
	if (!Minion)
		return;

	// Remove from active list
	ActiveMinions.Remove(Minion);

	// Deactivate and add to inactive pool
	DeactivateMinion(Minion);
	InactiveMinions.Add(Minion);

	UE_LOG(LogTemp, Log, TEXT("MinionPoolManager: Returned minion to pool. Active: %d, Inactive: %d"),
		ActiveMinions.Num(), InactiveMinions.Num());
}

AEnemy_Base* UMinionPoolManager::SpawnNewMinion()
{
	if (!MinionClassToSpawn)
	{
		UE_LOG(LogTemp, Error, TEXT("MinionPoolManager: No minion class set!"));
		return nullptr;
	}

	// Additional validation
	if (!MinionClassToSpawn->IsChildOf(AEnemy_Base::StaticClass()))
	{
		UE_LOG(LogTemp, Error, TEXT("MinionPoolManager: MinionClassToSpawn is not a valid Enemy_Base class! Class: %s"),
			*MinionClassToSpawn->GetName());
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("MinionPoolManager: World is null!"));
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	UE_LOG(LogTemp, Log, TEXT("MinionPoolManager: Attempting to spawn minion of class: %s"),
		*MinionClassToSpawn->GetName());

	AEnemy_Base* Minion = World->SpawnActor<AEnemy_Base>(
		MinionClassToSpawn,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (Minion)
	{
		// Disable lifespan (pooled minions don't auto-destroy)
		Minion->SetLifeSpan(0.f);

		// Spawn AI Controller if not already assigned
		if (!Minion->GetController())
		{
			Minion->SpawnDefaultController();
		}

		UE_LOG(LogTemp, Log, TEXT("MinionPoolManager: Successfully spawned minion %s with controller: %s"),
			*Minion->GetName(),
			Minion->GetController() ? TEXT("YES") : TEXT("NO"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MinionPoolManager: Failed to spawn minion! SpawnActor returned null."));
	}

	return Minion;
}

void UMinionPoolManager::DeactivateMinion(AEnemy_Base* Minion)
{
	if (!Minion)
		return;

	// Hide the minion
	Minion->SetActorHiddenInGame(true);
	Minion->SetActorEnableCollision(false);
	Minion->SetActorTickEnabled(false);

	// Clear combat target
	UCombatComponent* Combat = Minion->FindComponentByClass<UCombatComponent>();
	if (Combat)
	{
		Combat->ClearTarget();
	}

	// Reset stats
	UCharacterStatComponent* Stats = Minion->FindComponentByClass<UCharacterStatComponent>();
	if (Stats)
	{
		Stats->ResetStats();
	}

	// Clear movement target
	Minion->SetMovementTarget(nullptr);
}

void UMinionPoolManager::ActivateMinion(AEnemy_Base* Minion, const FVector& Location, const FRotator& Rotation)
{
	if (!Minion)
		return;

	// Reset location and rotation FIRST
	Minion->SetActorLocationAndRotation(Location, Rotation);

	// Show and enable the minion
	Minion->SetActorHiddenInGame(false);
	Minion->SetActorEnableCollision(true);
	Minion->SetActorTickEnabled(true);

	// Make sure mesh is visible
	if (ACharacter* Character = Cast<ACharacter>(Minion))
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			Mesh->SetVisibility(true);
			Mesh->SetHiddenInGame(false);
		}
	}

	// Ensure AI controller is assigned (CRITICAL for pooled minions!)
	if (!Minion->GetController())
	{
		Minion->SpawnDefaultController();
		UE_LOG(LogTemp, Warning, TEXT("MinionPool: Minion had no controller, spawned new one"));
	}

	// Re-bind death event (BeginPlay doesn't get called for pooled actors)
	UCharacterStatComponent* Stats = Minion->FindComponentByClass<UCharacterStatComponent>();
	if (Stats)
	{
		// Clear previous bindings to avoid duplicates
		Stats->OnDeath.Clear();
		Stats->OnDeath.AddDynamic(Minion, &AEnemy_Base::HandleDeath);

		// Reset stats to full health
		Stats->ResetStats();
	}

	// Re-enable character movement
	if (UCharacterMovementComponent* Movement = Minion->GetCharacterMovement())
	{
		Movement->SetComponentTickEnabled(true);
		Movement->Activate();
	}

	UE_LOG(LogTemp, Log, TEXT("MinionPool: Activated minion at location: %s, Controller: %s"),
		*Location.ToString(), Minion->GetController() ? TEXT("YES") : TEXT("NO"));
}

