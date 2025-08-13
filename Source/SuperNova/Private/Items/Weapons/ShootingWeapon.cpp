// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/ShootingWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "Components/WidgetComponent.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Items/Weapons/Casing.h"
#include "PlayerController/SuperNovaPlayerController.h"
#include "Character/SuperNovaCharacter.h"
#include "Items/Weapons/WeaponTypes.h"
#include "Kismet/KismetMathLibrary.h"

AShootingWeapon::AShootingWeapon()
{
	ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMeshComponent"));
	//希望武器在我们丢下它的时候能在地上反弹
	ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = ItemMesh;
	Sphere->SetupAttachment(GetRootComponent());
	ItemEffect->SetupAttachment(GetRootComponent());
	PickupWidget->SetupAttachment(GetRootComponent());

	EnableCustomDepth(true);
	ItemMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	ItemMesh->MarkRenderStateDirty();//确保刷新颜色
}

void AShootingWeapon::BeginPlay()
{
	Super::BeginPlay();

	ShowPickupWidget(false);
}

void AShootingWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation)
	{
		ItemMesh->PlayAnimation(FireAnimation, false);
	}
	if (CasingClass)
	{
		const USkeletalMeshSocket* AmmoEjectSocket = ItemMesh->GetSocketByName(FName("AmmoEject"));
		if (AmmoEjectSocket)
		{
			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(ItemMesh);
			UWorld* World = GetWorld();
			if (World)
			{
				World->SpawnActor<ACasing>(
					CasingClass,
					SocketTransform.GetLocation(),
					SocketTransform.GetRotation().Rotator()
				);
			}
		}
	}
	SpendRound();
}

void AShootingWeapon::Dropped()
{
	SetItemState(EItemState::EIS_Dropped);
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);//保持其世界Transform
	ItemMesh->DetachFromComponent(DetachRules);//卸下武器
	SetOwner(nullptr);
	SuperNovaOwnerCharacter = nullptr;
	SuperNovaOwnerController = nullptr;
}

void AShootingWeapon::SetHUDAmmo()
{
	SuperNovaOwnerCharacter = SuperNovaOwnerCharacter == nullptr ? Cast<ASuperNovaCharacter>(GetOwner()) : SuperNovaOwnerCharacter;
	if (SuperNovaOwnerCharacter)
	{
		SuperNovaOwnerController = SuperNovaOwnerController == nullptr ? Cast<ASuperNovaPlayerController>(SuperNovaOwnerCharacter->Controller) : SuperNovaOwnerController;
		if (SuperNovaOwnerController)
		{
			SuperNovaOwnerController->SetHUDWeaponAmmo(Ammo);
		}
	}
}

void AShootingWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	SetHUDAmmo();
}

FVector AShootingWeapon::TraceEndWithScatter(const FVector& HitTarget)
{
	//开火点 
	const USkeletalMeshSocket* MuzzleFlashSocket = GetItemMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket == nullptr) return FVector();

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetItemMesh());
	const FVector TraceStart = SocketTransform.GetLocation();

	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	//随机方向RandomUnitVector()，随机长度FRandRange
	const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
	const FVector EndLoc = SphereCenter + RandVec;
	const FVector ToEndLoc = EndLoc - TraceStart;

	
	/*DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
	DrawDebugSphere(GetWorld(), EndLoc, 4.f, 12, FColor::Orange, true);
	DrawDebugLine(
		GetWorld(),
		TraceStart,
		FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()),
		FColor::Cyan,
		true);*/

	return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size());
}

void AShootingWeapon::EnableCustomDepth(bool bEnable)
{
	if (ItemMesh)
	{
		ItemMesh->SetRenderCustomDepth(bEnable);
	}
}

void AShootingWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	SetHUDAmmo();
}

bool AShootingWeapon::IsEmpty()
{
	return Ammo <= 0;
}

bool AShootingWeapon::IsFull()
{
	return Ammo == MagCapacity;
}

void AShootingWeapon::OnEquipped()
{
	DisableSphereCollision();
	DeactivateEmbers();
	PlayEquipSound();
	ItemMesh->SetSimulatePhysics(false);
	ItemMesh->SetEnableGravity(false);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (WeaponType == EWeaponType::EWT_SubmachineGun)
	{
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ItemMesh->SetEnableGravity(true);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	}
	EnableCustomDepth(false);
}

void AShootingWeapon::OnEquippedSecondary()
{
	DisableSphereCollision();
	DeactivateEmbers();
	SpawnPickupSound();
	ItemMesh->SetSimulatePhysics(false);
	ItemMesh->SetEnableGravity(false);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (WeaponType == EWeaponType::EWT_SubmachineGun)
	{
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ItemMesh->SetEnableGravity(true);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	}

	ItemMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
	ItemMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);
}

void AShootingWeapon::OnDropped()
{
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	//OnDroppedUpdateAmmo(Ammo);
	ItemMesh->SetSimulatePhysics(true);
	ItemMesh->SetEnableGravity(true);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	
	ItemMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	ItemMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);

}
