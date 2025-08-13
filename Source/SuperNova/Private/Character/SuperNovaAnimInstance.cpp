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
	bIsCrouched = SuperNovaCharacter->bIsCrouched;//这个变量是可以复制的
	bIsAccelerating = SuperNovaCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
	CharacterState = SuperNovaCharacter->GetCharacterState();
	ActionState = SuperNovaCharacter->GetActionState();
	DeathPose = SuperNovaCharacter->GetDeathPose();
	bShootingWeaponEquipped = SuperNovaCharacter->IsShootingWeaponEquipped();
	SecondaryWeapon = SuperNovaCharacter->GetEquippedShootingWeapon();
	bAiming = SuperNovaCharacter->IsAiming();

	// 扫射YawOffset
	//角色直视世界X轴方向时才为0
	FRotator AimRotation = SuperNovaCharacter->GetBaseAimRotation();
	//角色移动的方向为X轴方向时才为0
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(SuperNovaCharacter->GetVelocity());
	//YawOffset = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	//如果是-180 到180,这个插值函数不会 [-180,180]插值 而是走最短路径
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f);
	YawOffset = DeltaRotation.Yaw;//跟上边YawOffset的区别就是AD来回按扫射的时候动画过渡更平滑

	//身体倾斜Lean
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = SuperNovaCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	//对当前的倾斜值 Lean 和目标值 Target 进行插值，使倾斜变化更加平滑。
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	//将 Interp 限制在 [-90°, 90°] 范围内
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	if (bShootingWeaponEquipped && SecondaryWeapon && SecondaryWeapon->GetItemMesh() && SuperNovaCharacter->GetMesh())
	{
		//获得LeftHandSocket的世界坐标，LeftHandSocket在武器骨骼中设置
		LeftHandTransform = SecondaryWeapon->GetItemMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		//在运行时武器不应该相对于右手进行调整或者移动,所以选择hand_r
		SuperNovaCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		//OutPosition 和 OutRotation 是左手插槽在右手骨骼空间中的位置和旋转。
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		//持枪人右手骨骼的Transform
		FTransform RightHandTransform = SecondaryWeapon->GetItemMesh()->GetSocketTransform(FName("Hand_R"), ERelativeTransformSpace::RTS_World);
		//右手骨骼Transform到瞄准目标的旋转（右手骨骼的方向向里，所以要看向相反的方向）
		FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - SuperNovaCharacter->GetHitTarget()));
		//对旋转进行插值
		RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);

		//FTransform MuzzleTipTransform = SecondaryWeapon->GetItemMesh()->GetSocketTransform(FName("MuzzleFlash"), ERelativeTransformSpace::RTS_World);
		////返回值是MuzzleFlash X 轴在世界坐标系中的方向向量
		//FVector MuzzleX(FRotationMatrix(MuzzleTipTransform.GetRotation().Rotator()).GetUnitAxis(EAxis::X));
		//DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), MuzzleTipTransform.GetLocation() + MuzzleX * 1000.f, FColor::Red);
		//DrawDebugLine(GetWorld(), MuzzleTipTransform.GetLocation(), SuperNovaCharacter->GetHitTarget(), FColor::Orange);

	}

	bUseFABRIK = SuperNovaCharacter->GetActionState() == EActionState::EAS_Unoccupied;
	bTransformRightHand = SuperNovaCharacter->GetActionState() == EActionState::EAS_Unoccupied;
}
