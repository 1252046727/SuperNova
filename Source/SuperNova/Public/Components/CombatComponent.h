// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HUD/SuperNovaHUD.h"
#include "Items/Weapons/WeaponTypes.h"
#include "CombatComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SUPERNOVA_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	friend class ASuperNovaCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void EquipWeapon(class AShootingWeapon* WeaponToEquip);
	void FireButtonPressed(bool bPressed);
	void Reload();
	UFUNCTION(BlueprintCallable)
	void FinishReloading();

protected:
	virtual void BeginPlay() override;

	void AttachActorToRightHand(AActor* ActorToAttach);
	void AttachActorToBackpack(AActor* ActorToAttach);
	void SetAiming(bool bIsAiming);
	void Fire();
	void TraceUnderCrosshairs(FHitResult& TraceHitResult);
	void SetHUDCrosshairs(float DeltaTime);
	void UpdateCarriedAmmo();
	int32 AmountToReload();
	void ReloadEmptyWeapon();

private:
	UPROPERTY()
	class ASuperNovaCharacter* Character;//ÔÚASuperNovaCharacterÖÐ¸³Öµ

	UPROPERTY()
	class ASuperNovaPlayerController* Controller;
	UPROPERTY()
	class ASuperNovaHUD* HUD;

	FHUDPackage HUDPackage;

	/**
	* HUD and crosshairs
	*/
	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;

	UPROPERTY()
	bool bAiming = false;

	UPROPERTY()
	bool bFireButtonPressed;

	FVector HitTarget;

	/**
	* Aiming and FOV
	*/
	// Field of view when not aiming; set to the camera's base FOV in BeginPlay
	float DefaultFOV;

	float CurrentFOV;

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomInterpSpeed = 20.f;

	void InterpFOV(float DeltaTime);

	/**
	* Automatic fire
	*/
	FTimerHandle FireTimer;
	FTimerHandle ReloadTimer;
	bool bCanFire = true;

	void StartFireTimer();
	void FireTimerFinished();

	bool CanFire();

	// Carried ammo for the currently-equipped weapon
	UPROPERTY()
	int32 CarriedAmmo;

	TMap<EWeaponType, int32> CarriedAmmoMap;

	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 30;

	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingSMGAmmo = 0;

	void InitializeCarriedAmmo();
	void UpdateAmmoValues();

public:	

};
