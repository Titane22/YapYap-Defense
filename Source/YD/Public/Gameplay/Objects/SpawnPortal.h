// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpawnPortal.generated.h"

class UBoxComponent;

UCLASS()
class YD_API ASpawnPortal : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASpawnPortal();

	/** Check if this portal is enabled for spawning */
	UFUNCTION(BlueprintPure, Category = "Spawn Portal")
	bool GetEnabled() const { return bIsEnabled; }

	/** Enable or disable this portal */
	UFUNCTION(BlueprintCallable, Category = "Spawn Portal")
	void SetEnabled(bool bEnabled) { bIsEnabled = bEnabled; }

	/** Get the spawn box component */
	UFUNCTION(BlueprintPure, Category = "Spawn Portal")
	UBoxComponent* GetSpawnBox() const { return SpawnBox; }

	/** Calculate grid spawn positions within the box */
	UFUNCTION(BlueprintCallable, Category = "Spawn Portal")
	TArray<FVector> GetGridSpawnPositions(int32 Count, int32 ColumnsPerRow = 5) const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootScene;

	/** Box defining spawn area (visible in editor) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* SpawnBox;

	/** Whether this portal is active and spawning minions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Portal")
	bool bIsEnabled = true;
};
