// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/MenuWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "GameInstance/SuperNovaGameInstance.h"

void UMenuWidget::MenuSetup(int32 NumOfMapSize, int32 NumOfMode)
{
	MapSize = NumOfMapSize;
	Mode = NumOfMode;
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = true;
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			//输入模式 只关注UI
			FInputModeUIOnly InputModeData;//允许玩家同时与游戏世界（Game）和用户界面（UI）交互
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}
}

bool UMenuWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	if (BeginButton)
	{
		BeginButton->OnClicked.AddDynamic(this, &ThisClass::BeginButtonClicked);
	}

	return true;
}

void UMenuWidget::NativeDestruct()
{
	MenuTearDown();

	Super::NativeDestruct();
}

void UMenuWidget::BeginButtonClicked()
{
	USuperNovaGameInstance* GameInstance = Cast<USuperNovaGameInstance>(GetWorld()->GetGameInstance());
	if (GameInstance)
	{
		GameInstance->MapSize = this->MapSize;
		GameInstance->Mode = this->Mode;

		UGameplayStatics::OpenLevel(this, TEXT("GameStartupMap"));
	}
}

void UMenuWidget::MenuTearDown()
{
	RemoveFromParent();
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(false);
		}
	}
}
