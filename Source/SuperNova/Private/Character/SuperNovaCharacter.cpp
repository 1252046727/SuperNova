// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SuperNovaCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PlayerController/SuperNovaPlayerController.h"
#include "Items/Item.h"
#include "Items/Weapons/Weapon.h"
#include "Animation/AnimMontage.h"
#include "Components/AttributeComponent.h"
#include "Items/Treasure.h"
#include "Components/CapsuleComponent.h"
#include "Components/CombatComponent.h"
#include "Items/Weapons/ShootingWeapon.h"
#include "GameMode/SuperNovaGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "Items/EvacuationPoint.h"
#include "Items/Portal.h"

ASuperNovaCharacter::ASuperNovaCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	GetMesh()->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	GetMesh()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));
}

void ASuperNovaCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Attributes)
	{
		if (bIsRuning && GetVelocity().Size() > 0) {
			if (GetVelocity().Z == 0) {
				Attributes->CostEndurance(DeltaTime);
			}
			if (Attributes->GetEndurance() == 0) {
				RunButtonPressed();
			}
		}
		else {
			if (GetVelocity().Z == 0) {
				Attributes->AddEndurance(DeltaTime);
			}
		}
	}
	UpdateHUDEndurance();
	HideCameraIfCharacterClose();
}

void ASuperNovaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASuperNovaCharacter::Jump);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASuperNovaCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Run", IE_Pressed, this, &ASuperNovaCharacter::RunButtonPressed);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ASuperNovaCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &ASuperNovaCharacter::Attack);
	PlayerInputComponent->BindAction("Attack", IE_Released, this, &ASuperNovaCharacter::AttackReleased);
	PlayerInputComponent->BindAction("Swap", IE_Pressed, this, &ASuperNovaCharacter::SwapButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ASuperNovaCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ASuperNovaCharacter::AimButtonReleased);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ASuperNovaCharacter::ReloadButtonPressed);
	PlayerInputComponent->BindAction("Interaction", IE_Pressed, this, &ASuperNovaCharacter::InteractionButtonPressed);

	PlayerInputComponent->BindAxis("MoveForward", this, &ASuperNovaCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ASuperNovaCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ASuperNovaCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ASuperNovaCharacter::LookUp);
}

void ASuperNovaCharacter::GetHit_Implementation(const FVector& ImpactPoint, AActor* Hitter)
{
	if (ActionState == EActionState::EAS_Dead) return;

	Super::GetHit_Implementation(ImpactPoint, Hitter);

	SetWeaponCollisionEnabled(ECollisionEnabled::NoCollision);
	if (Attributes && Attributes->GetHealthPercent() > 0.f)
	{
		if (bIsSwapingWeapon == false && bIsReloadingWeapon == false)
		{
			ActionState = EActionState::EAS_HitReaction;
		}
	}
}

void ASuperNovaCharacter::SetOverlappingItem(AItem* Item)
{
	if (OverlappingItem)
	{
		OverlappingItem->ShowPickupWidget(false);
	}
	OverlappingItem = Item;
	if (OverlappingItem)
	{
		OverlappingItem->ShowPickupWidget(true);
	}
}

void ASuperNovaCharacter::AddGold(ATreasure* Treasure)
{
	if (SuperNovaPlayerController && Attributes)
	{
		Attributes->AddGold(Treasure->GetGold());
		SuperNovaPlayerController->SetHUDGold(Attributes->GetGold());
	}
}

void ASuperNovaCharacter::AddAmmo(ATreasure* Treasure)
{
	SuperNovaPlayerController = SuperNovaPlayerController == nullptr ? Cast<ASuperNovaPlayerController>(Controller) : SuperNovaPlayerController;
	int32 AddAmmoAmount = 0;
	switch (SecondaryWeapon->GetWeaponType())
	{
	case EWeaponType::EWT_AssaultRifle:
		AddAmmoAmount = 5;
		break;
	case EWeaponType::EWT_Pistol:
		AddAmmoAmount = 7;
		break;
	case EWeaponType::EWT_SubmachineGun:
		AddAmmoAmount = 10;
		break;
	case EWeaponType::EWT_RocketLauncher:
		AddAmmoAmount = 1;
		break;
	}
	
	Combat->CarriedAmmo += AddAmmoAmount;
	Combat->CarriedAmmoMap[SecondaryWeapon->GetWeaponType()] += AddAmmoAmount;
	if (SuperNovaPlayerController)
	{
		SuperNovaPlayerController->SetHUDCarriedAmmo(Combat->CarriedAmmo);
	}
	if (CharacterState == ECharacterState::ECS_EquippedShootingWeapon)
	{
		Combat->ReloadEmptyWeapon();
	}
}

