// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/AttributeComponent.h"

UAttributeComponent::UAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}


void UAttributeComponent::BeginPlay()
{
	Super::BeginPlay();

	
}

void UAttributeComponent::ReceiveDamage(float Damage)
{
	if (Damage <= Shield)
	{
		Shield = FMath::Clamp(Shield - Damage, 0.f, MaxShield);
	}
	else
	{
		Damage -= Shield;
		Shield = 0;
		Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	}
}

float UAttributeComponent::GetHealthPercent()
{
	return Health / MaxHealth;
}

float UAttributeComponent::GetShieldPercent()
{
	return Shield / MaxShield;
}

float UAttributeComponent::GetEndurancePercent()
{
	return Endurance / MaxEndurance;
}

bool UAttributeComponent::IsAlive()
{
	return Health > 0.f;
}

void UAttributeComponent::AddGold(int32 AmountOfGold)
{
	Gold += AmountOfGold;
}

void UAttributeComponent::AddHealth(int32 AmountOfHealth)
{
	Health = FMath::Clamp(Health + AmountOfHealth, 0.f, MaxHealth);
}

void UAttributeComponent::AddShield(int32 AmountOfShield)
{
	Shield = FMath::Clamp(Shield + AmountOfShield, 0.f, MaxShield);
}

bool UAttributeComponent::TryJump()
{
	if (Endurance < JumpCostEndurance) return false;
	Endurance -= JumpCostEndurance;
	return true;
}

bool UAttributeComponent::TryAttack()
{
	if (Endurance < AttackCostEndurance) return false;
	Endurance -= AttackCostEndurance;
	return true;
}

void UAttributeComponent::CostEndurance(float DeltaTime)
{
	Endurance = FMath::Max(0, Endurance - DeltaTime * RunCostEndurance);
}

void UAttributeComponent::AddEndurance(float DeltaTime)
{
	Endurance = FMath::Min(MaxEndurance, Endurance + DeltaTime * RecoveryEndurance);
}

void UAttributeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

