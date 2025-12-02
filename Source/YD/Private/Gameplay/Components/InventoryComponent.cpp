// Fill out your copyright notice in the Description page of Project Settings.

#include "Gameplay/Components/InventoryComponent.h"
#include "Gameplay/Data/ItemData.h"
#include "Gameplay/Components/CharacterStatComponent.h"
#include "GameFramework/Actor.h"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// Initialize inventory slots (6 slots like LoL)
	InventorySlots.SetNum(6);
	for (int32 i = 0; i < InventorySlots.Num(); ++i)
	{
		InventorySlots[i].Item = nullptr;
		InventorySlots[i].StackCount = 0;
	}
}

void UInventoryComponent::EquipItem(UItemData* Item, int32 StackIndex)
{
	// Validate item
	if (!Item)
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipItem: Item is null"));
		return;
	}

	// Validate slot index
	if (!InventorySlots.IsValidIndex(StackIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipItem: Invalid slot index %d"), StackIndex);
		return;
	}

	// If slot already has an item, unequip it first
	if (InventorySlots[StackIndex].Item != nullptr)
	{
		UnEquipItem(StackIndex);
	}

	// Equip new item
	InventorySlots[StackIndex].Item = Item;
	InventorySlots[StackIndex].StackCount = 1;

	// Apply stat bonuses to owner
	AActor* Owner = GetOwner();
	if (Owner)
	{
		UCharacterStatComponent* StatComponent = Owner->FindComponentByClass<UCharacterStatComponent>();
		if (StatComponent)
		{
			StatComponent->AddStatModifier(Item->StatBonus);
			UE_LOG(LogTemp, Log, TEXT("Equipped item: %s in slot %d"), *Item->ItemName.ToString(), StackIndex);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("EquipItem: Owner has no CharacterStatComponent"));
		}
	}
}

void UInventoryComponent::UnEquipItem(int32 StackIndex)
{
	// Validate slot index
	if (!InventorySlots.IsValidIndex(StackIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("UnEquipItem: Invalid slot index %d"), StackIndex);
		return;
	}

	// Check if slot has an item
	UItemData* Item = InventorySlots[StackIndex].Item;
	if (!Item)
	{
		UE_LOG(LogTemp, Warning, TEXT("UnEquipItem: Slot %d is empty"), StackIndex);
		return;
	}

	// Remove stat bonuses from owner
	AActor* Owner = GetOwner();
	if (Owner)
	{
		UCharacterStatComponent* StatComponent = Owner->FindComponentByClass<UCharacterStatComponent>();
		if (StatComponent)
		{
			StatComponent->RemoveStatModifier(Item->StatBonus.ModifierName);
			UE_LOG(LogTemp, Log, TEXT("Unequipped item: %s from slot %d"), *Item->ItemName.ToString(), StackIndex);
		}
	}

	// Clear the slot
	InventorySlots[StackIndex].Item = nullptr;
	InventorySlots[StackIndex].StackCount = 0;
}
