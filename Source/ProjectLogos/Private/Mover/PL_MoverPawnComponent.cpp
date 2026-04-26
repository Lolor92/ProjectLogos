#include "Mover/PL_MoverPawnComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "MoverDataModelTypes.h"

UPL_MoverPawnComponent::UPL_MoverPawnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

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

	// Do not SetActorRotation immediately here.
	// Mover can overwrite it later in the same frame.
	QueueFacingSnapAfterMover(Yaw);

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn && OwnerPawn->IsLocallyControlled() && !OwnerPawn->HasAuthority())
	{
		ServerRequestForcedFacingYaw(Yaw);
	}
}

void UPL_MoverPawnComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bHasPendingFacingSnap)
	{
		SetComponentTickEnabled(false);
		return;
	}

	const float YawToApply = PendingFacingSnapYaw;

	bHasPendingFacingSnap = false;
	PendingFacingSnapYaw = 0.f;

	ApplyFacingSnapOnce(YawToApply);

	SetComponentTickEnabled(false);
}

void UPL_MoverPawnComponent::ServerRequestForcedFacingYaw_Implementation(float Yaw)
{
	Yaw = FRotator::NormalizeAxis(Yaw);

	const FRotator YawRotation(0.f, Yaw, 0.f);

	ForcedFacingIntent = YawRotation.Vector();
	bHasForcedFacingIntent = true;

	QueueFacingSnapAfterMover(Yaw);
}

void UPL_MoverPawnComponent::QueueFacingSnapAfterMover(float Yaw)
{
	PendingFacingSnapYaw = FRotator::NormalizeAxis(Yaw);
	bHasPendingFacingSnap = true;

	SetComponentTickEnabled(true);
}

void UPL_MoverPawnComponent::ApplyFacingSnapOnce(float Yaw) const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	const FRotator NewRotation(0.f, FRotator::NormalizeAxis(Yaw), 0.f);

	if (UpdatedComponent)
	{
		UpdatedComponent->SetWorldRotation(
			NewRotation,
			false,
			nullptr,
			ETeleportType::ResetPhysics
		);
	}
	else
	{
		OwnerActor->SetActorRotation(NewRotation, ETeleportType::ResetPhysics);
	}

	if (OwnerActor->HasAuthority())
	{
		OwnerActor->ForceNetUpdate();
	}
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
