// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Character/SuperNovaCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "particles/ParticleSystemComponent.h"
#include "Items/Treasure.h"
#include "SuperNova/DebugMacros.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();

	//ø™ªµ„ 
	const USkeletalMeshSocket* MuzzleFlashSocket = GetItemMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetItemMesh());
		FVector Start = SocketTransform.GetLocation();

		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);

		if (FireHit.GetActor())
		{
			UGameplayStatics::ApplyDamage(
				FireHit.GetActor(),
				Damage,
				InstigatorController,
				this,
				UDamageType::StaticClass()
			);

			if (FireHit.GetActor()->ActorHasTag(TEXT("Breakable")))
			{
				CreateFields(FireHit.ImpactPoint);
				//DRAW_SPHERE(FireHit.ImpactPoint);
			}
			IHitInterface* HitInterface = Cast<IHitInterface>(FireHit.GetActor());
			if (HitInterface)
			{
				HitInterface->Execute_GetHit(FireHit.GetActor(), FireHit.ImpactPoint, GetOwner());
			}
		}

		if (ImpactParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				ImpactParticles,
				FireHit.ImpactPoint,
				FireHit.ImpactNormal.Rotation()
			);
		}
		if (HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				HitSound,
				FireHit.ImpactPoint
			);
		}

		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				MuzzleFlash,
				SocketTransform
			);
		}
		if (FireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				FireSound,
				GetActorLocation()
			);
		}
	}
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	UWorld* World = GetWorld();
	if (World)
	{
		FVector End = TraceStart + (HitTarget - TraceStart) * 1.25f;

		FCollisionQueryParams QueryParams;
		TArray<AActor*> Treasures;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATreasure::StaticClass(), Treasures);
		QueryParams.AddIgnoredActors(Treasures);
		World->LineTraceSingleByChannel(
			OutHit,
			TraceStart,
			End,
			ECollisionChannel::ECC_Visibility,
			QueryParams
		);
		FVector BeamEnd = End;
		if (OutHit.bBlockingHit)
		{
			BeamEnd = OutHit.ImpactPoint;
		}
		else
		{
			OutHit.ImpactPoint = End;
		}
		//DRAW_POINT(OutHit.ImpactPoint);
		if (BeamParticles)
		{
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				World,
				BeamParticles,
				TraceStart,
				FRotator::ZeroRotator,
				true
			);
			if (Beam)
			{
				Beam->SetVectorParameter(FName("Target"), BeamEnd);//÷’µ„
			}
		}
	}
}
