// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile_Base.generated.h"

class UArrowComponent;
class UBoxComponent;
class UProjectileMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectileImpact, AActor*, OtherActor, FHitResult, Hit);

UCLASS()
class YD_API AProjectile_Base : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectile_Base();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Rotate to Target if Valid
	void RotateToTarget();

	void PlaySpawnSound();

	void PlayImpactSound(FVector Location);

	void SpawnImpactEffect(FVector Location);

	UFUNCTION()
	void OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

public:
	UPROPERTY(BlueprintAssignable, Category = "Projectile")
	FOnProjectileImpact OnProjectileImpact;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UProjectileMovementComponent* ProjectileMovement;
protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* ProjectileCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UArrowComponent* ProjectileArrow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Settings")
	float Speed = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Settings")
	float Gravity = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Settings")
	bool bIsHoming = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Settings")
	float HeightAboveGround = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Settings")
	float HeightAdjustmentSpeed = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Settings")
	float HeightTolerance = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Target")
	AActor* Target;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Sound")
	USoundBase* ImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Sound")
	USoundBase* SpawnSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Effect")
	UParticleSystem* ImpactEffect;
};
