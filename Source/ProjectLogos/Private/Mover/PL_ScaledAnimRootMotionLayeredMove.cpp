#include "Mover/PL_ScaledAnimRootMotionLayeredMove.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "MotionWarpingComponent.h"
#include "MoveLibrary/MovementUtilsTypes.h"
#include "MoverComponent.h"
#include "MoverDataModelTypes.h"
#include "MoverSimulationTypes.h"
#include "Pawn/BasePawn.h"

FPL_ScaledAnimRootMotionLayeredMove::FPL_ScaledAnimRootMotionLayeredMove()
{
	DurationMs = 0.f;
	MixMode = EMoveMixMode::OverrideAll;
}

bool FPL_ScaledAnimRootMotionLayeredMove::GenerateMove(
	const FMoverTickStartData& StartState,
	const FMoverTimeStep& TimeStep,
	const UMoverComponent* MoverComp,
	UMoverBlackboard* SimBlackboard,
	FProposedMove& OutProposedMove
)
{
	if (!MoverComp) return false;

	const float DeltaSeconds = TimeStep.StepMs / 1000.f;
	if (DeltaSeconds <= UE_KINDA_SMALL_NUMBER) return false;

	const FMoverDefaultSyncState* SyncState =
		StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();

	if (!SyncState) return false;

	// Stop if the montage is no longer playing during normal simulation.
	if (!TimeStep.bIsResimulating)
	{
		bool bIsMontageStillPlaying = false;

		if (const USkeletalMeshComponent* MeshComp = Cast<USkeletalMeshComponent>(MoverComp->GetPrimaryVisualComponent()))
		{
			if (const UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
			{
				bIsMontageStillPlaying =
					MontageState.Montage && AnimInstance->Montage_IsPlaying(MontageState.Montage);
			}
		}

		if (!bIsMontageStillPlaying)
		{
			DurationMs = 0.f;
			return false;
		}
	}

	const double SecondsSinceMontageStarted = (TimeStep.BaseSimTimeMs - StartSimTimeMs) / 1000.0;
	const double ScaledSecondsSinceMontageStarted = SecondsSinceMontageStarted * MontageState.PlayRate;
	const float ExtractionStartPosition = MontageState.StartingMontagePosition + ScaledSecondsSinceMontageStarted;
	const bool bPastReleasePoint = bUseRootMotionRelease && ExtractionStartPosition >= RootMotionReleasePosition;
	const bool bShouldReleaseRootMotion = bPastReleasePoint && (!bRequireMoveInputForRootMotionRelease || HasMovementInput(StartState));
	
	if (bShouldReleaseRootMotion)
	{
		if (ABasePawn* BasePawn = Cast<ABasePawn>(MoverComp->GetOwner()))
		{
			// Only authority or owning client should publish this.
			// Simulated proxies wait for replication.
			if (BasePawn->HasAuthority() || BasePawn->IsLocallyControlled())
			{
				BasePawn->SetShouldBlendMontage(true);
			}
		}

		DurationMs = 0.f;
		return false;
	}

	const float ExtractionEndPosition = ExtractionStartPosition + (DeltaSeconds * MontageState.PlayRate);
	
	// Pull root motion from the montage for this simulation step.
	FTransform LocalRootMotion = MontageState.Montage ? UMotionWarpingUtilities::ExtractRootMotionFromAnimation(
		MontageState.Montage,
		ExtractionStartPosition,
		ExtractionEndPosition
	) : FTransform::Identity;

	float EffectiveTranslationScale = RootMotionTranslationScale;

	const FTransform SimActorTransform = FTransform(
		SyncState->GetOrientation_WorldSpace().Quaternion(),
		SyncState->GetLocation_WorldSpace()
	);

	FMotionWarpingUpdateContext WarpingContext;
	WarpingContext.Animation = MontageState.Montage;
	WarpingContext.CurrentPosition = ExtractionEndPosition;
	WarpingContext.PreviousPosition = ExtractionStartPosition;
	WarpingContext.PlayRate = MontageState.PlayRate;
	WarpingContext.Weight = 1.f;

	if (RootMotionCollisionStopMode != EPLRootMotionCollisionStopMode::None &&
		!FMath::IsNearlyZero(RootMotionTranslationScale))
	{
		FTransform CollisionProbeRootMotion = LocalRootMotion;
		CollisionProbeRootMotion.SetTranslation(
			CollisionProbeRootMotion.GetTranslation() * RootMotionTranslationScale
		);

		const FTransform CollisionProbeWorldRootMotion =
			MoverComp->ConvertLocalRootMotionToWorld(
				CollisionProbeRootMotion,
				DeltaSeconds,
				&SimActorTransform,
				&WarpingContext
			);

		if (HasBlockingPawnCollision(MoverComp, CollisionProbeWorldRootMotion.GetTranslation()))
		{
			// Keep montage rotation, but stop translation into another pawn.
			EffectiveTranslationScale = 0.f;
		}
	}

	if (!FMath::IsNearlyEqual(EffectiveTranslationScale, 1.f))
	{
		LocalRootMotion.SetTranslation(LocalRootMotion.GetTranslation() * EffectiveTranslationScale);
	}

	const FTransform WorldRootMotion =
		MoverComp->ConvertLocalRootMotionToWorld(
			LocalRootMotion,
			DeltaSeconds,
			&SimActorTransform,
			&WarpingContext
		);

	OutProposedMove = FProposedMove();
	OutProposedMove.MixMode = MixMode;

	// Mover wants velocity, not raw translation.
	OutProposedMove.LinearVelocity = WorldRootMotion.GetTranslation() / DeltaSeconds;
	OutProposedMove.AngularVelocityDegrees =
		FMath::RadiansToDegrees(WorldRootMotion.GetRotation().ToRotationVector() / DeltaSeconds);

	MontageState.CurrentPosition = ExtractionStartPosition;

	return true;
}

FLayeredMoveBase* FPL_ScaledAnimRootMotionLayeredMove::Clone() const
{
	return new FPL_ScaledAnimRootMotionLayeredMove(*this);
}

void FPL_ScaledAnimRootMotionLayeredMove::NetSerialize(FArchive& Ar)
{
	Super::NetSerialize(Ar);

	Ar << RootMotionTranslationScale;

	uint8 CollisionStopModeAsByte = static_cast<uint8>(RootMotionCollisionStopMode);
	Ar << CollisionStopModeAsByte;
	RootMotionCollisionStopMode = static_cast<EPLRootMotionCollisionStopMode>(CollisionStopModeAsByte);

	Ar << bUseRootMotionRelease;
	Ar << RootMotionReleasePosition;
	Ar << bRequireMoveInputForRootMotionRelease;
}

UScriptStruct* FPL_ScaledAnimRootMotionLayeredMove::GetScriptStruct() const
{
	return FPL_ScaledAnimRootMotionLayeredMove::StaticStruct();
}

FString FPL_ScaledAnimRootMotionLayeredMove::ToSimpleString() const
{
	return TEXT("PL_ScaledAnimRootMotion");
}

bool FPL_ScaledAnimRootMotionLayeredMove::HasMovementInput(const FMoverTickStartData& StartState) const
{
	const FCharacterDefaultInputs* CharacterInputs =
		StartState.InputCmd.InputCollection.FindDataByType<FCharacterDefaultInputs>();

	if (!CharacterInputs)
	{
		return false;
	}

	return !CharacterInputs->GetMoveInput().IsNearlyZero();
}

bool FPL_ScaledAnimRootMotionLayeredMove::HasBlockingPawnCollision(
	const UMoverComponent* MoverComp,
	const FVector& WorldTranslation
) const
{
	if (!MoverComp || WorldTranslation.IsNearlyZero()) return false;

	const UPrimitiveComponent* UpdatedPrimitive =
		Cast<UPrimitiveComponent>(MoverComp->GetUpdatedComponent());

	if (!UpdatedPrimitive) return false;

	const UWorld* World = UpdatedPrimitive->GetWorld();
	if (!World) return false;

	const FVector Start = UpdatedPrimitive->GetComponentLocation();
	const FVector End = Start + WorldTranslation;
	const FQuat Rotation = UpdatedPrimitive->GetComponentQuat();
	const FCollisionShape CollisionShape = UpdatedPrimitive->GetCollisionShape(2.f);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PLRootMotionCollisionStop), false);
	QueryParams.AddIgnoredActor(UpdatedPrimitive->GetOwner());

	TArray<FHitResult> Hits;
	if (!World->SweepMultiByChannel(
		Hits,
		Start,
		End,
		Rotation,
		UpdatedPrimitive->GetCollisionObjectType(),
		CollisionShape,
		QueryParams
	))
	{
		return false;
	}

	for (const FHitResult& Hit : Hits)
	{
		if (!Hit.bBlockingHit) continue;

		const AActor* HitActor = Hit.GetActor();
		if (!HitActor || !HitActor->IsA<APawn>()) continue;

		const UPrimitiveComponent* HitComponent = Hit.GetComponent();
		const bool bIsCapsule = HitComponent && HitComponent->IsA<UCapsuleComponent>();
		const bool bIsMesh = HitComponent && HitComponent->IsA<USkeletalMeshComponent>();

		switch (RootMotionCollisionStopMode)
		{
		case EPLRootMotionCollisionStopMode::Capsule:
			return bIsCapsule;

		case EPLRootMotionCollisionStopMode::Mesh:
			return bIsMesh;

		case EPLRootMotionCollisionStopMode::CapsuleOrMesh:
			return bIsCapsule || bIsMesh;

		case EPLRootMotionCollisionStopMode::None:
		default:
			return false;
		}
	}

	return false;
}
