// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/SuperNovaGameMode.h"
#include "PlayerController/SuperNovaPlayerController.h"

void ASuperNovaGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CountdownTime =   MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
	if (CountdownTime <= 0.f)
	{
		ASuperNovaPlayerController* SuperNovaPlayerController = Cast<ASuperNovaPlayerController>(GetWorld()->GetFirstPlayerController());
		if (SuperNovaPlayerController != nullptr)
		{
			SuperNovaPlayerController->ShowSettlementWidget(false);
		}
	}
}

void ASuperNovaGameMode::PlayerEliminated(ASuperNovaCharacter* ElimmedCharacter, ASuperNovaPlayerController* VictimController)
{

}

void ASuperNovaGameMode::BeginPlay()
{
	//GetTimeSeconds在菜单栏点击启动游戏的时候就开始计时了，所以需要得到进入BlasterMap的所花的时间
	LevelStartingTime = GetWorld()->GetTimeSeconds();
}
