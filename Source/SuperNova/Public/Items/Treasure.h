// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "Treasure.generated.h"

UENUM(BlueprintType)
enum class ETreasureType : uint8
{
	//拾取物品类型
	ETT_Gold UMETA(DisplayName = "Add Golds"),
	ETT_Ammo UMETA(DisplayName = "Add Ammo"),
	ETT_Health UMETA(DisplayName = "Add Health"),
	ETT_Shield UMETA(DisplayName = "Add Shield"),

	ETT_MAX UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class SUPERNOVA_API ATreasure : public AItem
{
	GENERATED_BODY()
public:
	ATreasure();

protected:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* ItemMesh;

	UPROPERTY(VisibleAnywhere)
	class UNiagaraComponent* PickupEffectComponent;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* PickupEffect;

	/**
	* Enable or disable custom depth
	*/
	void EnableCustomDepth(bool bEnable);

	void SpawnPickupEffect();
private:
	UPROPERTY(EditAnywhere)
	ETreasureType TreasureType;

	UPROPERTY(EditAnywhere, Category = "Treasure Properties")
	int32 Gold=0;

	UPROPERTY(EditAnywhere, Category = "Treasure Properties")
	int32 Ammo=0; //这里的Ammo不用了，根据角色当前不同的武器加不同的子弹

	UPROPERTY(EditAnywhere, Category = "Treasure Properties")
	int32 Health=0;

	UPROPERTY(EditAnywhere, Category = "Treasure Properties")
	int32 Shield=0;
public:
	FORCEINLINE int32 GetGold() const { return Gold; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetHealth() const { return Health; }
	FORCEINLINE int32 GetShield() const { return Shield; }
};
