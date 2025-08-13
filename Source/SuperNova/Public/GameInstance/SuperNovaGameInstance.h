// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SuperNovaGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class SUPERNOVA_API USuperNovaGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
    // 地图尺寸参数：0 小，1 中，2 大
    UPROPERTY(BlueprintReadWrite, Category = "GameParams")
    int32 MapSize = 0;

    // 游戏模式：0 一般，1 困难
    UPROPERTY(BlueprintReadWrite, Category = "GameParams")
    int32 Mode = 0;

    // 重置参数
    UFUNCTION(BlueprintCallable)
    void ResetParams()
    {
        MapSize = 0;
        Mode = 0;
    }
};
