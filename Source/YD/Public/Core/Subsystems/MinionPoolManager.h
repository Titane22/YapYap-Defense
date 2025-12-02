// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MinionPoolManager.generated.h"

class AEnemy_Base;

/**
 * Object Pooling system for minions to reduce spawn/destroy overhead
 */
UCLASS()
class YD_API UMinionPoolManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Initialize the pool with a specific minion class */
	UFUNCTION(BlueprintCallable, Category = "Minion Pool")
	void InitializePool(TSubclassOf<AEnemy_Base> MinionClass, int32 InitialPoolSize = 20);

	/** Get a minion from the pool (reuses if available, spawns new if not) */
	UFUNCTION(BlueprintCallable, Category = "Minion Pool")
	AEnemy_Base* GetMinion(const FVector& SpawnLocation, const FRotator& SpawnRotation);

	/** Return a minion to the pool (instead of destroying) */
	UFUNCTION(BlueprintCallable, Category = "Minion Pool")
	void ReturnMinion(AEnemy_Base* Minion);

	/** Get number of active minions */
	UFUNCTION(BlueprintPure, Category = "Minion Pool")
	int32 GetActiveCount() const { return ActiveMinions.Num(); }

	/** Get number of inactive (pooled) minions */
	UFUNCTION(BlueprintPure, Category = "Minion Pool")
	int32 GetInactiveCount() const { return InactiveMinions.Num(); }

protected:
	/** The minion class to spawn */
	UPROPERTY()
	TSubclassOf<AEnemy_Base> MinionClassToSpawn;

	/** Currently active minions */
	UPROPERTY()
	TArray<AEnemy_Base*> ActiveMinions;

	/** Inactive minions ready for reuse */
	UPROPERTY()
	TArray<AEnemy_Base*> InactiveMinions;

	/** Spawn a new minion and add to pool */
	AEnemy_Base* SpawnNewMinion();

	/** Deactivate minion (hide and disable) */
	void DeactivateMinion(AEnemy_Base* Minion);

	/** Activate minion (show and enable) */
	void ActivateMinion(AEnemy_Base* Minion, const FVector& Location, const FRotator& Rotation);
};