void ASuperNovaCharacter::AddHealth(ATreasure* Treasure)
{
	if (SuperNovaPlayerController && Attributes)
	{
		Attributes->AddHealth(Treasure->GetHealth());
		UpdateHUDHealth();
	}
}

void ASuperNovaCharacter::AddShield(ATreasure* Treasure)
{
	if (SuperNovaPlayerController && Attributes)
	{
		Attributes->AddShield(Treasure->GetShield());
		UpdateHUDShield();
	}
}

void ASuperNovaCharacter::PostInitializeComponents()
{
	//在最早可以访问战斗组件的函数 初始化Character
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}
}

void ASuperNovaCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || SecondaryWeapon == nullptr || CharacterState != ECharacterState::ECS_EquippedShootingWeapon) return;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);//播放蒙太奇
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");//根据是否瞄准选择片段
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ASuperNovaCharacter::UpdateHUDEndurance()
{
	SuperNovaPlayerController = SuperNovaPlayerController == nullptr ? Cast<ASuperNovaPlayerController>(Controller) : SuperNovaPlayerController;
	if (SuperNovaPlayerController && Attributes)
	{
		SuperNovaPlayerController->SetHUDEndurance(Attributes->GetEndurancePercent());
	}
}

void ASuperNovaCharacter::UpdateHUDHealth()
{
	if (SuperNovaPlayerController && Attributes)
	{
		SuperNovaPlayerController->SetHUDHealth(Attributes->GetHealthPercent());
		SuperNovaPlayerController->SetHUDHealthText(Attributes->GetHealth(), Attributes->GetMaxHealth());
	}
}

void ASuperNovaCharacter::UpdateHUDShield()
{
	if (SuperNovaPlayerController && Attributes)
	{
		SuperNovaPlayerController->SetHUDShield(Attributes->GetShieldPercent());
		SuperNovaPlayerController->SetHUDShieldText(Attributes->GetShield(), Attributes->GetMaxShield());
	}
}

void ASuperNovaCharacter::UpdateHUDGold()
{
	if (SuperNovaPlayerController && Attributes)
	{
		SuperNovaPlayerController->SetHUDGold(Attributes->GetGold());
	}
}

void ASuperNovaCharacter::InitializeHUD()
{
	SuperNovaPlayerController = SuperNovaPlayerController == nullptr ? Cast<ASuperNovaPlayerController>(Controller) : SuperNovaPlayerController;
	if (SuperNovaPlayerController)
	{
		SuperNovaPlayerController->SetHUDGold(0);
		UpdateHUDHealth();
		SecondaryWeapon->SetHUDAmmo();
		Combat->UpdateCarriedAmmo();
	}
	else
	{
		GetWorldTimerManager().SetTimer(
			InitializeHUDTimer,
			this,
			&ASuperNovaCharacter::InitializeHUD,
			0.1f
		);
	}
}

bool ASuperNovaCharacter::HaveEnoughEvacuationGold()
{
	return Attributes && Attributes->GetGold() >= EvacuationGoldNeedToPay;
}

bool ASuperNovaCharacter::HaveEnoughPortalGold()
{
	return Attributes && Attributes->GetGold() >= PortalGoldNeedToPay;
}

void ASuperNovaCharacter::BeginPlay()
{
	Super::BeginPlay();

	OnTakeAnyDamage.AddDynamic(this, &ASuperNovaCharacter::ReceiveDamage);
	Tags.Add(FName("EngageableTarget"));
	SpawnDefaultWeapon();
	SpawnDefaultShootingWeapon();
	InitializeHUD();
}

