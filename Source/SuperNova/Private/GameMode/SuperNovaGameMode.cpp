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
	//GetTimeSeconds�ڲ˵������������Ϸ��ʱ��Ϳ�ʼ��ʱ�ˣ�������Ҫ�õ�����BlasterMap��������ʱ��
	LevelStartingTime = GetWorld()->GetTimeSeconds();
}
