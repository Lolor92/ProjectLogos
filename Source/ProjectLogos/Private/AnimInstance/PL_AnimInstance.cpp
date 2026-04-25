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
		return;
	}

	GroundSpeed = OwningBasePawn->GetGroundSpeed();
	bIsMoving = OwningBasePawn->IsMoving();

	if (!bIsMoving)
	{
		// No movement, no direction to calculate.
		MovementDirectionYaw = 0.f;
		return;
	}

	const FVector Velocity = OwningBasePawn->GetMoverVelocity();
	const FRotator MovementRotation = Velocity.ToOrientationRotator();
	const FRotator ActorRotation = OwningBasePawn->GetActorRotation();

	// Difference between where we move and where the pawn faces.
	MovementDirectionYaw = UKismetMathLibrary::NormalizedDeltaRotator(
		MovementRotation, ActorRotation).Yaw;

	if (bShouldBlendMontage)
	{
		UAnimMontage* CurrentMontage = GetCurrentActiveMontage();

		float MontagePosition = -1.f;
		float MontageLength = -1.f;
		float MontageWeight = -1.f;

		if (CurrentMontage)
		{
			MontagePosition = Montage_GetPosition(CurrentMontage);
			MontageLength = CurrentMontage->GetPlayLength();

			if (FAnimMontageInstance* MontageInstance =
				GetActiveInstanceForMontage(CurrentMontage))
			{
				MontageWeight = MontageInstance->GetWeight();
			}
		}

		const APawn* PawnOwner = TryGetPawnOwner();

		UE_LOG(LogTemp, Warning,
			TEXT("ANIM_BLEND_TICK Pawn=%s Local=%d Blend=%d Montage=%s Pos=%.3f Len=%.3f Weight=%.3f"),
			*GetNameSafe(PawnOwner),
			PawnOwner ? PawnOwner->IsLocallyControlled() : false,
			bShouldBlendMontage,
			*GetNameSafe(CurrentMontage),
			MontagePosition,
			MontageLength,
			MontageWeight
		);
	}
}

void UPL_AnimInstance::SetShouldBlendMontage(bool bNewShouldBlendMontage)
{
	bShouldBlendMontage = bNewShouldBlendMontage;
}
