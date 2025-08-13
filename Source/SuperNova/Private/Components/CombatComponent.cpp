// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CombatComponent.h"
#include "Items/Weapons/ShootingWeapon.h"
#include "Character/SuperNovaCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "PlayerController/SuperNovaPlayerController.h"
#include "HUD/SuperNovaHUD.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Items/Treasure.h"
#include "SuperNova/DebugMacros.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
	}
	InitializeCarriedAmmo();
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FHitResult HitResult;
	TraceUnderCrosshairs(HitResult);
	HitTarget = HitResult.ImpactPoint;

	SetHUDCrosshairs(DeltaTime);
	InterpFOV(DeltaTime);
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (Character->SecondaryWeapon == nullptr || Character->ActionState == EActionState::EAS_EquippingWeapon) return;

	if (bAiming)
	{
		//瞄准的ZoomedFOV和ZoomInterpSpeed根据武器不同而不同  ZoomedFOV越小，放大倍数越大
		CurrentFOV = FMath::FInterpTo(CurrentFOV, Character->SecondaryWeapon->ZoomedFOV, DeltaTime, Character->SecondaryWeapon->ZoomInterpSpeed);
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}
	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::EquipWeapon(AShootingWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr) return;
	if (Character->SecondaryWeapon)
	{
		Character->SecondaryWeapon->Dropped();
	}
	Character->SecondaryWeapon = WeaponToEquip;
	Character->SecondaryWeapon->SetOwner(Character);
	Character->SecondaryWeapon->SetHUDAmmo();
	UpdateCarriedAmmo();
	if (Character->CharacterState == ECharacterState::ECS_EquippedShootingWeapon)
	{

		Character->SecondaryWeapon->SetItemState(EItemState::EIS_Equipped);
		AttachActorToRightHand(WeaponToEquip);
		ReloadEmptyWeapon();
	}
	else if(Character->CharacterState == ECharacterState::ECS_EquippedWeapon)
	{
		Character->SecondaryWeapon->SetItemState(EItemState::EIS_EquippedSecondary);
		AttachActorToBackpack(WeaponToEquip);
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;//不复制该变量让服务器同步开火的原因是想实现持续按下鼠标左键自动持续开火，复制只会在改变的时候触发
	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && Character->ActionState == EActionState::EAS_Unoccupied && Character->SecondaryWeapon && !Character->SecondaryWeapon->IsFull())
	{
		Character->ActionState = EActionState::EAS_ReloadingWeapon;
		Character->bIsReloadingWeapon = true;
		Character->PlayReloadMontage();
	}
}

void UCombatComponent::FinishReloading()
{
	if (Character == nullptr) return;
	Character->ActionState = EActionState::EAS_Unoccupied;
	Character->bIsReloadingWeapon = false;
	UpdateAmmoValues();
	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::Fire()
{
	if (CanFire())
	{
		bCanFire = false;
		CrosshairShootingFactor = 0.75f;
		Character->PlayFireMontage(bAiming);
		HitTarget = !bAiming ? Character->SecondaryWeapon->TraceEndWithScatter(HitTarget) : HitTarget;
		Character->SecondaryWeapon->Fire(HitTarget);
		StartFireTimer();
	}
}

void UCombatComponent::StartFireTimer()
{
	if (Character == nullptr || Character->SecondaryWeapon == nullptr) return;
	Character->GetWorldTimerManager().SetTimer(
		FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,
		Character->SecondaryWeapon->FireDelay
	);
}

void UCombatComponent::FireTimerFinished()
{
	if (Character == nullptr || Character->SecondaryWeapon == nullptr) return;
	bCanFire = true;
	if (bFireButtonPressed && Character->SecondaryWeapon->bAutomatic)
	{
		Fire();
	}
	ReloadEmptyWeapon();
}

bool UCombatComponent::CanFire()
{
	if (Character == nullptr || Character->SecondaryWeapon == nullptr) return false;
	return !Character->SecondaryWeapon->IsEmpty() && bCanFire && Character->ActionState == EActionState::EAS_Unoccupied;
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SubmachineGun, StartingSMGAmmo);
}

