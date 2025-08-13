// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/Weapons/ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Character.h"
#include "Interfaces/HitInterface.h"
#include "SuperNova/DebugMacros.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;//ȷ���ӵ�����ת���ٶȱ���һ��
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = InitialSpeed;
}

//����ͼ�и���InitialSpeedʱ��ͨ�������������ProjectileMovementComponent���ٶȣ��Ӷ�����һ��
#if WITH_EDITOR
void AProjectileBullet::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);
	FName PropertyName = Event.Property != nullptr ? Event.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileBullet, InitialSpeed))
	{
		if (ProjectileMovementComponent)
		{
			ProjectileMovementComponent->InitialSpeed = InitialSpeed;
			ProjectileMovementComponent->MaxSpeed = InitialSpeed;
		}
	}
}
#endif

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	UGameplayStatics::ApplyDamage(
		OtherActor,
		Damage,
		GetInstigator()->GetController(),
		this,
		UDamageType::StaticClass());

	if (OtherActor->ActorHasTag(TEXT("Breakable")))
	{
		CreateFields(Hit.ImpactPoint);
		//DRAW_SPHERE(Hit.ImpactPoint);
	}
	IHitInterface* HitInterface = Cast<IHitInterface>(OtherActor);
	if (HitInterface)
	{
		HitInterface->Execute_GetHit(OtherActor, Hit.ImpactPoint, GetOwner());
	}
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}

