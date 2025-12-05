// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/Abilities/AOE_Base.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values
AAOE_Base::AAOE_Base()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AAOE_Base::BeginPlay()
{
	Super::BeginPlay();

	if (bTriggerOnBeginPlay)
	{
		// TODO: 0.1sec Delay
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(
			TimerHandle,
			this,
			&AAOE_Base::Trigger,
			0.1f,
			false
		);
	}
}

void AAOE_Base::SpawnAOE_Sphere()
{
	FVector SpawnLocation = GetActorLocation();
	if (bTriggerOnBeginPlay)
	{
		// Draw Debug Sphere
		DrawDebugSphere(
			GetWorld(),
			SpawnLocation,
			Radius,
			32,
			FColor::Red,
			false,
			2.f
		);
	}
	
	TArray<AActor*> OverlappedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> IgnoreActors;
	if (bIgnoreInstigator && GetInstigator())
	{
		IgnoreActors.Add(GetInstigator());
	}
	
	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		SpawnLocation,
		Radius,
		ObjectTypes,
		nullptr,
		IgnoreActors,
		OverlappedActors
	);

	for(AActor* Actor : OverlappedActors)
	{
		OnOverlapActor.Broadcast(Actor);
	}
}

void AAOE_Base::Trigger()
{
	SpawnAOE_Sphere();
}