void UCombatComponent::UpdateAmmoValues()
{
	if (Character == nullptr || Character->SecondaryWeapon == nullptr) return;
	int32 ReloadAmount = AmountToReload();
	if (CarriedAmmoMap.Contains(Character->SecondaryWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[Character->SecondaryWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[Character->SecondaryWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<ASuperNovaPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	Character->SecondaryWeapon->AddAmmo(ReloadAmount);
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}
	//CrosshairLocation是屏幕空间的坐标
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);//视口的中心
	FVector CrosshairWorldPosition;//位置
	FVector CrosshairWorldDirection;//方向 长度为1的单位向量
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);

	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;//起点是屏幕中心相对应的3D世界位置（也就是摄像机的中心位置）
		if (Character)
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			//检测起点前移到角色前面一点点
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
		}

		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;

		FCollisionQueryParams QueryParams;
		TArray<AActor*> Treasures;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATreasure::StaticClass(), Treasures);
		QueryParams.AddIgnoredActors(Treasures);
		GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility,
			QueryParams
		);
		//未击中设置为End
		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
		}
		HUDPackage.CrosshairsColor = FLinearColor::White;
		//DRAW_POINT_SingleFrame(TraceHitResult.ImpactPoint);
		//Actor是否有效以及是否实现该接口
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UHitInterface>())
		{
			HUDPackage.CrosshairsColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairsColor = FLinearColor::White;
		}
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr) return;
	Controller = Controller == nullptr ? Cast<ASuperNovaPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<ASuperNovaHUD>(Controller->GetHUD()) : HUD;
		if (HUD)
		{
			if (Character && 
				Character->SecondaryWeapon &&
				Character->CharacterState == ECharacterState::ECS_EquippedShootingWeapon &&
				Character->ActionState != EActionState::EAS_EquippingWeapon &&
				Character->ActionState != EActionState::EAS_Dead)
			{
				HUDPackage.CrosshairsCenter = Character->SecondaryWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsLeft = Character->SecondaryWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = Character->SecondaryWeapon->CrosshairsRight;
				HUDPackage.CrosshairsBottom = Character->SecondaryWeapon->CrosshairsBottom;
				HUDPackage.CrosshairsTop = Character->SecondaryWeapon->CrosshairsTop;
			}
			else
			{
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
			}
			// Calculate crosshair spread

			// [0, 600] -> [0, 1]
			FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;
			//CrosshairVelocityFactor根据速度转换成0-1之间的值
			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

			if (Character->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);//变化较慢
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);//变化较快
			}

			if (bAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
			}
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}

			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 20.f);

			HUDPackage.CrosshairSpread =
				0.5f +
				CrosshairVelocityFactor +
				CrosshairInAirFactor -
				CrosshairAimFactor +
				CrosshairShootingFactor;

			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::UpdateCarriedAmmo()
{
	if (Character->SecondaryWeapon == nullptr) return;
	if (CarriedAmmoMap.Contains(Character->SecondaryWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[Character->SecondaryWeapon->GetWeaponType()];
	}

	Controller = Controller == nullptr ? Cast<ASuperNovaPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

int32 UCombatComponent::AmountToReload()
{
	if (Character->SecondaryWeapon == nullptr) return 0;
	int32 RoomInMag = Character->SecondaryWeapon->GetMagCapacity() - Character->SecondaryWeapon->GetAmmo();//武器最多还可以装多少子弹

	if (CarriedAmmoMap.Contains(Character->SecondaryWeapon->GetWeaponType()))
	{
		int32 AmountCarried = CarriedAmmoMap[Character->SecondaryWeapon->GetWeaponType()];
		int32 Least = FMath::Min(RoomInMag, AmountCarried);
		return FMath::Clamp(RoomInMag, 0, Least);//返回RoomInMag，始终在[0,Least]之间
	}
	return 0;
}

void UCombatComponent::ReloadEmptyWeapon()
{
	if (Character && Character->SecondaryWeapon->IsEmpty() && CarriedAmmo > 0)
	{
		if (Character->ActionState == EActionState::EAS_Unoccupied)
		{
			Reload();
		}
		else
		{
			//一秒之后再尝试换弹
			Character->GetWorldTimerManager().SetTimer(
				ReloadTimer,
				this,
				&UCombatComponent::ReloadEmptyWeapon,
				1.f
			);
		}
	}
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr) return;
	//在骨骼中添加的socket名字叫RightHandWeaponSocket
	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandWeaponSocket"));
	if (HandSocket)
	{
		//WeaponState和AttachActor复制先后顺序无法保证，需要AttachActor之前物理和碰撞特性关闭
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::AttachActorToBackpack(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr) return;
	const USkeletalMeshSocket* BackpackSocket = Character->GetMesh()->GetSocketByName(FName("BackpackSocket"));
	if (BackpackSocket)
	{
		BackpackSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming;
}


