// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

/**
 * 
 */
UCLASS()
class SUPERNOVA_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetEndurance(float Percent);
	void SetHealth(float Percent);
	void SetHealthText(float Health,float MaxHealth);
	void SetShield(float Percent);
	void SetShieldText(float Shield, float MaxShield);
	void SetGold(int32 Gold);
	void SetWeaponAmmo(int32 Ammo);
	void SetCarriedAmmo(int32 Ammo);
	void SetMatchCountdown(float CountdownTime);

private:
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* EnduranceBar;

	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* HealthText;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* ShieldBar;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ShieldText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* GoldText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* WeaponAmmoAmount;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CarriedAmmoAmount;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MatchCountdownText;
};
