#include "AnimInstance/PL_AnimInstance.h"
#include "Kismet/KismetMathLibrary.h"
#include "Pawn/BasePawn.h"

void UPL_AnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// Cache the owning pawn once when the animation instance starts.
	OwningBasePawn = Cast<ABasePawn>(TryGetPawnOwner());
}

void UPL_AnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwningBasePawn)
	{
		// The pawn may not be ready during the first animation update.
		OwningBasePawn = Cast<ABasePawn>(TryGetPawnOwner());
	}

	if (!OwningBasePawn)
	{
		GroundSpeed = 0.f;
		bIsMoving = false;
		bIsAccelerating = false;
		bIsInAir = false;
		bIsAirborne = false;
		MovementDirectionYaw = 0.f;
		return;
	}

	GroundSpeed = OwningBasePawn->GetGroundSpeed();
	bIsMoving = OwningBasePawn->IsMoving();
	bIsAccelerating = OwningBasePawn->IsAcceleratingForAnimation();
	
	bIsInAir = OwningBasePawn->IsMoverFalling();
	
	bIsAirborne = OwningBasePawn->IsMoverAirborne();

	if (!bIsMoving)
	{
		MovementDirectionYaw = 0.f;
	}
	else
	{
		const FVector Velocity = OwningBasePawn->GetMoverVelocity();
		const FRotator MovementRotation = Velocity.ToOrientationRotator();
		const FRotator ActorRotation = OwningBasePawn->GetActorRotation();

		MovementDirectionYaw = UKismetMathLibrary::NormalizedDeltaRotator(
			MovementRotation,
			ActorRotation
		).Yaw;
	}

	OwningBasePawn->EnsureAbilityMontageVisual();
}

void UPL_AnimInstance::SetAbilityAnimState(
	bool bNewAbilityRootMotionActive,
	bool bNewShouldBlendMontage
)
{
	bAbilityRootMotionActive = bNewAbilityRootMotionActive;
	bShouldBlendMontage = bNewShouldBlendMontage;
}
