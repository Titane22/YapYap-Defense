// Fill out your copyright notice in the Description page of Project Settings.

#include "Gameplay/Data/TargetingStrategy.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/OverlapResult.h"
#include "WorldCollision.h"

void UTargetingStrategy::Initialize(const FTargetingConfig& IsConfig, AActor* Owner)
{
	Config = IsConfig;
	OwningActor = Owner;
}

// ============================================
// Main Interface
// ============================================

TArray<AActor*> UTargetingStrategy::GetValidTargets(const FAbilityTargetData& TargetData)
{
	if (!OwningActor)
		return TArray<AActor*>();

	// 1. 타겟 타입에 따라 잠재적 타겟들 수집
	TArray<AActor*> PotentialTargets = CollectPotentialTargets(TargetData);

	// 2. 필터링 및 검증
	TArray<AActor*> ValidTargets;
	for (AActor* Target : PotentialTargets)
	{
		if (ValidateTarget(Target))
		{
			ValidTargets.Add(Target);
		}
	}

	// 3. MaxTargets 제한 적용
	if (Config.MaxTargets > 0 && ValidTargets.Num() > Config.MaxTargets)
	{
		FVector ReferencePoint = TargetData.TargetLocation.IsNearlyZero()
			? OwningActor->GetActorLocation()
			: TargetData.TargetLocation;
		ValidTargets = LimitTargetCount(ValidTargets, ReferencePoint);
	}

	return ValidTargets;
}

bool UTargetingStrategy::ValidateTarget(AActor* Target) const
{
	if (!Target || !OwningActor)
		return false;

	// 1. 필터 체크
	if (!PassesFilter(Target))
		return false;

	// 2. 범위 체크
	if (!IsInRange(Target))
		return false;

	// 3. 시야 체크
	if (Config.bRequiresLineOfSight && !Config.bCanTargetThroughWalls)
	{
		if (!HasLineOfSight(Target))
			return false;
	}

	return true;
}

bool UTargetingStrategy::ValidateTargetData(const FAbilityTargetData& TargetData) const
{
	if (!OwningActor)
		return false;

	switch (Config.TargetingType)
	{
	case ETargetingType::Unit:
		return TargetData.TargetActor != nullptr;

	case ETargetingType::Ground:
	case ETargetingType::GroundAOE:
		return !TargetData.TargetLocation.IsNearlyZero();

	case ETargetingType::Direction:
	case ETargetingType::Cone:
		return !TargetData.Direction.IsNearlyZero();

	case ETargetingType::UnitAOE:
		return TargetData.TargetActor != nullptr || !TargetData.TargetLocation.IsNearlyZero();

	case ETargetingType::None:
	case ETargetingType::Auto:
		return true;

	default:
		return false;
	}
}

// ============================================
// Target Collection
// ============================================

TArray<AActor*> UTargetingStrategy::CollectPotentialTargets(const FAbilityTargetData& TargetData)
{
	switch (Config.TargetingType)
	{
	case ETargetingType::None:
		return CollectTargets_None();

	case ETargetingType::Unit:
		return CollectTargets_Unit(TargetData);

	case ETargetingType::Ground:
		return CollectTargets_Ground(TargetData);

	case ETargetingType::Direction:
		return CollectTargets_Direction(TargetData);

	case ETargetingType::GroundAOE:
		return CollectTargets_GroundAOE(TargetData);

	case ETargetingType::UnitAOE:
		return CollectTargets_UnitAOE(TargetData);

	case ETargetingType::Cone:
		return CollectTargets_Cone(TargetData);

	case ETargetingType::Auto:
		return CollectTargets_Auto();

	default:
		return TArray<AActor*>();
	}
}

TArray<AActor*> UTargetingStrategy::CollectTargets_None()
{
	TArray<AActor*> Targets;

	// Self 필터가 있으면 자신을 타겟으로
	if ((Config.TargetFilter & static_cast<int32>(ETargetFilter::Self)) != 0)
	{
		Targets.Add(OwningActor);
	}
	// 아니면 주변 범위 내 타겟 수집
	else if (Config.Radius > 0.0f)
	{
		FVector Origin = OwningActor->GetActorLocation();
		Targets = GetActorsInSphere(Origin, Config.Radius);
	}

	return Targets;
}

