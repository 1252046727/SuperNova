// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SuperNovaPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class SUPERNOVA_API ASuperNovaPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	virtual void Tick(float DeltaTime) override;

	void SetHUDEndurance(float Percent);
	void SetHUDHealth(float Percent);
	void SetHUDHealthText(float Health,float MaxHealth);
	void SetHUDShield(float Percent);
	void SetHUDShieldText(float Shield, float MaxShield);
	void SetHUDGold(int32 Gold);
	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountdown(float CountdownTime);
	void ShowSettlementWidget(bool bSuccess);
	void HideSettlementWidget();
protected:
	virtual void BeginPlay() override;
	void SetHUDTime();

private:
	UPROPERTY()
	class ASuperNovaHUD* SuperNovaHUD;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class USettlementWidget> SettlementWidgetClass;

	UPROPERTY()
	TObjectPtr<USettlementWidget> SettlementWidget;

	float MatchTime = 120.f;
	uint32 CountdownInt = 0;
	float LevelStartingTime;
};
