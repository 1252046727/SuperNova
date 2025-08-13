// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "CharacterTypes.h"
#include "Interfaces/PickupInterface.h"
#include "Components/TimelineComponent.h"
#include "SuperNovaCharacter.generated.h"

UCLASS()
class SUPERNOVA_API ASuperNovaCharacter : public ABaseCharacter, public IPickupInterface
{
	GENERATED_BODY()

public:
	ASuperNovaCharacter();
	friend class UCombatComponent;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetHit_Implementation(const FVector& ImpactPoint, AActor* Hitter) override;
	virtual void SetOverlappingItem(AItem* Item) override;
	virtual void AddGold(ATreasure* Treasure) override;
	virtual void AddAmmo(ATreasure* Treasure) override;
	virtual void AddHealth(ATreasure* Treasure) override;
	virtual void AddShield(ATreasure* Treasure) override;

	virtual void PostInitializeComponents() override;
	void PlayFireMontage(bool bAiming);

	void UpdateHUDEndurance();
	void UpdateHUDHealth();
	void UpdateHUDShield();
	void UpdateHUDGold();
	void InitializeHUD();
	bool HaveEnoughEvacuationGold();
	bool HaveEnoughPortalGold();
	virtual void Die() override;
	void ReduceGold(int32 AmountOfGold);

	int32 EvacuationGoldNeedToPay = 20;
	int32 PortalGoldNeedToPay = 5;

protected:
	virtual void BeginPlay() override;

	/** Callbacks for input	*/
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	virtual void Jump() override;
	void CrouchButtonPressed();
	void RunButtonPressed();
	void EquipButtonPressed();
	virtual void Attack() override;
	void AttackReleased();
	void SwapButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void ReloadButtonPressed();
	void InteractionButtonPressed();

	/** Combat */
	virtual void AttackEnd() override;
	virtual bool CanAttack() override;
	bool CanDisarm();
	bool CanArm();
	void Disarm();
	void Arm();
	void PlayEquipMontage(const FName& SectionName);
	void PlayReloadMontage();
	virtual void SpawnDefaultWeapon() override;
	void SpawnDefaultShootingWeapon();
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);

	UFUNCTION(BlueprintCallable)
	void AttackWeaponToBack();

	UFUNCTION(BlueprintCallable)
	void AttackWeaponToHand();

	UFUNCTION(BlueprintCallable)
	void FinishEquipping();

	UFUNCTION(BlueprintCallable)
	void HitReactEnd();

	UPROPERTY(EditAnywhere, Category = Combat)
	TSubclassOf<class AShootingWeapon> ShootingWeaponClass;
private:
	bool IsUnoccupied();
	void HideCameraIfCharacterClose();

	/** Character components */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* Combat;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;

	UPROPERTY(EditAnywhere, Category = Attribute)
	float WalkSpeed;

	UPROPERTY(EditAnywhere, Category = Attribute)
	float RunSpeed;

	UPROPERTY(EditAnywhere, Category = Attribute)
	float AimSpeed;

	bool bIsRuning=false;

	UPROPERTY()
	class ASuperNovaPlayerController* SuperNovaPlayerController;

	UPROPERTY(VisibleInstanceOnly)
	class AItem* OverlappingItem;

	UPROPERTY(EditDefaultsOnly, Category = Montages)
	UAnimMontage* EquipMontage;

	ECharacterState CharacterState = ECharacterState::ECS_Unequipped;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	EActionState ActionState = EActionState::EAS_Unoccupied;

	UPROPERTY(VisibleAnywhere, Category = Weapon)
	class AShootingWeapon* SecondaryWeapon;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY()
	class ASuperNovaGameMode* SuperNovaGameMode;

	FTimerHandle ElimTimer;

	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;

	void ElimTimerFinished();

	/**
	* Dissolve effect
	*/
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;
	FOnTimelineFloat DissolveTrack;//用于时间轴（Timeline）的一个委托

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;//曲线

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);//每一帧更新时间表时调用，参数代表在曲线上的位置
	void StartDissolve();

	// Dynamic instance that can change at runtime
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	// Material instance set on the Blueprint, used with the dynamic material instance
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance;

	/**
	* Elim effects
	*/
	UPROPERTY(EditAnywhere)
	UParticleSystem* ElimBotEffect;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* ElimBotComponent;

	UPROPERTY(EditAnywhere)
	class USoundCue* ElimBotSound;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere)
	class USoundBase* PortalSound;

	FTimerHandle InitializeHUDTimer;
public:
	FORCEINLINE ECharacterState GetCharacterState() const { return CharacterState; }
	FORCEINLINE EActionState GetActionState() const { return ActionState; }
	bool IsShootingWeaponEquipped();
	bool IsAiming();
	AShootingWeapon* GetEquippedShootingWeapon();
	FVector GetHitTarget() const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
