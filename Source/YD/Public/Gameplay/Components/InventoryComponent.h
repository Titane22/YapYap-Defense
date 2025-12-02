// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class UItemData;

USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	UItemData* Item;

	UPROPERTY()
	int32 StackCount;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class YD_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UInventoryComponent();

	/** Equip an item to the specified slot */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void EquipItem(UItemData* Item, int32 StackIndex);

	/** Unequip an item from the specified slot */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UnEquipItem(int32 StackIndex);

	/** Get item in specific slot */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	UItemData* GetItemInSlot(int32 StackIndex) const
	{
		return InventorySlots.IsValidIndex(StackIndex) ? InventorySlots[StackIndex].Item : nullptr;
	}

protected:
	/** Inventory slots (default 6 like LoL) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FInventorySlot> InventorySlots;

};
