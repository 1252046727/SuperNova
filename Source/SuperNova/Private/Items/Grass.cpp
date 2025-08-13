// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Grass.h"
#include "Components/BoxComponent.h"
#include "Character/SuperNovaCharacter.h"
#include "Kismet/GameplayStatics.h"

AGrass::AGrass()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMeshComponent"));
	ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemMesh->SetupAttachment(GetRootComponent());

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetupAttachment(GetRootComponent());

}

void AGrass::BeginPlay()
{
	Super::BeginPlay();
	
	Box->OnComponentBeginOverlap.AddDynamic(this, &AGrass::OnBoxOverlap);
	Box->OnComponentEndOverlap.AddDynamic(this, &AGrass::OnBoxEndOverlap);
}

void AGrass::OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ASuperNovaCharacter* Player = Cast<ASuperNovaCharacter>(OtherActor))
	{
		if (OverlappingCharacter != Player)
		{
			OverlappingCharacter = Player;
			GetWorld()->GetTimerManager().SetTimer(DamageTimerHandle, this, &AGrass::ApplyDamage, 1.5f, true, 0.0f);
		}
	}
}

void AGrass::OnBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor == OverlappingCharacter)
	{
		GetWorld()->GetTimerManager().ClearTimer(DamageTimerHandle);
		OverlappingCharacter = nullptr;
	}
}

void AGrass::ApplyDamage()
{
	if (OverlappingCharacter)
	{
		UGameplayStatics::ApplyDamage(
			OverlappingCharacter,
			2.0f,
			nullptr,
			this,
			UDamageType::StaticClass());
	}
	OverlappingCharacter->PlayHitSound(GetActorLocation());
	if (!OverlappingCharacter->IsAlive())
	{
		OverlappingCharacter->Die();
	}
}

void AGrass::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

