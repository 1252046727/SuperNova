// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Treasure.h"
#include "Character/SuperNovaCharacter.h"
#include "Interfaces/PickupInterface.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "Components/WidgetComponent.h"
#include "Items/Weapons/WeaponTypes.h"
#include "NiagaraFunctionLibrary.h"

ATreasure::ATreasure()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMeshComponent"));
	ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemMesh->SetupAttachment(GetRootComponent());

	Sphere->SetupAttachment(GetRootComponent());
	Sphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	ItemEffect->SetupAttachment(GetRootComponent());
	PickupWidget->SetupAttachment(GetRootComponent());

	//PickupEffectComponent用于Health和Shield的外形
	PickupEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PickupEffectComponent"));
	PickupEffectComponent->SetupAttachment(RootComponent);

	EnableCustomDepth(true);
	ItemMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_PURPLE);
	ItemMesh->MarkRenderStateDirty();//确保刷新颜色
}

void ATreasure::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	IPickupInterface* PickupInterface = Cast<IPickupInterface>(OtherActor);
	if (PickupInterface)
	{
		switch (TreasureType)
		{
		case ETreasureType::ETT_Gold:
			PickupInterface->AddGold(this);
			break;
		case ETreasureType::ETT_Ammo:
			PickupInterface->AddAmmo(this);
			break;
		case ETreasureType::ETT_Health:
			PickupInterface->AddHealth(this);
			break;
		case ETreasureType::ETT_Shield:
			PickupInterface->AddShield(this);
			break;
		}

		SpawnPickupSound();
		SpawnPickupEffect();
		Destroy();
	}
}

void ATreasure::EnableCustomDepth(bool bEnable)
{
	if (ItemMesh)
	{
		ItemMesh->SetRenderCustomDepth(bEnable);
	}
}

void ATreasure::SpawnPickupEffect()
{
	if (PickupEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			PickupEffect,
			GetActorLocation(),
			GetActorRotation()
		);
	}
}
