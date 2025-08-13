// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/SettlementWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerController/SuperNovaPlayerController.h"
#include "Components/Image.h"

void USettlementWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (ReturnToManu != nullptr)
	{
		ReturnToManu->OnClicked.AddDynamic(this, &USettlementWidget::OnReturnToManuClicked);
	}
	if (RestartGame != nullptr)
	{
		RestartGame->OnClicked.AddDynamic(this, &USettlementWidget::OnRestartGameClicked);
	}
}

void USettlementWidget::SetTextBySuccess(bool bSuccess)
{
	if (bSuccess)
	{
		MainText->SetText(FText::FromString(TEXT("Mission Success")));
		MainText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
	}
	else
	{
		MainText->SetText(FText::FromString(TEXT("Mission Fail")));
		MainText->SetColorAndOpacity(FSlateColor(FLinearColor::Green));
	}
}

//void USettlementWidget::SetBackgroundImage(UTexture2D* NewTexture)
//{
//	if (Background)
//	{
//		Background->SetBrushFromTexture(NewTexture);
//	}
//}

void USettlementWidget::OnRestartGameClicked()
{
	ASuperNovaPlayerController* SuperNovaPlayerController = Cast<ASuperNovaPlayerController>(GetOwningPlayer());
	if (SuperNovaPlayerController != nullptr)
	{
		SuperNovaPlayerController->HideSettlementWidget();
	}
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}

void USettlementWidget::OnReturnToManuClicked()
{
	UGameplayStatics::OpenLevel(this, TEXT("Menu"));
}
