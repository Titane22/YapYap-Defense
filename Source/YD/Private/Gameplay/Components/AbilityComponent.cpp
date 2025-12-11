// Fill out your copyright notice in the Description page of Project Settings.

#include "Gameplay/Components/AbilityComponent.h"
#include "Gameplay/Data/AbilityData.h"
#include "Gameplay/Data/Character_Data.h"
#include "Gameplay/Data/AbilityTypes.h"
#include "Gameplay/Data/Ability.h"
#include "Gameplay/Data/TargetingStrategy.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Net/UnrealNetwork.h"

UAbilityComponent::UAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UAbilityComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UAbilityComponent, bIsSilenced);
	DOREPLIFETIME(UAbilityComponent, bIsStunned);
	DOREPLIFETIME(UAbilityComponent, bIsRooted);
}

void UAbilityComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeAbilities();
}

void UAbilityComponent::InitializeAbilities()
{
	// Initialize abilities array with 4 slots (Q, W, E, R)
	Abilities.SetNum(4);

	for (int32 i = 0; i < AbilityDataAssets.Num() && i < 4; i++)
	{
		if (UAbilityData* Data = AbilityDataAssets[i])
		{
			UAbility* NewAbility = NewObject<UAbility>(this, UAbility::StaticClass());
			NewAbility->Initialize(Data, GetOwner(), static_cast<EAbilitySlot>(i));
			Abilities[i] = NewAbility;
		}
	}
}

void UAbilityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update cooldowns and check pending executions
	for (UAbility* Ability : Abilities)
	{
		if (Ability)
		{
			Ability->UpdateCooldown(DeltaTime);
			Ability->CheckPendingExecution();
		}
	}
}

UAbility* UAbilityComponent::GetAbility(EAbilitySlot Slot) const
{
	int32 Index = static_cast<int32>(Slot);
	if (Abilities.IsValidIndex(Index))
	{
		return Abilities[Index];
	}
	return nullptr;
}

void UAbilityComponent::LearnAbility(UAbilityData* AbilityData, EAbilitySlot Slot)
{
	if (!AbilityData)
	{
		UE_LOG(LogTemp, Warning, TEXT("LearnAbility: AbilityData is null"));
		return;
	}

	int32 Index = static_cast<int32>(Slot);

	// Ensure array is large enough
	if (Abilities.Num() < 4)
	{
		Abilities.SetNum(4);
	}

	// Create new ability instance
	UAbility* NewAbility = NewObject<UAbility>(this, UAbility::StaticClass());
	NewAbility->Initialize(AbilityData, GetOwner(), Slot);
	Abilities[Index] = NewAbility;

	UE_LOG(LogTemp, Log, TEXT("Learned ability %s in slot %d"), *AbilityData->AbilityName.ToString(), Index);
}

void UAbilityComponent::ExecuteAbility(EAbilitySlot Slot, const FAbilityTargetData& TargetData)
{
	UAbility* Ability = GetAbility(Slot);
	if (!Ability)
	{
		UE_LOG(LogTemp, Warning, TEXT("ExecuteAbility: No ability in slot %d"), static_cast<int32>(Slot));
		return;
	}

	// Set current casting ability for AnimNotify to use
	CurrentCastingAbility = Ability;

	Ability->Execute(TargetData);

	// Note: CurrentCastingAbility will be cleared after ability finishes
	// For now, it stays set until next ability is cast
}

void UAbilityComponent::SpawnProjectileFromNotify(const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
	if (!CurrentCastingAbility)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnProjectileFromNotify: No CurrentCastingAbility set!"));
		return;
	}

	// Delegate to the ability to spawn its projectile
	CurrentCastingAbility->SpawnProjectile(SpawnLocation, SpawnRotation);

	UE_LOG(LogTemp, Log, TEXT("SpawnProjectileFromNotify called for ability: %s"),
		*CurrentCastingAbility->AbilityData->AbilityName.ToString());
}

