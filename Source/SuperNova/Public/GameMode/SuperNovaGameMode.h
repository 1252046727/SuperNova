// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SuperNovaGameMode.generated.h"

/**
 * 
 */
UCLASS()
class SUPERNOVA_API ASuperNovaGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	virtual void Tick(float DeltaTime) override;
	virtual void PlayerEliminated(class ASuperNovaCharacter* ElimmedCharacter, class ASuperNovaPlayerController* VictimController);

	UPROPERTY(EditDefaultsOnly)
	float MatchTime = 120.f;

	float LevelStartingTime = 0.f;
protected:
	virtual void BeginPlay() override;

private:
	float CountdownTime = 0.f;
};
