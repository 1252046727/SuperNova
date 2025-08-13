// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttributeComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SUPERNOVA_API UAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAttributeComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	
private:
	// Current Health
	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	float Health;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	float MaxHealth;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	float Shield;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	float MaxShield;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	float Endurance;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	float MaxEndurance;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	float JumpCostEndurance;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	float RunCostEndurance;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	float AttackCostEndurance;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	float RecoveryEndurance;

	UPROPERTY(EditAnywhere, Category = "Actor Attributes")
	int32 Gold;
public:
	void ReceiveDamage(float Damage);
	float GetHealthPercent();
	float GetShieldPercent();
	float GetEndurancePercent();
	bool IsAlive();
	void AddGold(int32 AmountOfGold);
	void AddHealth(int32 AmountOfHealth);
	void AddShield(int32 AmountOfShield);
	bool TryJump();
	bool TryAttack();
	void CostEndurance(float DeltaTime);
	void AddEndurance(float DeltaTime);
	FORCEINLINE int32 GetGold() const { return Gold; }
	FORCEINLINE float GetEndurance() { return Endurance; }
	FORCEINLINE float GetMaxEndurance() { return MaxEndurance; }
	FORCEINLINE float GetHealth() { return Health; }
	FORCEINLINE float GetMaxHealth() { return MaxHealth; }
	FORCEINLINE float GetShield() { return Shield; }
	FORCEINLINE float GetMaxShield() { return MaxShield; }
};
