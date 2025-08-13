// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SettlementWidget.generated.h"

/**
 * 
 */
UCLASS()
class SUPERNOVA_API USettlementWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;
	void SetTextBySuccess(bool bSuccess);

protected:
	UFUNCTION()
	void OnRestartGameClicked();

	UFUNCTION()
	void OnReturnToManuClicked();
private:
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* MainText;

	UPROPERTY(meta = (BindWidget))
	class UButton* ReturnToManu;

	UPROPERTY(meta = (BindWidget))
	UButton* RestartGame;

	//UPROPERTY(meta = (BindWidget))
	//class UImage* Background;
};
