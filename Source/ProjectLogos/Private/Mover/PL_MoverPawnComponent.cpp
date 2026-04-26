#include "Mover/PL_MoverPawnComponent.h"
#include "Backends/MoverBackendLiaison.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "MoverComponent.h"
#include "MoverDataModelTypes.h"
#include "MoverSimulationTypes.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogPLMoverFacing, Log, All);

UPL_MoverPawnComponent::UPL_MoverPawnComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPL_MoverPawnComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolveOwnerComponents();

	if (!CharacterMoverComponent) return;

	// Tell Mover what component it should physically move.
	if (UpdatedComponent)
	{
		CharacterMoverComponent->SetUpdatedComponent(UpdatedComponent);
	}

	if (CharacterMoverComponent)
	{
		// We sometimes do intentional one-shot external snaps,
		// for example facing camera yaw when an ability activates.
		CharacterMoverComponent->bAcceptExternalMovement = true;
		CharacterMoverComponent->bWarnOnExternalMovement = false;
	}

	// Tell Mover what component represents the character visually.
	if (PrimaryVisualComponent)
	{
		CharacterMoverComponent->SetPrimaryVisualComponent(PrimaryVisualComponent);
	}
}

void UPL_MoverPawnComponent::ResolveOwnerComponents()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return;

	// Find the Mover component on the pawn.
	CharacterMoverComponent = OwnerActor->FindComponentByClass<UCharacterMoverComponent>();

	// Usually the capsule/root is what Mover should move.
	UpdatedComponent = OwnerActor->GetRootComponent();

	// Usually the skeletal mesh is the visual component.
	PrimaryVisualComponent = OwnerActor->FindComponentByClass<USkeletalMeshComponent>();
}

void UPL_MoverPawnComponent::RequestMoveIntent(const FVector& MoveIntent)
{
	// Cache input until Mover asks for it.
	CachedMoveInputIntent = MoveIntent;
}

void UPL_MoverPawnComponent::ApplyAuthoritativeExternalTransformSnap(
	const FVector& NewLocation,
	const FRotator& NewRotation,
	const bool bApplyLocation,
	const bool bApplyRotation,
	const bool bSweep,
	const ETeleportType TeleportType
)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	// Server authoritative result.
	ApplyExternalTransformSnap(
		NewLocation,
		NewRotation,
		bApplyLocation,
		bApplyRotation,
		bSweep,
		TeleportType
	);

	// The owning client is an autonomous proxy and may keep predicting with old local state.
	// Send the same one-shot transform directly to that owner.
	const APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	if (OwnerPawn && OwnerPawn->GetController() && !OwnerPawn->GetController()->IsLocalController())
	{
		ClientApplyExternalTransformSnap(
			NewLocation,
			NewRotation,
			bApplyLocation,
			bApplyRotation,
			bSweep,
			TeleportType
		);
	}
}

void UPL_MoverPawnComponent::ApplyAuthoritativeHitStop(
	const float Duration,
	const float TimeScale,
	const bool bAffectAnimation,
	const bool bAffectMoverRootMotion
)
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	if (Duration <= 0.f || TimeScale >= 1.f)
	{
		return;
	}

	// Server executes this too, and relevant clients receive it.
	MulticastApplyHitStop(
		Duration,
		TimeScale,
		bAffectAnimation,
		bAffectMoverRootMotion
	);
}

void UPL_MoverPawnComponent::ClientApplyExternalTransformSnap_Implementation(
	FVector NewLocation,
	FRotator NewRotation,
	bool bApplyLocation,
	bool bApplyRotation,
	bool bSweep,
	ETeleportType TeleportType
)
{
	ApplyExternalTransformSnap(
		NewLocation,
		NewRotation,
		bApplyLocation,
		bApplyRotation,
		bSweep,
		TeleportType
	);
}