void ASuperNovaCharacter::MoveForward(float Value)
{
	if (!IsUnoccupied()) return;
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void ASuperNovaCharacter::MoveRight(float Value)
{
	if (!IsUnoccupied()) return;
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void ASuperNovaCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ASuperNovaCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ASuperNovaCharacter::Jump()
{
	if (!IsUnoccupied() || ActionState == EActionState::EAS_Dead) return;
	if (Attributes && Attributes->TryJump())
	{
		Super::Jump();
	}
}

void ASuperNovaCharacter::CrouchButtonPressed()
{
	if (!IsUnoccupied() || ActionState == EActionState::EAS_Dead) return;
	if (GetCharacterMovement()->IsFalling())
	{
		return;
	}
	if (bIsRuning)
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		bIsRuning = !bIsRuning;
	}
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();//如果成功 bIsCrouch变为true
	}
}

void ASuperNovaCharacter::RunButtonPressed()
{
	if (GetCharacterMovement()->IsFalling()|| bIsCrouched || Combat->bAiming || ActionState == EActionState::EAS_Dead) return;
	if (bIsRuning)
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
	else {
		GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
	}
	bIsRuning = !bIsRuning;
}

void ASuperNovaCharacter::EquipButtonPressed()
{
	if (!IsUnoccupied() || ActionState == EActionState::EAS_Dead) return;
	AShootingWeapon* OverlappingWeapon = Cast<AShootingWeapon>(OverlappingItem);
	if (Combat && OverlappingWeapon)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
	else
	{
		if (CanDisarm())
		{
			Disarm();
		}
		else if (CanArm())
		{
			Arm();
		}
	}
}

void ASuperNovaCharacter::Attack()
{
	if (ActionState == EActionState::EAS_Dead) return;
	if (CharacterState == ECharacterState::ECS_EquippedShootingWeapon && SecondaryWeapon)
	{
		if (Combat)
		{
			Combat->FireButtonPressed(true);
		}
	}
	else
	{
		Super::Attack();
		if (CanAttack() && Attributes && Attributes->TryAttack())
		{
			PlayAttackMontage();
			ActionState = EActionState::EAS_Attacking;
		}
	}
}

void ASuperNovaCharacter::AttackReleased()
{
	if (ActionState == EActionState::EAS_Dead) return;
	if (CharacterState == ECharacterState::ECS_EquippedShootingWeapon && SecondaryWeapon)
	{
		if (Combat)
		{
			Combat->FireButtonPressed(false);
		}
	}
}

void ASuperNovaCharacter::SwapButtonPressed()
{
	if (ActionState == EActionState::EAS_Dead) return;
	if (CanDisarm())
	{
		Disarm();
	}
	else if (CanArm())
	{
		Arm();
	}
}

void ASuperNovaCharacter::AimButtonPressed()
{
	if (ActionState == EActionState::EAS_Dead) return;
	if (Combat && CharacterState==ECharacterState::ECS_EquippedShootingWeapon)
	{
		Combat->SetAiming(true);
		GetCharacterMovement()->MaxWalkSpeed = AimSpeed;
		if (bIsRuning)
		{
			bIsRuning = !bIsRuning;
		}
	}
}

