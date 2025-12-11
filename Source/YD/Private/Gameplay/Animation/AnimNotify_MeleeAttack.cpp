// Fill out your copyright notice in the Description page of Project Settings.

#include "Gameplay/Animation/AnimNotify_MeleeAttack.h"
#include "Gameplay/Components/CombatComponent.h"

void UAnimNotify_MeleeAttack::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
		return;

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
		return;

	// Find CombatComponent and apply melee damage
	UCombatComponent* CombatComponent = Owner->FindComponentByClass<UCombatComponent>();
	if (CombatComponent)
	{
		CombatComponent->ApplyMeleeDamage();
		UE_LOG(LogTemp, Log, TEXT("AnimNotify_MeleeAttack triggered for %s"), *Owner->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AnimNotify_MeleeAttack: No CombatComponent found on %s"), *Owner->GetName());
	}
}
