// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "TargetingStrategy.h"
#include "Engine/DataAsset.h"
#include "AbilityTypes.h"
#include "AbilityData.generated.h"

class UTargetingStrategy;
/**
 * Data asset for ability configuration
 */
UCLASS(BlueprintType)
class YD_API UAbilityData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
    // ============ Basic Info ============
    UPROPERTY(EditDefaultsOnly, Category = "Basic")
    FText AbilityName;
    
    UPROPERTY(EditDefaultsOnly, Category = "Basic")
    FText Description;
    
    UPROPERTY(EditDefaultsOnly, Category = "Basic")
    UTexture2D* Icon;
    
    UPROPERTY(EditDefaultsOnly, Category = "Basic")
    FGameplayTag AbilityTag;
    
    // ============ Targeting ============
    UPROPERTY(EditDefaultsOnly, Category = "Targeting")
    FTargetingConfig TargetingConfig;
    
    UPROPERTY(EditDefaultsOnly, Category = "Targeting", AdvancedDisplay)
    TSubclassOf<UTargetingStrategy> CustomTargetingClass;  // 특수 케이스용
    
    // ============ Cost & Cooldown ============
    UPROPERTY(EditDefaultsOnly, Category = "Cost")
    float ManaCost = 0.0f;
    
    UPROPERTY(EditDefaultsOnly, Category = "Cost")
    float Cooldown = 0.0f;
    
    UPROPERTY(EditDefaultsOnly, Category = "Cost")
    float CastTime = 0.0f;
    
    UPROPERTY(EditDefaultsOnly, Category = "Cost")
    bool bCanMoveWhileCasting = false;
    
    UPROPERTY(EditDefaultsOnly, Category = "Cost")
    int32 MaxCharges = 1;  // 충전 시스템 (예: 코르키 R)
    
    // ============ Delivery (발사 방식) ============
    UPROPERTY(EditDefaultsOnly, Category = "Delivery")
    FAbilityDeliveryConfig DeliveryConfig;
    
    // ============ Effects ============
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    TArray<FAbilityEffectData> Effects;
    
    // ============ Scaling ============
    UPROPERTY(EditDefaultsOnly, Category = "Scaling")
    TArray<FAbilityLevelData> LevelScaling;  // 5레벨 분량
    
    // ============ Visual & Audio ============
    UPROPERTY(EditDefaultsOnly, Category = "Presentation")
    UAnimMontage* CastAnimation;
    
    UPROPERTY(EditDefaultsOnly, Category = "Presentation")
    UParticleSystem* CastVFX;
    
    UPROPERTY(EditDefaultsOnly, Category = "Presentation")
    USoundBase* CastSound;
    
    // ============ Special Flags ============
    UPROPERTY(EditDefaultsOnly, Category = "Advanced")
    bool bIsPassive = false;
    
    UPROPERTY(EditDefaultsOnly, Category = "Advanced")
    bool bRequiresFacing = true;  // 타겟을 바라봐야 하는지
    
    UPROPERTY(EditDefaultsOnly, Category = "Advanced")
    TArray<FGameplayTag> RequiredBuffs;  // 필요한 버프 
};
