// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/Weapons/ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "Items/Weapons/RocketMovementComponent.h"
#include "Interfaces/HitInterface.h"
#include "Engine/OverlapResult.h"
#include "SuperNova/DebugMacros.h"
#include "Items/Weapons/WeaponTypes.h"
#include "NiagaraComponent.h"
#include "Sound/SoundCue.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "NiagaraSystemInstance.h"

AProjectileRocket::AProjectileRocket()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rocket Mesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("RocketMovementComponent"));
	RocketMovementComponent->bRotationFollowsVelocity = true;
}

void AProjectileRocket::BeginPlay()
{
	Super::BeginPlay();

	SpawnTrailSystem();

	if (ProjectileLoop && LoopingSoundAttenuation)
	{
		//�����������
		ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached(
			ProjectileLoop,
			GetRootComponent(),
			FName(),
			GetActorLocation(),
			EAttachLocation::KeepWorldPosition,
			false,
			1.f,
			1.f,
			0.f,
			LoopingSoundAttenuation,
			(USoundConcurrency*)nullptr,
			false
		);
	}
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner())//��ֹ�ƶ������ʱ��ż������ը���Լ������
	{
		return;//����õ�UProjectileMovementComponent�����ڼ�⵽�����¼�ʱ��ֹͣ�ƶ�
	}

	APawn* FiringPawn = GetInstigator();
	if (FiringPawn)
	{
		AController* FiringController = FiringPawn->GetController();
		if (FiringController)
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this, // World context object
				Damage, // BaseDamage
				10.f, // MinimumDamage
				GetActorLocation(), // Origin
				DamageInnerRadius, // DamageInnerRadius
				DamageOuterRadius, // DamageOuterRadius
				1.f, // DamageFalloff
				UDamageType::StaticClass(), // DamageTypeClass
				TArray<AActor*>(), // IgnoreActors  ������ �����ķ�����Ҳ������˺�
				this, // DamageCauser
				FiringController // InstigatorController
			);
		}
	}

	// �ֶ����ұ�ը��Χ�ڵ� Actor
	TArray<FOverlapResult> Overlaps;
	FCollisionShape CollisionShape = FCollisionShape::MakeSphere(DamageOuterRadius);

	if (GetWorld()->OverlapMultiByChannel(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ECC_WorldDynamic, 
		CollisionShape))
	{
		for (const FOverlapResult& Result : Overlaps)
		{
			AActor* AffectedActor = Result.GetActor();
			if (!AffectedActor || AffectedActor == this) continue;
			IHitInterface* HitInterface = Cast<IHitInterface>(AffectedActor);
			if (!HitInterface && !AffectedActor->ActorHasTag(TEXT("Breakable"))) continue;
			// ������� ImpactPoint���ӱ�ը������ Actor �������߼��
			FHitResult ImpactHit;
			const FVector ActorCenter = AffectedActor->GetActorLocation();
			const FVector Direction = (ActorCenter - GetActorLocation()).GetSafeNormal();
			const FVector TraceEnd = GetActorLocation() + Direction * TRACE_LENGTH;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this);
			bool bHit = GetWorld()->LineTraceSingleByChannel(
				ImpactHit,
				GetActorLocation(),
				TraceEnd,
				ECC_WorldDynamic,
				QueryParams);
			//DRAW_LINE(GetActorLocation(), TraceEnd);
			FVector ImpactPoint = ImpactHit.ImpactPoint;
			//DRAW_SPHERE(ImpactPoint);
			// Breakable ������
			if (AffectedActor->ActorHasTag(TEXT("Breakable")))
			{
				CreateFields(ImpactPoint);
			}

			// IHitInterface ����
			if (HitInterface)
			{
				HitInterface->Execute_GetHit(AffectedActor, ImpactPoint, GetOwner());
			}
		}
	}

	StartDestroyTimer();

	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, GetActorTransform());
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
	if (ProjectileMesh)//3�������٣�������
	{
		ProjectileMesh->SetVisibility(false);
	}
	if (CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (TrailSystemComponent && TrailSystemComponent->GetSystemInstance())
	{
		//ֹͣ��������
		TrailSystemComponent->GetSystemInstance()->Deactivate();
	}
	if (ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying())
	{
		ProjectileLoopComponent->Stop();//���е�ʱ��ֹͣ����
	}
}
