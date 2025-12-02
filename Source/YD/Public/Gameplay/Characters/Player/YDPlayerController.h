// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "YDPlayerController.generated.h"

class UNiagaraSystem;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class AYDPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AYDPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	/** MappingContext for LoL-style controls */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Right Mouse Button Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* RightMouseButtonAction;

	/** Q Skill Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* QSkillAction;

	/** W Skill Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* WSkillAction;

	/** E Skill Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ESkillAction;

	/** R Skill Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* RSkillAction;

	// Input Callbacks
	void OnRightMouseButtonStarted();
	void OnRightMouseButtonCompleted();
	void OnQSkillTriggered();
	void OnWSkillTriggered();
	void OnESkillTriggered();
	void OnRSkillTriggered();

	// Movement
	void MoveToMouseCursor();
	float FollowTime;
	FVector CachedDestination;
	bool bIsToDestination;

	// Attack and Targeting
	AActor* CurrentTarget;
	void AttackTarget(AActor* Target);
	bool IsEnemy(AActor* Actor);

	// Camera Edge Scrolling
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraScrollSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraScrollBorderThickness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float MaxCameraDistanceFromCharacter;

	// Independent camera actor for LoL-style camera control
	UPROPERTY()
	class AActor* IndependentCamera;

	FVector CameraTargetLocation;

	void HandleCameraEdgeScrolling(float DeltaTime);
	void CenterCameraOnCharacter();

	// Click Effect
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	UNiagaraSystem* ClickEffect;

	void SpawnClickEffect(const FVector& Location);

	// Skill cooldowns
	UPROPERTY(BlueprintReadOnly, Category = "Skills")
	float QCooldown;

	UPROPERTY(BlueprintReadOnly, Category = "Skills")
	float WCooldown;

	UPROPERTY(BlueprintReadOnly, Category = "Skills")
	float ECooldown;

	UPROPERTY(BlueprintReadOnly, Category = "Skills")
	float RCooldown;

	float QLastUsedTime;
	float WLastUsedTime;
	float ELastUsedTime;
	float RLastUsedTime;

	bool CanUseSkill(float LastUsedTime, float Cooldown);
};