void ASuperNovaCharacter::AimButtonReleased()
{
	if (ActionState == EActionState::EAS_Dead) return;
	if (Combat && CharacterState == ECharacterState::ECS_EquippedShootingWeapon)
	{
		Combat->SetAiming(false);
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
}

void ASuperNovaCharacter::ReloadButtonPressed()
{
	if (Combat)
	{
		Combat->Reload();
	}
}

void ASuperNovaCharacter::InteractionButtonPressed()
{
	if (!IsUnoccupied() || ActionState == EActionState::EAS_Dead) return;

	//撤离点交互
	AEvacuationPoint* EvacuationPoint = Cast<AEvacuationPoint>(OverlappingItem);
	if (EvacuationPoint)
	{
		EvacuationPoint->ShowPickupWidget(false);
		if (HaveEnoughEvacuationGold())
		{
			SuperNovaPlayerController = SuperNovaPlayerController == nullptr ? Cast<ASuperNovaPlayerController>(Controller) : SuperNovaPlayerController;
			SuperNovaPlayerController->ShowSettlementWidget(true);
		}
		else
		{
			EvacuationPoint->ShowNoMoneyWidget(true);
		}
	}

	//传送门交互
	APortal* Portal = Cast<APortal>(OverlappingItem);
	if (Portal)
	{
		Portal->ShowPickupWidget(false);
		if (HaveEnoughPortalGold())
		{
			ReduceGold(PortalGoldNeedToPay);
			int32 RandomIndex = FMath::RandRange(0, Portal->PortalLocation.Num() - 1);
			FVector TargetLocation = Portal->PortalLocation[RandomIndex];
			SetActorLocation(TargetLocation);
			if (PortalSound)
			{
				UGameplayStatics::SpawnSoundAtLocation(
					this,
					PortalSound,
					GetActorLocation()
				);
			}
		}
		else
		{
			Portal->ShowNoMoneyWidget(true);
		}
	}
}

void ASuperNovaCharacter::AttackEnd()
{
	ActionState = EActionState::EAS_Unoccupied;
}

bool ASuperNovaCharacter::CanAttack()
{
	return ActionState == EActionState::EAS_Unoccupied &&
		CharacterState != ECharacterState::ECS_Unequipped;
}

bool ASuperNovaCharacter::CanDisarm()
{
	return ActionState == EActionState::EAS_Unoccupied && 
		CharacterState == ECharacterState::ECS_EquippedShootingWeapon&& //当前拿着的是枪
		SecondaryWeapon;
}

bool ASuperNovaCharacter::CanArm()
{
	return ActionState == EActionState::EAS_Unoccupied &&
		CharacterState == ECharacterState::ECS_EquippedWeapon&& //当前拿着的是剑
		SecondaryWeapon;
}

void ASuperNovaCharacter::Disarm()
{
	PlayEquipMontage(FName("EquipWeapon"));
	bIsSwapingWeapon = true;
	if (IsAiming())
	{
		AimButtonReleased();
	}
	CharacterState = ECharacterState::ECS_EquippedWeapon;
	ActionState = EActionState::EAS_EquippingWeapon;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
}

void ASuperNovaCharacter::Arm()
{
	PlayEquipMontage(FName("EquipShootingWeapon"));
	bIsSwapingWeapon = true;
	CharacterState = ECharacterState::ECS_EquippedShootingWeapon;
	ActionState = EActionState::EAS_EquippingWeapon;
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void ASuperNovaCharacter::PlayEquipMontage(const FName& SectionName)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && EquipMontage)
	{
		AnimInstance->Montage_Play(EquipMontage);
		AnimInstance->Montage_JumpToSection(SectionName, EquipMontage);
	}
}

void ASuperNovaCharacter::PlayReloadMontage()
{
	if (SecondaryWeapon == nullptr || CharacterState!=ECharacterState::ECS_EquippedShootingWeapon) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;

		switch (SecondaryWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("RocketLauncher");
			break;
		case EWeaponType::EWT_Pistol:
			SectionName = FName("Pistol");
			break;
		case EWeaponType::EWT_SubmachineGun:
			SectionName = FName("Pistol");
			break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ASuperNovaCharacter::Die()
{
	Super::Die();

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	if (SuperNovaPlayerController)
	{
		SuperNovaPlayerController->SetHUDWeaponAmmo(0);
	}

	if (IsAiming())
	{
		AimButtonReleased();
	}
	ActionState = EActionState::EAS_Dead;

	DisableMeshCollision();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	// Spawn elim bot
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
		);
	}
	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		);
	}

	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&ASuperNovaCharacter::ElimTimerFinished,
		ElimDelay
	);

	SuperNovaGameMode = SuperNovaGameMode == nullptr ? GetWorld()->GetAuthGameMode<ASuperNovaGameMode>() : SuperNovaGameMode;
	if (SuperNovaGameMode)
	{
		SuperNovaPlayerController = SuperNovaPlayerController == nullptr ? Cast<ASuperNovaPlayerController>(Controller) : SuperNovaPlayerController;
		SuperNovaGameMode->PlayerEliminated(this, SuperNovaPlayerController);
	}
}

void ASuperNovaCharacter::ReduceGold(int32 AmountOfGold)
{
	if (SuperNovaPlayerController && Attributes)
	{
		Attributes->AddGold(-AmountOfGold);
		SuperNovaPlayerController->SetHUDGold(Attributes->GetGold());
	}
}

