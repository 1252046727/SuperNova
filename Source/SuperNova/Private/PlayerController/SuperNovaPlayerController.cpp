// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/SuperNovaPlayerController.h"
#include "HUD/SuperNovaHUD.h"
#include "HUD/CharacterOverlay.h"
#include "HUD/SettlementWidget.h"
#include "GameMode/SuperNovaGameMode.h"
#include "Kismet/GameplayStatics.h"

void ASuperNovaPlayerController::BeginPlay()
{
	Super::BeginPlay();

	SuperNovaHUD = Cast<ASuperNovaHUD>(GetHUD());

	ASuperNovaGameMode* GameMode = Cast<ASuperNovaGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		MatchTime = GameMode->MatchTime;
		LevelStartingTime = GameMode->LevelStartingTime;
	}
}

void ASuperNovaPlayerController::SetHUDTime()
{
	uint32 SecondsLeft = FMath::CeilToInt(MatchTime + LevelStartingTime - GetWorld()->GetTimeSeconds());
	if (CountdownInt != SecondsLeft)
	{
		SetHUDMatchCountdown(MatchTime + LevelStartingTime - GetWorld()->GetTimeSeconds());
	}

	CountdownInt = SecondsLeft;
}

void ASuperNovaPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();
}

void ASuperNovaPlayerController::SetHUDEndurance(float Percent)
{
	if (SuperNovaHUD && SuperNovaHUD->CharacterOverlay)
	{
		SuperNovaHUD->CharacterOverlay->SetEndurance(Percent);
	}
}

void ASuperNovaPlayerController::SetHUDHealth(float Percent)
{
	if (SuperNovaHUD && SuperNovaHUD->CharacterOverlay)
	{
		SuperNovaHUD->CharacterOverlay->SetHealth(Percent);
	}
}

void ASuperNovaPlayerController::SetHUDHealthText(float Health, float MaxHealth)
{
	if (SuperNovaHUD && SuperNovaHUD->CharacterOverlay)
	{
		SuperNovaHUD->CharacterOverlay->SetHealthText(Health, MaxHealth);
	}
}

void ASuperNovaPlayerController::SetHUDShield(float Percent)
{
	if (SuperNovaHUD && SuperNovaHUD->CharacterOverlay)
	{
		SuperNovaHUD->CharacterOverlay->SetShield(Percent);
	}
}

void ASuperNovaPlayerController::SetHUDShieldText(float Shield, float MaxShield)
{
	if (SuperNovaHUD && SuperNovaHUD->CharacterOverlay)
	{
		SuperNovaHUD->CharacterOverlay->SetShieldText(Shield, MaxShield);
	}
}

void ASuperNovaPlayerController::SetHUDGold(int32 Gold)
{
	if (SuperNovaHUD && SuperNovaHUD->CharacterOverlay)
	{
		SuperNovaHUD->CharacterOverlay->SetGold(Gold);
	}
}

void ASuperNovaPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	if (SuperNovaHUD && SuperNovaHUD->CharacterOverlay)
	{
		SuperNovaHUD->CharacterOverlay->SetWeaponAmmo(Ammo);
	}
}

void ASuperNovaPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	if (SuperNovaHUD && SuperNovaHUD->CharacterOverlay)
	{
		SuperNovaHUD->CharacterOverlay->SetCarriedAmmo(Ammo);
	}
}

void ASuperNovaPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	if (SuperNovaHUD && SuperNovaHUD->CharacterOverlay)
	{
		SuperNovaHUD->CharacterOverlay->SetMatchCountdown(CountdownTime);
	}
}

void ASuperNovaPlayerController::ShowSettlementWidget(bool bSuccess)
{
	if (SettlementWidgetClass != nullptr)
	{
		//暂停
		SetPause(true);
		//只能操控ui界面
		SetInputMode(FInputModeUIOnly());
		//显示鼠标光标
		bShowMouseCursor = true;
		SettlementWidget = CreateWidget<USettlementWidget>(this, SettlementWidgetClass);
		if (SettlementWidget != nullptr)
		{
			SettlementWidget->SetTextBySuccess(bSuccess);
			SettlementWidget->AddToViewport();
		}
	}
}

void ASuperNovaPlayerController::HideSettlementWidget()
{
	SettlementWidget->RemoveFromParent();
	SettlementWidget->Destruct();
	SetPause(false);
	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}


