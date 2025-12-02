// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "MinionBatchProcessor.generated.h"

class AEnemy_Base;

/**
 * Batch processor for minions - handles all minion AI updates in one place for performance
 */
UCLASS()
class YD_API UMinionBatchProcessor : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// UWorldSubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// FTickableGameObject interface
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return !IsTemplate(); }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMinionBatchProcessor, STATGROUP_Tickables); }

	/** Register a minion to be processed by batch system */
	UFUNCTION(BlueprintCallable, Category = "Minion Batch")
	void RegisterMinion(AEnemy_Base* Minion);

	/** Unregister a minion from batch processing */
	UFUNCTION(BlueprintCallable, Category = "Minion Batch")
	void UnregisterMinion(AEnemy_Base* Minion);

	/** Get number of registered minions */
	UFUNCTION(BlueprintPure, Category = "Minion Batch")
	int32 GetRegisteredCount() const { return RegisteredMinions.Num(); }

protected:
	/** All minions being batch processed */
	UPROPERTY()
	TArray<AEnemy_Base*> RegisteredMinions;

	/** Timer for target updates */
	float TargetUpdateTimer;

	/** How often to update targets (in seconds) */
	UPROPERTY(EditAnywhere, Category = "Minion Batch")
	float TargetUpdateInterval;

	/** Batch update all minion targets */
	void BatchUpdateTargets();

	/** Find closest enemy for a minion from a list of potential targets */
	AActor* FindClosestEnemy(AEnemy_Base* Minion, const TArray<FOverlapResult>& PotentialTargets) const;
};