TArray<AActor*> UTargetingStrategy::CollectTargets_Unit(const FAbilityTargetData& TargetData)
{
	TArray<AActor*> Targets;

	if (TargetData.TargetActor)
	{
		Targets.Add(TargetData.TargetActor);
	}

	return Targets;
}

TArray<AActor*> UTargetingStrategy::CollectTargets_Ground(const FAbilityTargetData& TargetData)
{
	TArray<AActor*> Targets;

	// Ground 타입은 위치만 필요하고 타겟 수집은 안함
	// (실제 타겟이 필요하면 Radius 설정)
	if (Config.Radius > 0.0f)
	{
		Targets = GetActorsInSphere(TargetData.TargetLocation, Config.Radius);
	}

	return Targets;
}

TArray<AActor*> UTargetingStrategy::CollectTargets_Direction(const FAbilityTargetData& TargetData)
{
	if (!OwningActor)
		return TArray<AActor*>();

	FVector Start = OwningActor->GetActorLocation();
	FVector End = Start + (TargetData.Direction.GetSafeNormal() * Config.Range);

	return GetActorsInBox(Start, End, Config.Width);
}

TArray<AActor*> UTargetingStrategy::CollectTargets_GroundAOE(const FAbilityTargetData& TargetData)
{
	return GetActorsInSphere(TargetData.TargetLocation, Config.Radius);
}

TArray<AActor*> UTargetingStrategy::CollectTargets_UnitAOE(const FAbilityTargetData& TargetData)
{
	FVector Center;

	if (TargetData.TargetActor)
	{
		Center = TargetData.TargetActor->GetActorLocation();
	}
	else if (!TargetData.TargetLocation.IsNearlyZero())
	{
		Center = TargetData.TargetLocation;
	}
	else
	{
		Center = OwningActor->GetActorLocation();
	}

	return GetActorsInSphere(Center, Config.Radius);
}

TArray<AActor*> UTargetingStrategy::CollectTargets_Cone(const FAbilityTargetData& TargetData)
{
	if (!OwningActor)
		return TArray<AActor*>();

	FVector Origin = OwningActor->GetActorLocation();
	FVector Direction = TargetData.Direction.GetSafeNormal();

	if (Direction.IsNearlyZero())
	{
		Direction = OwningActor->GetActorForwardVector();
	}

	return GetActorsInCone(Origin, Direction, Config.Range, Config.Angle);
}

TArray<AActor*> UTargetingStrategy::CollectTargets_Auto()
{
	// 자동 타겟: 범위 내 가장 가까운 적
	TArray<AActor*> PotentialTargets = GetActorsInSphere(
		OwningActor->GetActorLocation(),
		Config.Range
	);

	if (PotentialTargets.Num() > 0)
	{
		SortByDistance(PotentialTargets, OwningActor->GetActorLocation());
		return TArray<AActor*>{PotentialTargets[0]};
	}

	return TArray<AActor*>();
}

// ============================================
// Filtering & Validation Helpers
// ============================================

bool UTargetingStrategy::PassesFilter(AActor* Target) const
{
	if (!Target)
		return false;

	// 필터가 없으면 통과
	if (Config.TargetFilter == 0)
		return true;

	bool bPasses = false;

	// Ally 필터
	if ((Config.TargetFilter & static_cast<int32>(ETargetFilter::Ally)) != 0)
	{
		bPasses |= IsAlly(Target);
	}

	// Enemy 필터
	if ((Config.TargetFilter & static_cast<int32>(ETargetFilter::Enemy)) != 0)
	{
		bPasses |= IsEnemy(Target);
	}

	// Self 필터
	if ((Config.TargetFilter & static_cast<int32>(ETargetFilter::Self)) != 0)
	{
		bPasses |= IsSelf(Target);
	}

	// Minion 필터
	if ((Config.TargetFilter & static_cast<int32>(ETargetFilter::Minion)) != 0)
	{
		bPasses |= IsMinion(Target);
	}

	// Champion 필터
	if ((Config.TargetFilter & static_cast<int32>(ETargetFilter::Champion)) != 0)
	{
		bPasses |= IsChampion(Target);
	}

	// Structure 필터
	if ((Config.TargetFilter & static_cast<int32>(ETargetFilter::Structure)) != 0)
	{
		bPasses |= IsStructure(Target);
	}

	// Neutral 필터
	if ((Config.TargetFilter & static_cast<int32>(ETargetFilter::Neutral)) != 0)
	{
		bPasses |= IsNeutral(Target);
	}

	return bPasses;
}