void ASuperNovaCharacter::ElimTimerFinished()
{
	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}
	if (SuperNovaPlayerController)
	{
		SuperNovaPlayerController->ShowSettlementWidget(false);
	}
}


void ASuperNovaCharacter::SpawnDefaultWeapon()
{
	UWorld* World = GetWorld();
	if (World && WeaponClass)
	{
		AWeapon* DefaultWeapon = World->SpawnActor<AWeapon>(WeaponClass);
		DefaultWeapon->Equip(GetMesh(), FName("SpineSocket"), this, this);
		EquippedWeapon = DefaultWeapon;
		EquippedWeapon->SetItemState(EItemState::EIS_EquippedSecondary);
	}
}

void ASuperNovaCharacter::SpawnDefaultShootingWeapon()
{
	CharacterState = ECharacterState::ECS_EquippedShootingWeapon;
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	UWorld* World = GetWorld();
	if (World && ShootingWeaponClass)
	{
		AShootingWeapon* DefaultShootingWeapon = World->SpawnActor<AShootingWeapon>(ShootingWeaponClass);
		Combat->EquipWeapon(DefaultShootingWeapon);
	}
}

void ASuperNovaCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	if (ActionState == EActionState::EAS_Dead) return;
	HandleDamage(Damage);
	UpdateHUDHealth();
	UpdateHUDShield();
}

void ASuperNovaCharacter::AttackWeaponToBack()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->AttachMeshToSocket(GetMesh(), FName("SpineSocket"));
	}
}

void ASuperNovaCharacter::AttackWeaponToHand()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->AttachMeshToSocket(GetMesh(), FName("RightHandSocket"));
	}
}

void ASuperNovaCharacter::FinishEquipping()
{
	ActionState = EActionState::EAS_Unoccupied;
	bIsSwapingWeapon = false;
	if (CharacterState == ECharacterState::ECS_EquippedWeapon && EquippedWeapon && Combat && SecondaryWeapon)
	{
		EquippedWeapon->PlayEquipSound();
		EquippedWeapon->SetItemState(EItemState::EIS_Equipped);
		SecondaryWeapon->SetItemState(EItemState::EIS_EquippedSecondary);
		AttackWeaponToHand();
		Combat->AttachActorToBackpack(SecondaryWeapon);
	}
	if (CharacterState == ECharacterState::ECS_EquippedShootingWeapon && EquippedWeapon && Combat && SecondaryWeapon)
	{
		SecondaryWeapon->PlayEquipSound();
		EquippedWeapon->SetItemState(EItemState::EIS_EquippedSecondary);
		SecondaryWeapon->SetItemState(EItemState::EIS_Equipped);
		AttackWeaponToBack();
		Combat->AttachActorToRightHand(SecondaryWeapon);
		Combat->ReloadEmptyWeapon();
	}
}

void ASuperNovaCharacter::HitReactEnd()
{
	ActionState = EActionState::EAS_Unoccupied;
}

bool ASuperNovaCharacter::IsUnoccupied()
{
	return ActionState == EActionState::EAS_Unoccupied;
}

void ASuperNovaCharacter::HideCameraIfCharacterClose()
{
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (EquippedWeapon)
		{
			EquippedWeapon->GetItemMesh()->bOwnerNoSee = true;
		}
		if (SecondaryWeapon)
		{
			SecondaryWeapon->GetItemMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (EquippedWeapon)
		{
			EquippedWeapon->GetItemMesh()->bOwnerNoSee = false;
		}
		if (SecondaryWeapon)
		{
			SecondaryWeapon->GetItemMesh()->bOwnerNoSee = false;
		}
	}
}

void ASuperNovaCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ASuperNovaCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ASuperNovaCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		//设置时间表的曲线和委托
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

bool ASuperNovaCharacter::IsShootingWeaponEquipped()
{
	return CharacterState == ECharacterState::ECS_EquippedShootingWeapon;
}

bool ASuperNovaCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

AShootingWeapon* ASuperNovaCharacter::GetEquippedShootingWeapon()
{
	return SecondaryWeapon;
}

FVector ASuperNovaCharacter::GetHitTarget() const
{
	if (Combat == nullptr) return FVector();
	return Combat->HitTarget;
}
