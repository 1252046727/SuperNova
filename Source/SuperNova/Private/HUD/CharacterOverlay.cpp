// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UCharacterOverlay::SetEndurance(float Percent)
{
	if (EnduranceBar)
	{
		EnduranceBar->SetPercent(Percent);
	}
}

void UCharacterOverlay::SetHealth(float Percent)
{
	if (HealthBar)
	{
		HealthBar->SetPercent(Percent);
	}
}

void UCharacterOverlay::SetHealthText(float Health, float MaxHealth)
{
	if (HealthText)
	{
		FString Text = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		HealthText->SetText(FText::FromString(Text));
	}
}

void UCharacterOverlay::SetShield(float Percent)
{
	if (ShieldBar)
	{
		ShieldBar->SetPercent(Percent);
	}
}

void UCharacterOverlay::SetShieldText(float Shield, float MaxShield)
{
	if (ShieldText)
	{
		FString Text = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		ShieldText->SetText(FText::FromString(Text));
	}
}

void UCharacterOverlay::SetGold(int32 Gold)
{
	if (GoldText)
	{
		const FString String = FString::Printf(TEXT("%d"), Gold);
		const FText Text = FText::FromString(String);
		GoldText->SetText(Text);
	}
}

void UCharacterOverlay::SetWeaponAmmo(int32 Ammo)
{
	if (WeaponAmmoAmount)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void UCharacterOverlay::SetCarriedAmmo(int32 Ammo)
{
	if (CarriedAmmoAmount)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
}

void UCharacterOverlay::SetMatchCountdown(float CountdownTime)
{
	if (MatchCountdownText)
	{
		//状态转换时未来得及更新会出现短暂负值
		if (CountdownTime < 0.f)
		{
			MatchCountdownText->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}
