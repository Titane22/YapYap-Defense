// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class UCharacterStatComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttackStarted, AActor*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttackHit, AActor*, Target);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class YD_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Set the current attack target */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetTarget(AActor* NewTarget);

	/** Clear the current attack target */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ClearTarget();

	/** Get the current attack target */
	UFUNCTION(BlueprintPure, Category = "Combat")
	AActor* GetTarget() const { return CurrentTarget; }

	/** Check if currently attacking */
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsAttacking() const { return CurrentTarget != nullptr; }

	/** Check if in attack range of target */
	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsInAttackRange(AActor* Target) const;

	/** Manually trigger an attack (used by AI or special abilities) */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Attack(AActor* Target);

	/** Apply melee damage to current target (called from AnimNotify) */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ApplyMeleeDamage();

	/** Cancel current attack montage (e.g., when target goes out of range) */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void CancelAttackMontage();

	/** Events for animation/effects */
	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnAttackStarted OnAttackStarted;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnAttackHit OnAttackHit;

protected:
	/** Current attack target */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	AActor* CurrentTarget;

	/** Reference to owner's stat component */
	UPROPERTY()
	UCharacterStatComponent* StatComponent;

	/** Time since last attack */
	float TimeSinceLastAttack;

	/** Attack cooldown (calculated from attack speed) */
	float AttackCooldown;

	/** Perform the attack logic */
	void PerformAttack();

	/** Update attack cooldown based on attack speed */
	void UpdateAttackCooldown();

	/** Check if target is valid and alive */
	bool IsTargetValid() const;
};
