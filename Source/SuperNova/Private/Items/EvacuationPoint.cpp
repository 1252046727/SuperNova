// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/EvacuationPoint.h"
#include "Character/SuperNovaCharacter.h"
#include "Interfaces/PickupInterface.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/WidgetComponent.h"
#include "PlayerController/SuperNovaPlayerController.h"

AEvacuationPoint::AEvacuationPoint()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Sphere->SetupAttachment(GetRootComponent());
	Sphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	//PickupEffectComponent用于撤离点的外形
	PickupEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PickupEffectComponent"));
	PickupEffectComponent->SetupAttachment(GetRootComponent());

	ItemEffect->SetupAttachment(GetRootComponent());
	PickupWidget->SetupAttachment(GetRootComponent());

	NoMoneyWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("NoMoneyWidget"));
	NoMoneyWidget->SetupAttachment(GetRootComponent());
}

void AEvacuationPoint::ShowNoMoneyWidget(bool bSeen)
{
	if (bSeen)
	{
		NoMoneyWidget->SetVisibility(true);
	}
	else
	{
		NoMoneyWidget->SetVisibility(false);
	}
}

void AEvacuationPoint::BeginPlay()
{
	Super::BeginPlay();

	ShowPickupWidget(false);
	ShowNoMoneyWidget(false);
}

void AEvacuationPoint::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnSphereEndOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);
	ShowNoMoneyWidget(false);
}

