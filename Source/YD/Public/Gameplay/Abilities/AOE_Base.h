// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AOE_Base.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAOE_OverlapActor, AActor*, TargetActor);

UCLASS()
class YD_API AAOE_Base : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAOE_Base();
	
	void SpawnAOE_Sphere();

	void Trigger();
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AOE|Settings")
	float Radius;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AOE|Settings")
	bool bIgnoreInstigator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AOE|Settings")
	bool bTriggerOnBeginPlay;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AOE|Debug")
	bool bDrawDebugSphere;

public:
	UPROPERTY(BlueprintAssignable, Category = "AOE")
	FOnAOE_OverlapActor OnOverlapActor;
};
