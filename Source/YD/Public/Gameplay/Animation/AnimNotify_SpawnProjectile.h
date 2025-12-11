// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_SpawnProjectile.generated.h"

/**
 * Animation Notify for spawning projectiles at specific animation frames
 * Used for ranged attacks (bow, spells, etc.)
 */
UCLASS()
class YD_API UAnimNotify_SpawnProjectile : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	/** Socket name to spawn projectile from (e.g., "hand_r_socket") */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	FName SpawnSocketName = TEXT("hand_r");

#if WITH_EDITOR
	virtual FString GetNotifyName_Implementation() const override
	{
		return TEXT("Spawn Projectile");
	}
#endif
};
