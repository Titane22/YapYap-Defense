// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CharacterStatComponent.generated.h"

USTRUCT(BlueprintType)
struct FStatModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ModifierName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HealthModifier = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackDamageModifier = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ArmorModifier = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MoveSpeedModifier = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackSpeedModifier = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Duration = -1.f; // -1 = permanent

	float RemainingTime = 0.f;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, CurrentHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatsUpdated, FStatModifier, Modifier);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class YD_API UCharacterStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCharacterStatComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
public:	
	// Base Stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Base")
	float BaseMaxHealth = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Base")
	float BaseAttackDamage = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Base")
	float BaseAbilityPower = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Base")
	float BaseArmor = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Base")
	float BaseMoveSpeed = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Base")
	float BaseAttackSpeed = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Base")
	float BaseAttackRange = 150.f;

	// Current Stats
	UPROPERTY(BlueprintReadOnly, Category = "Stats|Current")
	float CurrentHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|Current")
	float CurrentMaxHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|Current")
	float CurrentAttackDamage;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|Current")
	float CurrentAbilityPower;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|Current")
	float CurrentArmor;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|Current")
	float CurrentMoveSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|Current")
	float CurrentAttackSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|Current")
	float CurrentAttackRange;
		
	// Stat Modifiers
	UPROPERTY(BlueprintReadOnly, Category = "Stats|Modifiers")
	TArray<FStatModifier> StatModifiers;

	// Delegates
	UPROPERTY(BlueprintAssignable, Category = "Stats|Events")
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Stats|Events")
	FOnDeath OnDeath;

	UPROPERTY(BlueprintAssignable, Category = "Stats|Events")
	FOnStatsUpdated OnStatsUpdated;	

public:
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void TakeDamage(float Damage, AActor* DamageDealer);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void Heal(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void AddStatModifier(FStatModifier Modifier);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RemoveStatModifier(FName ModifierName);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RecalculateStats();

	UFUNCTION(BlueprintPure, Category = "Stats")
	bool IsAlive() const { return CurrentHealth > 0.f; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetHealthPercent() const { return CurrentHealth > 0 ? CurrentHealth / CurrentMaxHealth : 0.f; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetCurrentAttackRange() const { return CurrentAttackRange; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetCurrentAttackDamage() const { return CurrentAttackDamage; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetCurrentAbilityPower() const { return CurrentAbilityPower; }
	
	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetCurrentAttackSpeed() const { return CurrentAttackSpeed; }

	void ResetStats();
	
private:
	void UpdateTimedModifiers(float DeltaTime);
	void Die();
};
