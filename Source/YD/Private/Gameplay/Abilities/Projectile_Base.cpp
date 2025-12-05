// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/Abilities/Projectile_Base.h"

#include "Chaos/PBDSuspensionConstraintData.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystem.h"

// Sets default values
AProjectile_Base::AProjectile_Base()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	ProjectileCollision = CreateDefaultSubobject<UBoxComponent>(FName("ProjectileCollision"));
	ProjectileCollision->OnComponentHit.AddDynamic(this, &AProjectile_Base::OnComponentHit);
	RootComponent = ProjectileCollision;

	ProjectileArrow = CreateDefaultSubobject<UArrowComponent>(FName("ProjectileArrow"));
	ProjectileArrow->SetupAttachment(RootComponent);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(FName("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(FName("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->ProjectileGravityScale = Gravity;
	if (bIsHoming && Target)
	{
		ProjectileMovement->HomingTargetComponent = Target->GetRootComponent();
		ProjectileMovement->bIsHomingProjectile = true;
	}
}

// Called when the game starts or when spawned
void AProjectile_Base::BeginPlay()
{
	Super::BeginPlay();

	RotateToTarget();

	PlaySpawnSound();

	AActor* ProjectileOwner = GetInstigator();
	if (!ProjectileOwner)
		ProjectileOwner = GetOwner();

	ProjectileCollision->IgnoreActorWhenMoving(ProjectileOwner, true);
}

void AProjectile_Base::RotateToTarget()
{
	if (!Target)
		return;

	FVector RotateVelocity = UKismetMathLibrary::GetDirectionUnitVector(GetActorLocation(), Target->GetActorLocation()) * Speed;
	ProjectileMovement->Velocity = RotateVelocity;
}

void AProjectile_Base::PlaySpawnSound()
{
	if (!SpawnSound)
		return;

	UGameplayStatics::SpawnSoundAtLocation(
		GetWorld(),
		SpawnSound,
		GetActorLocation()
	);
}

void AProjectile_Base::PlayImpactSound(FVector Location)
{
	if (!ImpactSound)
		return;

	UGameplayStatics::SpawnSoundAtLocation(GetWorld(), ImpactSound, Location);
}

void AProjectile_Base::SpawnImpactEffect(FVector Location)
{
	if (!ImpactEffect)
		return;

	UGameplayStatics::SpawnEmitterAtLocation(
		GetWorld(),
		ImpactEffect,
		Location
	);
}

void AProjectile_Base::OnComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	OnProjectileImpact.Broadcast(OtherActor, Hit);
	
	FVector ImpactLocation = Hit.ImpactPoint;
	SpawnImpactEffect(ImpactLocation);

	PlayImpactSound(ImpactLocation);

	Destroy();
}