void UPL_MoverPawnComponent::MulticastApplyHitStop_Implementation(
	const float Duration,
	const float TimeScale,
	const bool bAffectAnimation,
	const bool bAffectMoverRootMotion
)
{
	ApplyHitStopLocal(
		Duration,
		TimeScale,
		bAffectAnimation,
		bAffectMoverRootMotion
	);
}

void UPL_MoverPawnComponent::ApplyHitStopLocal(
	const float Duration,
	const float TimeScale,
	const bool bAffectAnimation,
	const bool bAffectMoverRootMotion
)
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();

	if (!OwnerActor || !World || Duration <= 0.f)
	{
		return;
	}

	const float ClampedTimeScale = FMath::Clamp(TimeScale, 0.f, 1.f);

	++HitStopSerial;
	const uint32 ThisSerial = HitStopSerial;

	if (bAffectMoverRootMotion)
	{
		HitStopRootMotionTimeScale = ClampedTimeScale;
	}

	if (bAffectAnimation)
	{
		if (USkeletalMeshComponent* Mesh = OwnerActor->FindComponentByClass<USkeletalMeshComponent>())
		{
			if (!bHasSavedGlobalAnimRateScale)
			{
				SavedGlobalAnimRateScale = Mesh->GlobalAnimRateScale;
				bHasSavedGlobalAnimRateScale = true;
			}

			Mesh->GlobalAnimRateScale = ClampedTimeScale;
		}
	}

	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &UPL_MoverPawnComponent::FinishHitStop, ThisSerial);

	World->GetTimerManager().ClearTimer(HitStopTimerHandle);
	World->GetTimerManager().SetTimer(
		HitStopTimerHandle,
		TimerDelegate,
		Duration,
		false
	);
}

bool UPL_MoverPawnComponent::IsHitStopActive() const
{
	const UWorld* World = GetWorld();

	const bool bTimerActive = World
		&& World->GetTimerManager().IsTimerActive(HitStopTimerHandle);

	return bTimerActive
		|| HitStopRootMotionTimeScale < 1.f
		|| bHasSavedGlobalAnimRateScale;
}

void UPL_MoverPawnComponent::CancelHitStop()
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(HitStopTimerHandle);
	}

	++HitStopSerial;

	HitStopRootMotionTimeScale = 1.f;

	AActor* OwnerActor = GetOwner();
	if (OwnerActor)
	{
		if (USkeletalMeshComponent* Mesh = OwnerActor->FindComponentByClass<USkeletalMeshComponent>())
		{
			if (bHasSavedGlobalAnimRateScale)
			{
				Mesh->GlobalAnimRateScale = SavedGlobalAnimRateScale;
			}
			else
			{
				Mesh->GlobalAnimRateScale = 1.f;
			}
		}
	}

	bHasSavedGlobalAnimRateScale = false;
	SavedGlobalAnimRateScale = 1.f;
}

void UPL_MoverPawnComponent::CancelHitStopFromAbilityStart()
{
	if (!IsHitStopActive())
	{
		return;
	}

	const AActor* OwnerActor = GetOwner();

	if (OwnerActor && OwnerActor->HasAuthority())
	{
		MulticastCancelHitStop();
		return;
	}

	// Local predicted abilities can cancel immediately on the owning client.
	CancelHitStop();
}

void UPL_MoverPawnComponent::MulticastCancelHitStop_Implementation()
{
	CancelHitStop();
}

void UPL_MoverPawnComponent::FinishHitStop(const uint32 Serial)
{
	if (Serial != HitStopSerial)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();

	HitStopRootMotionTimeScale = 1.f;

	if (OwnerActor)
	{
		if (USkeletalMeshComponent* Mesh = OwnerActor->FindComponentByClass<USkeletalMeshComponent>())
		{
			if (bHasSavedGlobalAnimRateScale)
			{
				Mesh->GlobalAnimRateScale = SavedGlobalAnimRateScale;
			}
			else
			{
				Mesh->GlobalAnimRateScale = 1.f;
			}
		}
	}

	bHasSavedGlobalAnimRateScale = false;
	SavedGlobalAnimRateScale = 1.f;
}

