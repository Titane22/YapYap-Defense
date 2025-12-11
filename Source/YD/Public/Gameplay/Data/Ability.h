// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TargetingStrategy.h"
#include "UObject/Object.h"
#include "Ability.generated.h"

enum class EAbilitySlot : uint8;
class UAbilityComponent;
class UAbilityData;
class UAbilityEffect;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAbilityExecuted, UAbility*, Ability, FAbilityTargetData, TargetData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAbilityCastStarted, UAbility*, Ability, float, CastTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityCastCancelled, UAbility*, Ability);
/**
 * 
 */
UCLASS()
class YD_API UAbility : public UObject
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// ============ References ============
	UPROPERTY(BlueprintReadOnly, Category = "Ability")
	UAbilityData* AbilityData;  // 데이터 참조
    
	UPROPERTY()
	AActor* OwningActor;
    
	UPROPERTY()
	UAbilityComponent* OwningComponent;
    
	UPROPERTY()
	EAbilitySlot AbilitySlot;  // Q=0, W=1, E=2, R=3
    
	// ============ Runtime State ============
	UPROPERTY(Replicated)
	int32 CurrentLevel = 0;  // 0 = 스킬 배우지 않음
    
	UPROPERTY(Replicated)
	float RemainingCooldown = 0.0f;
    
	UPROPERTY(Replicated)
	int32 CurrentCharges = 1;
    
	UPROPERTY(Replicated)
	bool bIsCasting = false;

	// Pending execution (for out of range targets)
	bool bHasPendingExecution = false;
	FAbilityTargetData PendingExecutionTargetData;

	// ============================================
	// Runtime Objects
	// ============================================
    
	UPROPERTY()
	UTargetingStrategy* TargetingStrategy;
    
	UPROPERTY()
	TArray<UAbilityEffect*> RuntimeEffects;
    
	UPROPERTY()
	FTimerHandle CastTimerHandle;
    
	UPROPERTY()
	FAbilityTargetData PendingTargetData;
public:
	// ============ Public Interface ============
	UFUNCTION(BlueprintCallable, Category = "Ability")
	void Initialize(UAbilityData* InData, AActor* Owner, EAbilitySlot Slot);

	UFUNCTION(BlueprintCallable, Category = "Ability")
	void Execute(const FAbilityTargetData& TargetData);

	UFUNCTION(BlueprintCallable, Category = "Ability")
	void LevelUp();

	UFUNCTION(BlueprintCallable, Category = "Ability")
	void CancelCast();

	/** Spawn projectile at specified location (called from AnimNotify) */
	UFUNCTION(BlueprintCallable, Category = "Ability")
	void SpawnProjectile(const FVector& SpawnLocation, const FRotator& SpawnRotation);

	// ============ Queries(외부에서 조회) ============
	bool CanCast() const;
	bool IsOnCooldown() const { return RemainingCooldown > 0.0f && CurrentCharges == 0; }
	bool IsMaxLevel() const;
    
	float GetManaCost() const;
	float GetCooldown() const;
	float GetRange() const;
	float GetCooldownPercent() const;

	void UpdateCooldown(float DeltaTime);

	UFUNCTION()
	void CheckPendingExecution();

	UFUNCTION()
	void CancelPendingExecution();

	// ============ Events ============
	UPROPERTY(BlueprintAssignable)
	FOnAbilityExecuted OnAbilityExecuted;
    
	UPROPERTY(BlueprintAssignable)
	FOnAbilityCastStarted OnAbilityCastStarted;
    
	UPROPERTY(BlueprintAssignable)
	FOnAbilityCastCancelled OnAbilityCastCancelled;
	
protected:
	// ============ Initialization Helpers ============
	void CreateTargetingStrategy();
	void CreateRuntimeEffects();

	// ============ Validation ============
	bool ValidateTargetData(const FAbilityTargetData& TargetData) const;

	void StartCasting(const FAbilityTargetData& TargetData);
	void OnCastComplete();

	// 실제 실행 (캐스팅 완료 후)
	void ExecuteDelivery(const FAbilityTargetData& TargetData);

	// Rotate character to face target
	void RotateOwnerToTarget(const FAbilityTargetData& TargetData);

	void ExecuteInstant(const FAbilityTargetData& TargetData);
	void ExecuteProjectile(const FAbilityTargetData& TargetData);
	void ExecuteAOE(const FAbilityTargetData& TargetData);

	void ApplyEffectsToActor(AActor* Target);

	void ConsumeResources();
	void RefundResources();
	void StartCooldown();

	void PlayPresentation(const FVector& Location);

	// Helpers (TargetingStrategy 활용)
	TArray<AActor*> GetValidTargetsFromData(const FAbilityTargetData& TargetData);

	// Delivery Callbacks
	UFUNCTION()
	void OnProjectileHit(AActor* HitActor, FHitResult Hit);

	UFUNCTION()
	void OnAOEOverlap(AActor* OverlappedActor);
};
