// Fill out your copyright notice in the Description page of Project Settings.

#include "Gameplay/Animation/AnimNotify_SpawnProjectile.h"
#include "Gameplay/Components/AbilityComponent.h"

void UAnimNotify_SpawnProjectile::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
		return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
		return;

	// Find AbilityComponent and spawn projectile
	UAbilityComponent* AbilityComponent = Owner->FindComponentByClass<UAbilityComponent>();
	if (AbilityComponent)
	{
		// Get spawn location from socket
		FVector SpawnLocation = MeshComp->GetSocketLocation(SpawnSocketName);
		FRotator SpawnRotation = Owner->GetActorRotation();

		AbilityComponent->SpawnProjectileFromNotify(SpawnLocation, SpawnRotation);
		UE_LOG(LogTemp, Log, TEXT("AnimNotify_SpawnProjectile triggered for %s at socket %s"), *Owner->GetName(), *SpawnSocketName.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AnimNotify_SpawnProjectile: No AbilityComponent found on %s"), *Owner->GetName());
	}
}
