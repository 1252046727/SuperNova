// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "EvacuationPoint.generated.h"

/**
 * 
 */
UCLASS()
class SUPERNOVA_API AEvacuationPoint : public AItem
{
	GENERATED_BODY()
public:
	AEvacuationPoint();

	void ShowNoMoneyWidget(bool bSeen);
protected:
	virtual void BeginPlay() override;
	virtual void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) override;

	UPROPERTY(VisibleAnywhere)
	class UNiagaraComponent* PickupEffectComponent;

	UPROPERTY(VisibleAnywhere)
	class UWidgetComponent* NoMoneyWidget;

};
