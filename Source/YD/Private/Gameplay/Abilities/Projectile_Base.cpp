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
	PrimaryActorTick.bCanEverTick = true;

	ProjectileCollision = CreateDefaultSubobject<UBoxComponent>(FName("ProjectileCollision"));
	ProjectileCollision->SetBoxExtent(FVector(32.f, 32.f, 32.f));
	ProjectileCollision->BodyInstance.SetCollisionProfileName("BlockAll");
	ProjectileCollision->BodyInstance.bNotifyRigidBodyCollision = true;
	ProjectileCollision->SetGenerateOverlapEvents(false);
	ProjectileCollision->OnComponentHit.AddDynamic(this, &AProjectile_Base::OnComponentHit);
	RootComponent = ProjectileCollision;

	UE_LOG(LogTemp, Warning, TEXT("Projectile_Base constructor - Collision set up with BlockAll"));

	ProjectileArrow = CreateDefaultSubobject<UArrowComponent>(FName("ProjectileArrow"));
	ProjectileArrow->SetupAttachment(RootComponent);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(FName("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Mesh doesn't collide, only the collision box

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

	UE_LOG(LogTemp, Warning, TEXT("=== Projectile BeginPlay ==="));
	UE_LOG(LogTemp, Log, TEXT("Collision Enabled: %d (3=QueryAndPhysics)"), (int32)ProjectileCollision->GetCollisionEnabled());
	UE_LOG(LogTemp, Log, TEXT("Notify Rigid Body Collision: %s"),
		ProjectileCollision->BodyInstance.bNotifyRigidBodyCollision ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Log, TEXT("Collision Profile: %s"), *ProjectileCollision->GetCollisionProfileName().ToString());

	// Set initial velocity from spawn rotation
	if (ProjectileMovement)
	{
		// If we have a target, aim at it
		if (Target)
		{
			RotateToTarget();
		}
		else
		{
			// Otherwise use the forward direction from spawn rotation
			FVector ForwardDirection = GetActorForwardVector();
			ProjectileMovement->Velocity = ForwardDirection * ProjectileMovement->InitialSpeed;

			UE_LOG(LogTemp, Log, TEXT("Projectile initial velocity: %s (Speed: %.1f)"),
				*ProjectileMovement->Velocity.ToString(), ProjectileMovement->InitialSpeed);
		}
	}

	PlaySpawnSound();

	AActor* ProjectileOwner = GetInstigator();
	if (!ProjectileOwner)
		ProjectileOwner = GetOwner();

	if (ProjectileOwner)
	{
		// Multiple ways to ignore owner collision
		ProjectileCollision->IgnoreActorWhenMoving(ProjectileOwner, true);
		ProjectileCollision->MoveIgnoreActors.Add(ProjectileOwner);

		UE_LOG(LogTemp, Log, TEXT("Ignoring collision with owner: %s"), *ProjectileOwner->GetName());
	}
}

// Called every frame
void AProjectile_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector CurrentLocation = GetActorLocation();
	FVector BoxExtent = ProjectileCollision->GetScaledBoxExtent();

	TArray<FVector> TraceOffsets;
	TraceOffsets.Add(FVector(BoxExtent.X * 0.8f, BoxExtent.Y * 0.8f, 0.f));
	TraceOffsets.Add(FVector(BoxExtent.X * 0.8f, -BoxExtent.Y * 0.8f, 0.f));
	TraceOffsets.Add(FVector(-BoxExtent.X * 0.8f, BoxExtent.Y * 0.8f, 0.f));
	TraceOffsets.Add(FVector(-BoxExtent.X * 0.8f, -BoxExtent.Y * 0.8f, 0.f));

	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);
	if (GetOwner())
		TraceParams.AddIgnoredActor(GetOwner());

	TArray<float> GroundHeights;
	bool bAllTracesHit = true;

	for (const FVector& Offset : TraceOffsets)
	{
		FVector TraceLocation = CurrentLocation + Offset;
		FVector TraceStart = TraceLocation + FVector(0.f, 0.f, 500.f);
		FVector TraceEnd = TraceLocation - FVector(0.f, 0.f, 2000.f);

		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility, TraceParams))
		{
			GroundHeights.Add(Hit.Location.Z);
		}
		else
		{
			bAllTracesHit = false;
			break;
		}
	}

	if (bAllTracesHit && GroundHeights.Num() == TraceOffsets.Num())
	{
		// Find min and max heights
		float MinHeight = GroundHeights[0];
		float MaxHeight = GroundHeights[0];
		float AverageHeight = 0.f;

		for (float Height : GroundHeights)
		{
			MinHeight = FMath::Min(MinHeight, Height);
			MaxHeight = FMath::Max(MaxHeight, Height);
			AverageHeight += Height;
		}
		AverageHeight /= GroundHeights.Num();

		if ((MaxHeight - MinHeight) <= HeightTolerance)
		{
			// All heights are similar - we're on flat ground or completely airborne
			// Use average height for smooth result
			float DesiredZ = AverageHeight + HeightAboveGround;

			// Smoothly interpolate from current height to desired height
			float NewZ = FMath::FInterpTo(CurrentLocation.Z, DesiredZ, DeltaTime, HeightAdjustmentSpeed);

			FVector NewLocation = CurrentLocation;
			NewLocation.Z = NewZ;

			SetActorLocation(NewLocation);
		}
		// If heights differ too much, we're over a cliff edge - don't adjust
	}
	
}

void AProjectile_Base::RotateToTarget()
{
	if (!Target || !ProjectileMovement)
		return;

	FVector RotateVelocity = UKismetMathLibrary::GetDirectionUnitVector(GetActorLocation(), Target->GetActorLocation()) * ProjectileMovement->InitialSpeed;
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
	// Only process collision if the hit actor has specific tags
	// Ignore terrain/background objects that don't have tags
	if (OtherActor)
	{
		bool bShouldProcessHit = OtherActor->Tags.Contains(FName("Enemy")) ||
		                         OtherActor->Tags.Contains(FName("Structure")) ||
		                         OtherActor->Tags.Contains(FName("Character")) ||
		                         OtherActor->Tags.Contains(FName("Destructible"));

		if (!bShouldProcessHit)
		{
			// Ignore collision with terrain/background objects
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("=== Projectile Hit ==="));
	UE_LOG(LogTemp, Log, TEXT("Hit Actor: %s"), OtherActor ? *OtherActor->GetName() : TEXT("None"));
	UE_LOG(LogTemp, Log, TEXT("Hit Location: %s"), *Hit.ImpactPoint.ToString());
	UE_LOG(LogTemp, Log, TEXT("Broadcasting OnProjectileImpact delegate"));

	OnProjectileImpact.Broadcast(OtherActor, Hit);

	FVector ImpactLocation = Hit.ImpactPoint;
	SpawnImpactEffect(ImpactLocation);

	PlayImpactSound(ImpactLocation);

	Destroy();
}