void UPL_MoverPawnComponent::RequestForcedFacingYaw(float Yaw)
{
	Yaw = FRotator::NormalizeAxis(Yaw);

	const FRotator YawRotation(0.f, Yaw, 0.f);

	// One Mover input hint, consumed once in ProduceInput.
	ForcedFacingIntent = YawRotation.Vector();
	bHasForcedFacingIntent = true;

	// One immediate external snap.
	ApplyFacingSnapOnce(Yaw);

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn && OwnerPawn->IsLocallyControlled() && !OwnerPawn->HasAuthority())
	{
		ServerRequestForcedFacingYaw(Yaw);
	}
}

void UPL_MoverPawnComponent::ServerRequestForcedFacingYaw_Implementation(float Yaw)
{
	Yaw = FRotator::NormalizeAxis(Yaw);

	const FRotator YawRotation(0.f, Yaw, 0.f);

	ForcedFacingIntent = YawRotation.Vector();
	bHasForcedFacingIntent = true;

	ApplyFacingSnapOnce(Yaw);
}

void UPL_MoverPawnComponent::ApplyFacingSnapOnce(float Yaw) const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	FRotator NewRotation = OwnerActor->GetActorRotation();
	NewRotation.Yaw = FRotator::NormalizeAxis(Yaw);

	OwnerActor->SetActorRotation(NewRotation, ETeleportType::TeleportPhysics);

	// This is the missing piece:
	// push the new actor/root rotation into Mover's pending sync state.
	WriteCurrentTransformToMoverSyncState();

	UE_LOG(
		LogPLMoverFacing,
		VeryVerbose,
		TEXT("Facing snap applied. Actor=%s DesiredYaw=%.2f ActorYaw=%.2f Authority=%d Local=%d"),
		*GetNameSafe(OwnerActor),
		Yaw,
		OwnerActor->GetActorRotation().Yaw,
		OwnerActor->HasAuthority(),
		Cast<APawn>(OwnerActor) ? Cast<APawn>(OwnerActor)->IsLocallyControlled() : false
	);

	if (OwnerActor->HasAuthority())
	{
		OwnerActor->ForceNetUpdate();
	}
}

void UPL_MoverPawnComponent::ApplyExternalTransformSnap(
	const FVector& NewLocation,
	const FRotator& NewRotation,
	const bool bApplyLocation,
	const bool bApplyRotation,
	const bool bSweep,
	const ETeleportType TeleportType
) const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	USceneComponent* MoveComponent = UpdatedComponent.Get();
	if (!MoveComponent)
	{
		MoveComponent = OwnerActor->GetRootComponent();
	}

	if (!MoveComponent)
	{
		return;
	}

	const FVector FinalLocation = bApplyLocation
		? NewLocation
		: MoveComponent->GetComponentLocation();

	const FRotator FinalRotation = bApplyRotation
		? NewRotation
		: MoveComponent->GetComponentRotation();

	FHitResult SweepHit;

	MoveComponent->SetWorldLocationAndRotation(
		FinalLocation,
		FinalRotation,
		bSweep,
		&SweepHit,
		TeleportType
	);

	WriteCurrentTransformToMoverSyncState();

	if (OwnerActor->HasAuthority())
	{
		OwnerActor->ForceNetUpdate();
	}
}

