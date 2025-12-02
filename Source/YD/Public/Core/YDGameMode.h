// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "YDGameMode.generated.h"

class UMinionPoolManager;
class UMinionBatchProcessor;
class ASpawnPortal;
class AEnemy_Base;

UCLASS(minimalapi)
class AYDGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AYDGameMode();

	virtual void BeginPlay();

	/** Spawn minions at all active spawn portals */
	UFUNCTION(BlueprintCallable, Category = "Minions")
	void SpawnMinionWave();

	/** Get all active spawn portals in the level */
	UFUNCTION(BlueprintCallable, Category = "Minions")
	TArray<ASpawnPortal*> GetActiveSpawnPortals() const;

protected:
	UPROPERTY()
	UMinionPoolManager* MinionPool;

	UPROPERTY()
	UMinionBatchProcessor* BatchProcessor;

	/** Number of minions to spawn per portal */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manager|Minions")
	int32 MinionsPerPortal = 3;

	/** Minion class to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manager|Minions")
	TSubclassOf<AEnemy_Base> MinionClass;

	/** Initial pool size for minion pooling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Manager|Minions")
	int32 InitialPoolSize = 20;
};



