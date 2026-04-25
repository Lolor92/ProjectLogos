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

		const ENetMode NetMode = GetWorld() ? GetWorld()->GetNetMode() : NM_MAX;

		const TCHAR* NetModeText = TEXT("Unknown");
		switch (NetMode)
		{
		case NM_Standalone: NetModeText = TEXT("Standalone"); break;
		case NM_DedicatedServer: NetModeText = TEXT("DedicatedServer"); break;
		case NM_ListenServer: NetModeText = TEXT("ListenServer"); break;
		case NM_Client: NetModeText = TEXT("Client"); break;
		default: break;
		}

		const APawn* PawnOwner = TryGetPawnOwner();

		UE_LOG(LogTemp, Warning,
			TEXT("ANIM_BLEND_TICK World=%s Anim=%s Pawn=%s Authority=%d Local=%d Blend=%d Montage=%s Pos=%.3f Len=%.3f Weight=%.3f"),
			NetModeText,
			*GetNameSafe(this),
			*GetNameSafe(PawnOwner),
			PawnOwner ? PawnOwner->HasAuthority() : false,
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

void UPL_AnimInstance::SetAbilityAnimState(
	bool bNewAbilityRootMotionActive,
	bool bNewShouldBlendMontage
)
{
	bAbilityRootMotionActive = bNewAbilityRootMotionActive;
	bShouldBlendMontage = bNewShouldBlendMontage;

	UE_LOG(LogTemp, Warning,
		TEXT("ANIM_SET_ABILITY_STATE Pawn=%s Local=%d RMActive=%d Blend=%d"),
		*GetNameSafe(TryGetPawnOwner()),
		TryGetPawnOwner() ? TryGetPawnOwner()->IsLocallyControlled() : false,
		bAbilityRootMotionActive,
		bShouldBlendMontage
	);
}
