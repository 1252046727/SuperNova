// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "Portal.generated.h"

/**
 * 
 */
UCLASS()
class SUPERNOVA_API APortal : public AItem
{
	GENERATED_BODY()
public:
	APortal();

	void ShowNoMoneyWidget(bool bSeen);

	TArray<FVector> PortalLocation;
protected:
	virtual void BeginPlay() override;
	virtual void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) override;

	UPROPERTY(VisibleAnywhere)
	class UNiagaraComponent* PortalEffectComponent;

	UPROPERTY(VisibleAnywhere)
	class UWidgetComponent* NoMoneyWidget;

};
