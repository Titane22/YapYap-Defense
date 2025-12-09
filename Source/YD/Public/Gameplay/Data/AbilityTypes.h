// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AbilityTypes.generated.h"

// Forward declarations
class UAbilityEffect;
class AProjectile_Base;
class AAOE_Base;

// ============ Enums ============

/** Enum for ability slots */
UENUM(BlueprintType)
enum class EAbilitySlot : uint8
{
	Q UMETA(DisplayName = "Q Ability"),
	W UMETA(DisplayName = "W Ability"),
	E UMETA(DisplayName = "E Ability"),
	R UMETA(DisplayName = "R Ability")
};

UENUM(BlueprintType)
enum class EAbilityDeliveryType : uint8
{
	Instant,        // 즉시 적용 (타겟팅 순간 바로 효과)
	Projectile,     // 발사체 (이동하는 투사체)
	GroundAOE,      // 지면 AOE (지정 위치에 즉시 생성)
	DelayedAOE,     // 지연 AOE (일정 시간 후 발동)
	Beam,           // 빔/레이저
	Chain,          // 연쇄 (한 타겟에서 다른 타겟으로)
	Dash            // 돌진 (캐릭터가 이동하며 적용)
};

// ============ Structs ============

USTRUCT(BlueprintType)
struct FAbilityDeliveryConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	EAbilityDeliveryType DeliveryType = EAbilityDeliveryType::Instant;

	// Projectile 설정
	UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "DeliveryType == EAbilityDeliveryType::Projectile"))
	TSubclassOf<AProjectile_Base> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "DeliveryType == EAbilityDeliveryType::Projectile"))
	float ProjectileSpeed = 1000.0f;

	UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "DeliveryType == EAbilityDeliveryType::Projectile"))
	bool bIsHoming = false;

	UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "DeliveryType == EAbilityDeliveryType::Projectile"))
	float HomingAcceleration = 500.0f;

	UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "DeliveryType == EAbilityDeliveryType::Projectile"))
	bool bPierceTargets = false;  // 관통 여부

	UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "DeliveryType == EAbilityDeliveryType::Projectile"))
	int32 MaxPierceCount = 0;

	// AOE 설정
	UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "DeliveryType == EAbilityDeliveryType::GroundAOE || DeliveryType == EAbilityDeliveryType::DelayedAOE"))
	float AOERadius = 0.0f;

	UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "DeliveryType == EAbilityDeliveryType::DelayedAOE"))
	float DelayTime = 1.0f;

	UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "DeliveryType == EAbilityDeliveryType::GroundAOE || DeliveryType == EAbilityDeliveryType::DelayedAOE"))
	TSubclassOf<AAOE_Base> AOEIndicatorClass;

	// Beam 설정
	UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "DeliveryType == EAbilityDeliveryType::Beam"))
	float BeamDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "DeliveryType == EAbilityDeliveryType::Beam"))
	float BeamWidth = 50.0f;

	// 공통 설정
	UPROPERTY(EditDefaultsOnly)
	UParticleSystem* SpawnVFX;

	UPROPERTY(EditDefaultsOnly)
	UParticleSystem* ImpactVFX;

	UPROPERTY(EditDefaultsOnly)
	USoundBase* SpawnSound;

	UPROPERTY(EditDefaultsOnly)
	USoundBase* ImpactSound;
};

USTRUCT(BlueprintType)
struct FAbilityEffectData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UAbilityEffect> EffectClass;

	UPROPERTY(EditDefaultsOnly)
	float BaseValue;

	UPROPERTY(EditDefaultsOnly)
	float ADScaling;  // 공격력 계수

	UPROPERTY(EditDefaultsOnly)
	float APScaling;  // 주문력 계수

	UPROPERTY(EditDefaultsOnly)
	float Duration;

	UPROPERTY(EditDefaultsOnly)
	UParticleSystem* EffectVFX;

	UPROPERTY(EditDefaultsOnly)
	USoundBase* EffectSound;
};

USTRUCT(BlueprintType)
struct FAbilityLevelData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	float DamageMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float CooldownReduction = 0.0f;

	UPROPERTY(EditDefaultsOnly)
	float ManaCostIncrease = 0.0f;

	UPROPERTY(EditDefaultsOnly)
	float RangeIncrease = 0.0f;
};