bool UTargetingStrategy::IsInRange(AActor* Target) const
{
	if (!Target || !OwningActor)
		return false;

	if (Config.Range <= 0.0f)
		return true;

	float Distance = FVector::Dist(OwningActor->GetActorLocation(), Target->GetActorLocation());
	return Distance <= Config.Range;
}

bool UTargetingStrategy::HasLineOfSight(AActor* Target) const
{
	if (!Target || !OwningActor)
		return false;

	UWorld* World = GetWorld();
	if (!World)
		return false;

	FVector Start = OwningActor->GetActorLocation();
	FVector End = Target->GetActorLocation();

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwningActor);
	QueryParams.AddIgnoredActor(Target);

	// 벽에 막히는지 체크
	bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		QueryParams
	);

	return !bHit;
}

bool UTargetingStrategy::IsAlly(AActor* Target) const
{
	// TODO: Team Component나 Interface로 팀 정보 가져오기
	if (!Target || !OwningActor)
		return false;

	// 자신이면 아군
	if (Target == OwningActor)
		return true;

	// 같은 태그를 가진 액터들끼리는 아군 (Enemy끼리, Player끼리)
	// Enemy 태그가 있으면 Enemy 팀
	bool bOwnerIsEnemy = OwningActor->Tags.Contains(FName("Enemy"));
	bool bTargetIsEnemy = Target->Tags.Contains(FName("Enemy"));

	if (bOwnerIsEnemy && bTargetIsEnemy)
		return true; // Enemy끼리는 아군

	// Player/Champion 태그가 있으면 Player 팀
	bool bOwnerIsPlayer = OwningActor->Tags.Contains(FName("Player")) || OwningActor->Tags.Contains(FName("Champion"));
	bool bTargetIsPlayer = Target->Tags.Contains(FName("Player")) || Target->Tags.Contains(FName("Champion"));

	if (bOwnerIsPlayer && bTargetIsPlayer)
		return true; // Player끼리는 아군

	return false;
}

bool UTargetingStrategy::IsEnemy(AActor* Target) const
{
	// TODO: Team Component나 Interface로 팀 정보 가져오기
	if (!Target || !OwningActor)
		return false;

	// 아군이 아니고 자신이 아니면 적
	return !IsAlly(Target) && !IsSelf(Target);
}

bool UTargetingStrategy::IsSelf(AActor* Target) const
{
	return Target == OwningActor;
}

bool UTargetingStrategy::IsMinion(AActor* Target) const
{
	// TODO: Actor의 타입을 확인하는 로직 구현
	// Interface나 Tag, Class 체크 등
	return Target && Target->Tags.Contains("Minion");
}

bool UTargetingStrategy::IsChampion(AActor* Target) const
{
	// TODO: Actor의 타입을 확인하는 로직 구현
	return Target && Target->Tags.Contains("Champion");
}

bool UTargetingStrategy::IsStructure(AActor* Target) const
{
	// TODO: Actor의 타입을 확인하는 로직 구현
	return Target && Target->Tags.Contains("Structure");
}

bool UTargetingStrategy::IsNeutral(AActor* Target) const
{
	// TODO: Actor의 타입을 확인하는 로직 구현
	return Target && Target->Tags.Contains("Neutral");
}

// ============================================
// Geometry Helpers
// ============================================

TArray<AActor*> UTargetingStrategy::GetActorsInSphere(const FVector& Center, float Radius) const
{
	TArray<AActor*> FoundActors;

	UWorld* World = GetWorld();
	if (!World)
		return FoundActors;

	// OverlapMulti로 구체 범위 내 액터 수집
	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwningActor);

	World->OverlapMultiByChannel(
		Overlaps,
		Center,
		FQuat::Identity,
		ECC_Pawn,
		Sphere,
		QueryParams
	);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (Actor)
		{
			// Only collect actors with gameplay-relevant tags
			// Ignore terrain/background objects without tags
			bool bHasGameplayTag = Actor->Tags.Contains(FName("Enemy")) ||
			                       Actor->Tags.Contains(FName("Structure")) ||
			                       Actor->Tags.Contains(FName("Character")) ||
			                       Actor->Tags.Contains(FName("Minion")) ||
			                       Actor->Tags.Contains(FName("Champion")) ||
			                       Actor->Tags.Contains(FName("Neutral")) ||
			                       Actor->Tags.Contains(FName("Player"));

			if (bHasGameplayTag)
			{
				FoundActors.Add(Actor);
			}
		}
	}

	return FoundActors;
}

