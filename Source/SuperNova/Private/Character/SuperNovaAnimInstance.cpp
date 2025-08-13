// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/SuperNovaAnimInstance.h"
#include "Character/SuperNovaCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Items/Weapons/ShootingWeapon.h"

void USuperNovaAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	SuperNovaCharacter = Cast<ASuperNovaCharacter>(TryGetPawnOwner());
}

void USuperNovaAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (SuperNovaCharacter == nullptr)
	{
		SuperNovaCharacter = Cast<ASuperNovaCharacter>(TryGetPawnOwner());
	}
	if (SuperNovaCharacter == nullptr) return;

	FVector Velocity = SuperNovaCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();
	bIsInAir = SuperNovaCharacter->GetCharacterMovement()->IsFalling();
	bIsCrouched = SuperNovaCharacter->bIsCrouched;//��������ǿ��Ը��Ƶ�
	bIsAccelerating = SuperNovaCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
	CharacterState = SuperNovaCharacter->GetCharacterState();
	ActionState = SuperNovaCharacter->GetActionState();
	DeathPose = SuperNovaCharacter->GetDeathPose();
	bShootingWeaponEquipped = SuperNovaCharacter->IsShootingWeaponEquipped();
	SecondaryWeapon = SuperNovaCharacter->GetEquippedShootingWeapon();
	bAiming = SuperNovaCharacter->IsAiming();

	// ɨ��YawOffset
	//��ɫֱ������X�᷽��ʱ��Ϊ0
	FRotator AimRotation = SuperNovaCharacter->GetBaseAimRotation();
	//��ɫ�ƶ��ķ���ΪX�᷽��ʱ��Ϊ0
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(SuperNovaCharacter->GetVelocity());
	//YawOffset = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	//�����-180 ��180,�����ֵ�������� [-180,180]��ֵ ���������·��
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f);
	YawOffset = DeltaRotation.Yaw;//���ϱ�YawOffset���������AD���ذ�ɨ���ʱ�򶯻����ɸ�ƽ��

	//������бLean
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = SuperNovaCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	//�Ե�ǰ����бֵ Lean ��Ŀ��ֵ Target ���в�ֵ��ʹ��б�仯����ƽ����
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	//�� Interp ������ [-90��, 90��] ��Χ��
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	if (bShootingWeaponEquipped && SecondaryWeapon && SecondaryWeapon->GetItemMesh() && SuperNovaCharacter->GetMesh())
	{
		//���LeftHandSocket���������꣬LeftHandSocket����������������
		LeftHandTransform = SecondaryWeapon->GetItemMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		//������ʱ������Ӧ����������ֽ��е��������ƶ�,����ѡ��hand_r
		SuperNovaCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		//OutPosition �� OutRotation �����ֲ�������ֹ����ռ��е�λ�ú���ת��
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		//��ǹ�����ֹ�����Transform
		FTransform RightHandTransform = SecondaryWeapon->GetItemMesh()->GetSocketTransform(FName("Hand_R"), ERelativeTransformSpace::RTS_World);
		//���ֹ���Transform����׼Ŀ�����ת�����ֹ����ķ����������Ҫ�����෴�ķ���
		FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - SuperNovaCharacter->GetHitTarget()));
		//����ת���в�ֵ
		RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);

		//FTransform MuzzleTipTransform = SecondaryWeapon->GetItemMesh()->GetSocketTransform(FName("MuzzleFlash"), ERelativeTransformSpace::RTS_World);
		////����ֵ��MuzzleFlash X ������������ϵ�еķ�������
		//FVector MuzzleX(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X));
		//DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX * 1000.f, FColor::Red);
		//DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), SuperNovaCharacter->GetHitTarget(), FColor::Orange);

	}

	bUseFABRIK = SuperNovaCharacter->GetActionState() == EActionState::EAS_Unoccupied;
	bTransformRightHand = SuperNovaCharacter->GetActionState() == EActionState::EAS_Unoccupied;
}
