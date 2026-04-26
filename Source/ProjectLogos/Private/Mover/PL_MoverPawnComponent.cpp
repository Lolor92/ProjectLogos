#include "Mover/PL_MoverPawnComponent.h"
#include "Backends/MoverBackendLiaison.h"
#include "Components/SkeletalMeshComponent.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "MoverComponent.h"
#include "MoverDataModelTypes.h"
#include "MoverSimulationTypes.h"

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

	if (CharacterMoverComponent)
	{
		CharacterMoverComponent->bAcceptExternalMovement = true;
		CharacterMoverComponent->bWarnOnExternalMovement = false;
	}

	FRotator NewRotation = OwnerActor->GetActorRotation();
	NewRotation.Yaw = FRotator::NormalizeAxis(Yaw);

	OwnerActor->SetActorRotation(NewRotation, ETeleportType::TeleportPhysics);

	// This is the missing piece:
	// push the new actor/root rotation into Mover's pending sync state.
	WriteCurrentTransformToMoverSyncState();

	UE_LOG(
		LogTemp,
		Warning,
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
			LogTemp,
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
			LogTemp,
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
			LogTemp,
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
		DefaultSyncState->GetMovementBase(),
		DefaultSyncState->GetMovementBaseBoneName()
	);

	LiaisonComponent->WritePendingSyncState(PendingSyncState);

	UE_LOG(
		LogTemp,
		Warning,
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
