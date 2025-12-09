// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TargetingStrategy.generated.h"

UENUM(BlueprintType)
enum class ETargetingType : uint8
{
	None            UMETA(DisplayName = "Non-Target"),      // 논타겟 (자신/주변)
	Unit            UMETA(DisplayName = "Unit Target"),     // 단일 유닛 타겟
	Ground          UMETA(DisplayName = "Ground Target"),   // 지면 타겟
	Direction       UMETA(DisplayName = "Direction/Line"),  // 방향/스킬샷
	GroundAOE       UMETA(DisplayName = "Ground AOE"),      // 지면 AOE
	UnitAOE         UMETA(DisplayName = "Unit AOE"),        // 유닛 중심 AOE
	Cone            UMETA(DisplayName = "Cone"),            // 부채꼴
	Auto            UMETA(DisplayName = "Auto Target")      // 자동 타겟 (패시브)
};

// 타겟 필터 옵션
UENUM(BlueprintType, Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ETargetFilter : uint8
{
	None        = 0 UMETA(Hidden), 
	Ally        = 1 << 0,    // 아군
	Enemy       = 1 << 1,    // 적군
	Self        = 1 << 2,    // 자신
	Minion      = 1 << 3,    // 미니언
	Champion    = 1 << 4,    // 챔피언
	Structure   = 1 << 5,    // 구조물
	Neutral     = 1 << 6     // 중립 몬스터
};

// 타겟팅 설정
USTRUCT(BlueprintType)
struct FTargetingConfig
{
	GENERATED_BODY()
    
	UPROPERTY(EditDefaultsOnly)
	ETargetingType TargetingType = ETargetingType::None;
    
	UPROPERTY(EditDefaultsOnly, Meta = (Bitmask, BitmaskEnum = "ETargetFilter"))
	int32 TargetFilter = 0;
    
	UPROPERTY(EditDefaultsOnly)
	float Range = 0.0f;
    
	UPROPERTY(EditDefaultsOnly)
	float Radius = 0.0f;  // AOE 반경
    
	UPROPERTY(EditDefaultsOnly)
	float Width = 0.0f;   // 스킬샷 폭
    
	UPROPERTY(EditDefaultsOnly)
	float Angle = 0.0f;   // Cone 각도
    
	UPROPERTY(EditDefaultsOnly)
	bool bRequiresLineOfSight = true;
    
	UPROPERTY(EditDefaultsOnly)
	bool bCanTargetThroughWalls = false;
    
	UPROPERTY(EditDefaultsOnly)
	int32 MaxTargets = 1;  // 다중 타겟 개수
};

// 타겟팅 결과
USTRUCT(BlueprintType)
struct FAbilityTargetData
{
	GENERATED_BODY()
    
	UPROPERTY()
	AActor* TargetActor = nullptr;
    
	UPROPERTY()
	TArray<AActor*> TargetActors;
    
	UPROPERTY()
	FVector TargetLocation = FVector::ZeroVector;
    
	UPROPERTY()
	FVector Direction = FVector::ZeroVector;
    
	UPROPERTY()
	bool bIsValid = false;
};
/**
 * Targeting strategy for abilities
 * Handles target collection, validation, and filtering based on FTargetingConfig
 */
UCLASS()
class YD_API UTargetingStrategy : public UObject
{
	GENERATED_BODY()

public:
	
	UPROPERTY()
	FTargetingConfig Config;

	UPROPERTY()
	AActor* OwningActor = nullptr;

public:
	
	// ===========================
	// Initialization
	// ===========================
	virtual void Initialize(const FTargetingConfig& IsConfig, AActor* Owner);

	// ===========================
	// Main Interface - UAbility에서 호출
	// ===========================

	/**
	 * 타겟 데이터를 기반으로 최종 유효한 타겟들을 반환
	 * @param TargetData - 플레이어 입력으로부터 받은 타겟 데이터
	 * @return 필터링되고 검증된 타겟 배열
	 */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	virtual TArray<AActor*> GetValidTargets(const FAbilityTargetData& TargetData);

	/**
	 * 특정 액터가 타겟 가능한지 검증
	 * @param Target - 검증할 액터
	 * @return 타겟 가능 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	virtual bool ValidateTarget(AActor* Target) const;

	/**
	 * 타겟 데이터가 유효한지 검증 (범위, 위치 등)
	 * @param TargetData - 검증할 타겟 데이터
	 * @return 유효성 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "Targeting")
	virtual bool ValidateTargetData(const FAbilityTargetData& TargetData) const;

protected:
	// ===========================
	// Target Collection (타입별)
	// ===========================

	/** 타겟 타입에 따라 잠재적 타겟들 수집 */
	virtual TArray<AActor*> CollectPotentialTargets(const FAbilityTargetData& TargetData);

	/** None 타입: 자신 또는 주변 */
	virtual TArray<AActor*> CollectTargets_None();

	/** Unit 타입: 단일 유닛 */
	virtual TArray<AActor*> CollectTargets_Unit(const FAbilityTargetData& TargetData);

	/** Ground 타입: 지면 위치 기준 */
	virtual TArray<AActor*> CollectTargets_Ground(const FAbilityTargetData& TargetData);

	/** Direction 타입: 방향/스킬샷 */
	virtual TArray<AActor*> CollectTargets_Direction(const FAbilityTargetData& TargetData);

	/** UnitAOE 타입: 유닛 중심 AOE */
	virtual TArray<AActor*> CollectTargets_UnitAOE(const FAbilityTargetData& TargetData);

	/** GroundAOE 타입: 지면 AOE */
	virtual TArray<AActor*> CollectTargets_GroundAOE(const FAbilityTargetData& TargetData);

	/** Cone 타입: 부채꼴 */
	virtual TArray<AActor*> CollectTargets_Cone(const FAbilityTargetData& TargetData);

	/** Auto 타입: 자동 타겟 (가장 가까운 적 등) */
	virtual TArray<AActor*> CollectTargets_Auto();

	// ===========================
	// Filtering & Validation Helpers
	// ===========================

	/** 타겟 필터 통과 여부 */
	virtual bool PassesFilter(AActor* Target) const;

	virtual bool IsInRange(AActor* Target) const;

	virtual bool HasLineOfSight(AActor* Target) const;

	virtual bool IsAlly(AActor* Target) const;

	virtual bool IsEnemy(AActor* Target) const;

	virtual bool IsSelf(AActor* Target) const;

	virtual bool IsMinion(AActor* Target) const;

	virtual bool IsChampion(AActor* Target) const;

	virtual bool IsStructure(AActor* Target) const;

	virtual bool IsNeutral(AActor* Target) const;

	// ===========================
	// Geometry Helpers (범위 수집)
	// ===========================

	/** 구체 범위 내 액터 수집 */
	TArray<AActor*> GetActorsInSphere(const FVector& Center, float Radius) const;

	/** 박스 범위 내 액터 수집 (스킬샷용) */
	TArray<AActor*> GetActorsInBox(const FVector& Start, const FVector& End, float Width) const;

	/** 부채꼴 범위 내 액터 수집 */
	TArray<AActor*> GetActorsInCone(const FVector& Origin, const FVector& Direction, float Range, float AngleDegrees) const;

	// ===========================
	// Utility
	// ===========================

	/** MaxTargets에 맞춰 타겟 수 제한 (가까운 순으로 정렬) */
	virtual TArray<AActor*> LimitTargetCount(TArray<AActor*> Targets, const FVector& ReferencePoint) const;

	/** 거리순 정렬 */
	void SortByDistance(TArray<AActor*>& Targets, const FVector& ReferencePoint) const;

	/** World 가져오기 */
	UWorld* GetWorld() const override;
};