void UPL_MoverPawnComponent::WriteCurrentTransformToMoverSyncState() const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !UpdatedComponent)
	{
		return;
	}

	UActorComponent* LiaisonActorComponent =
		OwnerActor->FindComponentByInterface(UMoverBackendLiaisonInterface::StaticClass());
	IMoverBackendLiaisonInterface* LiaisonComponent =
		Cast<IMoverBackendLiaisonInterface>(LiaisonActorComponent);

	if (!LiaisonComponent)
	{
		UE_LOG(
			LogPLMoverFacing,
			Warning,
			TEXT("Facing snap could not update Mover sync state. No backend liaison found on %s."),
			*GetNameSafe(OwnerActor)
		);
		return;
	}

	FMoverSyncState PendingSyncState;
	if (!LiaisonComponent->ReadPendingSyncState(PendingSyncState))
	{
		UE_LOG(
			LogPLMoverFacing,
			Warning,
			TEXT("Facing snap could not read pending Mover sync state. Actor=%s"),
			*GetNameSafe(OwnerActor)
		);
		return;
	}

	FMoverDefaultSyncState* DefaultSyncState =
		PendingSyncState.SyncStateCollection.FindMutableDataByType<FMoverDefaultSyncState>();

	if (!DefaultSyncState)
	{
		UE_LOG(
			LogPLMoverFacing,
			Warning,
			TEXT("Facing snap could not find FMoverDefaultSyncState. Actor=%s"),
			*GetNameSafe(OwnerActor)
		);
		return;
	}

	DefaultSyncState->SetTransforms_WorldSpace(
		UpdatedComponent->GetComponentLocation(),
		UpdatedComponent->GetComponentRotation(),
		DefaultSyncState->GetVelocity_WorldSpace(),
		FVector::ZeroVector, // World angular velocity, degrees/sec
		DefaultSyncState->GetMovementBase(),
		DefaultSyncState->GetMovementBaseBoneName()
	);

	LiaisonComponent->WritePendingSyncState(PendingSyncState);

	UE_LOG(
		LogPLMoverFacing,
		VeryVerbose,
		TEXT("Facing snap wrote Mover sync state. Actor=%s SyncYaw=%.2f"),
		*GetNameSafe(OwnerActor),
		UpdatedComponent->GetComponentRotation().Yaw
	);
}

void UPL_MoverPawnComponent::ClearMoveIntent()
{
	// No input means no movement intent.
	CachedMoveInputIntent = FVector::ZeroVector;
}

void UPL_MoverPawnComponent::ProduceInput_Implementation(
	int32 SimTimeMs,
	FMoverInputCmdContext& InputCmdResult
)
{
	// This is the default input struct used by CharacterMoverComponent.
	FCharacterDefaultInputs& CharacterInputs =
		InputCmdResult.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();
	
	// Reset every frame so old facing data does not linger.
	CharacterInputs.OrientationIntent = FVector::ZeroVector;

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const APlayerController* PlayerController =
		OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;

	if (!PlayerController)
	{
		// NPCs or non-player pawns can still use raw world input.
		CharacterInputs.SetMoveInput(EMoveInputType::DirectionalIntent, CachedMoveInputIntent);

		if (CachedMoveInputIntent.SizeSquared2D() > 0.01f)
		{
			// Face the direction we are moving.
			CharacterInputs.OrientationIntent = CachedMoveInputIntent.GetSafeNormal2D();
		}

		if (bHasForcedFacingIntent)
		{
			CharacterInputs.OrientationIntent = ForcedFacingIntent;
			bHasForcedFacingIntent = false;
			ForcedFacingIntent = FVector::ZeroVector;
		}

		return;
	}

	const FRotator ControlRotation = PlayerController->GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

	// Convert local input into camera-relative world movement.
	const FVector WorldMoveIntent = YawRotation.RotateVector(CachedMoveInputIntent);

	CharacterInputs.ControlRotation = ControlRotation;
	CharacterInputs.SetMoveInput(EMoveInputType::DirectionalIntent, WorldMoveIntent);

	if (WorldMoveIntent.SizeSquared2D() > 0.01f)
	{
		// Player can either face camera yaw or movement direction.
		CharacterInputs.OrientationIntent = bOrientToCameraYaw
			? YawRotation.Vector()
			: WorldMoveIntent.GetSafeNormal2D();
	}

	if (bHasForcedFacingIntent)
	{
		CharacterInputs.OrientationIntent = ForcedFacingIntent;
		bHasForcedFacingIntent = false;
		ForcedFacingIntent = FVector::ZeroVector;
	}
}