TArray<AActor*> UTargetingStrategy::GetActorsInBox(const FVector& Start, const FVector& End, float Width) const
{
	TArray<AActor*> FoundActors;

	UWorld* World = GetWorld();
	if (!World)
		return FoundActors;

	FVector Direction = (End - Start).GetSafeNormal();
	float Length = FVector::Dist(Start, End);
	FVector Center = Start + Direction * (Length * 0.5f);

	// 박스 크기 계산
	FVector HalfExtent = FVector(Width * 0.5f, Width * 0.5f, Length * 0.5f);
	FRotator Rotation = Direction.Rotation();

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Box = FCollisionShape::MakeBox(HalfExtent);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwningActor);

	World->OverlapMultiByChannel(
		Overlaps,
		Center,
		Rotation.Quaternion(),
		ECC_Pawn,
		Box,
		QueryParams
	);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (Actor)
		{
			// Only collect actors with gameplay-relevant tags
			// Ignore terrain/background objects without tags
			bool bHasGameplayTag = Actor->Tags.Contains(FName("Enemy")) ||
			                       Actor->Tags.Contains(FName("Structure")) ||
			                       Actor->Tags.Contains(FName("Character")) ||
			                       Actor->Tags.Contains(FName("Minion")) ||
			                       Actor->Tags.Contains(FName("Champion")) ||
			                       Actor->Tags.Contains(FName("Neutral")) ||
			                       Actor->Tags.Contains(FName("Player"));

			if (bHasGameplayTag)
			{
				FoundActors.Add(Actor);
			}
		}
	}

	return FoundActors;
}

TArray<AActor*> UTargetingStrategy::GetActorsInCone(const FVector& Origin, const FVector& Direction, float Range, float AngleDegrees) const
{
	TArray<AActor*> FoundActors;

	// 먼저 구체 범위로 수집
	TArray<AActor*> PotentialTargets = GetActorsInSphere(Origin, Range);

	// 부채꼴 각도로 필터링
	float HalfAngleRad = FMath::DegreesToRadians(AngleDegrees * 0.5f);
	FVector NormalizedDirection = Direction.GetSafeNormal();

	for (AActor* Actor : PotentialTargets)
	{
		FVector ToTarget = (Actor->GetActorLocation() - Origin).GetSafeNormal();
		float DotProduct = FVector::DotProduct(NormalizedDirection, ToTarget);
		float Angle = FMath::Acos(DotProduct);

		if (Angle <= HalfAngleRad)
		{
			FoundActors.Add(Actor);
		}
	}

	return FoundActors;
}

// ============================================
// Utility
// ============================================

TArray<AActor*> UTargetingStrategy::LimitTargetCount(TArray<AActor*> Targets, const FVector& ReferencePoint) const
{
	if (Targets.Num() <= Config.MaxTargets)
		return Targets;

	// 거리순 정렬
	SortByDistance(Targets, ReferencePoint);

	// MaxTargets만큼만 반환
	TArray<AActor*> LimitedTargets;
	for (int32 i = 0; i < Config.MaxTargets && i < Targets.Num(); i++)
	{
		LimitedTargets.Add(Targets[i]);
	}

	return LimitedTargets;
}

void UTargetingStrategy::SortByDistance(TArray<AActor*>& Targets, const FVector& ReferencePoint) const
{
	Targets.Sort([ReferencePoint](const AActor& A, const AActor& B)
	{
		float DistA = FVector::DistSquared(A.GetActorLocation(), ReferencePoint);
		float DistB = FVector::DistSquared(B.GetActorLocation(), ReferencePoint);
		return DistA < DistB;
	});
}

UWorld* UTargetingStrategy::GetWorld() const
{
	if (OwningActor)
	{
		return OwningActor->GetWorld();
	}
	return nullptr;
}
