// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Characters/Player/YDCharacter.h"
#include "Enemy_Base.generated.h"

class UCharacterStatComponent;
class UCombatComponent;
class UTargetingStrategy;
class AActor;
class AEnemy_Base;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemyDeath, AEnemy_Base*, Enemy, AActor*, Killer);

/**
 * 
 */
UCLASS()
class YD_API AEnemy_Base : public AYDCharacter
{
	GENERATED_BODY()

public:
	AEnemy_Base();

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void SetMovementTarget(AActor* NewTarget);

	virtual bool IsEnemy(AActor* Actor) const;

	UFUNCTION()
	void HandleDeath();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	virtual void OnDeath(AActor* Killer);

	UFUNCTION(BlueprintPure, Category = "Enemy")
	AActor* GetMovementTarget() const { return MovementTarget; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	int32 GetGoldReward() const { return GoldReward; }

	UPROPERTY(BLueprintAssignable, Category = "Enemy")
	FOnEnemyDeath OnEnemyDeath;

	void SearchForEnmies();

	void InitializeTargetingStrategy();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	AActor* MovementTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rewards")
	int32 GoldReward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rewards")
	int32 ExperienceReward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float DetectionRange;

	float DetectionTimer;
	float MoveUpdateTimer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UAnimMontage* AttackMontage;

	// TargetingStrategy for enemy detection
	UPROPERTY()
	UTargetingStrategy* EnemyDetectionStrategy;

private:
	UFUNCTION()
	void OnAttackStartedHandler(AActor* Target);

};
