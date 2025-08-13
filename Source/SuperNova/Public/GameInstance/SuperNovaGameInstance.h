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
    // ��ͼ�ߴ������0 С��1 �У�2 ��
    UPROPERTY(BlueprintReadWrite, Category = "GameParams")
    int32 MapSize = 0;

    // ��Ϸģʽ��0 һ�㣬1 ����
    UPROPERTY(BlueprintReadWrite, Category = "GameParams")
    int32 Mode = 0;

    // ���ò���
    UFUNCTION(BlueprintCallable)
    void ResetParams()
    {
        MapSize = 0;
        Mode = 0;
    }
};
