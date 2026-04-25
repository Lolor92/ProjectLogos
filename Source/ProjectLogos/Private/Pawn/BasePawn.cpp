#include "Pawn/BasePawn.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/GameStateBase.h"
#include "Combat/Components/PL_CombatComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "GAS/Component/PL_AbilitySystemComponent.h"
#include "Mover/PL_MoverPawnComponent.h"
#include "AnimInstance/PL_AnimInstance.h"
#include "Net/UnrealNetwork.h"


ABasePawn::ABasePawn()
{
	PrimaryActorTick.bCanEverTick = false;

	// Mover handles movement replication.
	bReplicates = true;
	SetReplicateMovement(false);

	// Main collision body. Mover moves this.
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	SetRootComponent(CapsuleComponent);

	CapsuleComponent->InitCapsuleSize(42.f, 90.f);
	CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CapsuleComponent->SetCollisionObjectType(ECC_Pawn);
	CapsuleComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CapsuleComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	// Visible character body.
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CapsuleComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Default mannequin-style offset.
	MeshComponent->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	MeshComponent->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	// Mover movement component.
	CharacterMoverComponent = CreateDefaultSubobject<UCharacterMoverComponent>(TEXT("CharacterMoverComponent"));

	// Tell Mover what to move and what the visual body is.
	CharacterMoverComponent->SetUpdatedComponent(CapsuleComponent);
	CharacterMoverComponent->SetPrimaryVisualComponent(MeshComponent);

	// Produces input for Mover.
	MoverPawnComponent = CreateDefaultSubobject<UPL_MoverPawnComponent>(TEXT("MoverPawnComponent"));

	// Owns combat setup and ability grants.
	CombatComponent = CreateDefaultSubobject<UPL_CombatComponent>(TEXT("CombatComponent"));
}

void ABasePawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABasePawn, AbilityAnimState);
	DOREPLIFETIME_CONDITION(ABasePawn, AbilityMontageVisualState, COND_SkipOwner);
}

UAbilitySystemComponent* ABasePawn::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABasePawn::InitializeAbilitySystem(
	UPL_AbilitySystemComponent* InAbilitySystemComponent,
	AActor* OwnerActor
)
{
	if (!InAbilitySystemComponent || !OwnerActor) return;

	AbilitySystemComponent = InAbilitySystemComponent;

	// For players:
	// OwnerActor = PlayerState
	// AvatarActor = this pawn
	AbilitySystemComponent->InitAbilityActorInfo(OwnerActor, this);
}

FVector ABasePawn::GetMoverVelocity() const
{
	if (!CharacterMoverComponent) return FVector::ZeroVector;

	// Mover owns the movement velocity.
	return CharacterMoverComponent->GetVelocity();
}

float ABasePawn::GetGroundSpeed() const
{
	// Horizontal speed only. Useful for locomotion animation.
	return GetMoverVelocity().Size2D();
}

bool ABasePawn::IsMoving() const
{
	// True when horizontal velocity is meaningful.
	return GetGroundSpeed() > KINDA_SMALL_NUMBER;
}

void ABasePawn::SetAbilityAnimState(const FPLRepAbilityAnimState& NewState)
{
	UE_LOG(LogTemp, Warning,
		TEXT("SET_ABILITY_ANIM_STATE Pawn=%s Authority=%d Local=%d RMActive %d->%d Blend %d->%d"),
		*GetNameSafe(this),
		HasAuthority(),
		IsLocallyControlled(),
		AbilityAnimState.bAbilityRootMotionActive,
		NewState.bAbilityRootMotionActive,
		AbilityAnimState.bShouldBlendMontage,
		NewState.bShouldBlendMontage
	);
	
	if (AbilityAnimState == NewState)
	{
		return;
	}

	AbilityAnimState = NewState;

	ApplyAbilityAnimState(AbilityAnimState);

	if (!HasAuthority())
	{
		if (IsLocallyControlled())
		{
			ServerSetAbilityAnimState(AbilityAnimState);
		}

		return;
	}

	ForceNetUpdate();
}

void ABasePawn::SetAbilityAnimStateValues(
	bool bNewAbilityRootMotionActive,
	bool bNewShouldBlendMontage
)
{
	FPLRepAbilityAnimState NewState;
	NewState.bAbilityRootMotionActive = bNewAbilityRootMotionActive;
	NewState.bShouldBlendMontage = bNewShouldBlendMontage;

	SetAbilityAnimState(NewState);
}

void ABasePawn::SetShouldBlendMontage(bool bNewShouldBlendMontage)
{
	FPLRepAbilityAnimState NewState = AbilityAnimState;
	NewState.bShouldBlendMontage = bNewShouldBlendMontage;

	SetAbilityAnimState(NewState);
}

void ABasePawn::ResetAbilityAnimState()
{
	FPLRepAbilityAnimState DefaultState;
	SetAbilityAnimState(DefaultState);
}

void ABasePawn::ServerSetAbilityAnimState_Implementation(const FPLRepAbilityAnimState& NewState)
{
	UE_LOG(LogTemp, Warning,
		TEXT("SERVER_SET_ABILITY_ANIM_STATE Pawn=%s Authority=%d Local=%d RMActive %d->%d Blend %d->%d"),
		*GetNameSafe(this),
		HasAuthority(),
		IsLocallyControlled(),
		AbilityAnimState.bAbilityRootMotionActive,
		NewState.bAbilityRootMotionActive,
		AbilityAnimState.bShouldBlendMontage,
		NewState.bShouldBlendMontage
	);
	
	if (AbilityAnimState == NewState)
	{
		return;
	}

	AbilityAnimState = NewState;

	ApplyAbilityAnimState(AbilityAnimState);
	ForceNetUpdate();
}

void ABasePawn::OnRep_AbilityAnimState()
{
	UE_LOG(LogTemp, Warning,
		TEXT("ONREP_ABILITY_ANIM_STATE Pawn=%s Authority=%d Local=%d RMActive=%d Blend=%d"),
		*GetNameSafe(this),
		HasAuthority(),
		IsLocallyControlled(),
		AbilityAnimState.bAbilityRootMotionActive,
		AbilityAnimState.bShouldBlendMontage
	);
	
	ApplyAbilityAnimState(AbilityAnimState);
}

void ABasePawn::ApplyAbilityAnimState(const FPLRepAbilityAnimState& NewState)
{
	if (!MeshComponent)
	{
		return;
	}

	UPL_AnimInstance* PLAnimInstance =
		Cast<UPL_AnimInstance>(MeshComponent->GetAnimInstance());

	if (!PLAnimInstance)
	{
		return;
	}

	PLAnimInstance->SetAbilityAnimState(
		NewState.bAbilityRootMotionActive,
		NewState.bShouldBlendMontage
	);

	UAnimMontage* CurrentMontage = PLAnimInstance->GetCurrentActiveMontage();

	float MontagePosition = -1.f;
	float MontageLength = -1.f;
	float MontageWeight = -1.f;

	if (CurrentMontage)
	{
		MontagePosition = PLAnimInstance->Montage_GetPosition(CurrentMontage);
		MontageLength = CurrentMontage->GetPlayLength();

		if (FAnimMontageInstance* MontageInstance =
			PLAnimInstance->GetActiveInstanceForMontage(CurrentMontage))
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

	UE_LOG(LogTemp, Warning,
		TEXT("APPLY_ABILITY_ANIM_STATE World=%s Pawn=%s Anim=%s Authority=%d Local=%d RMActive=%d Blend=%d Montage=%s Pos=%.3f Len=%.3f Weight=%.3f"),
		NetModeText,
		*GetNameSafe(this),
		*GetNameSafe(PLAnimInstance),
		HasAuthority(),
		IsLocallyControlled(),
		NewState.bAbilityRootMotionActive,
		NewState.bShouldBlendMontage,
		*GetNameSafe(CurrentMontage),
		MontagePosition,
		MontageLength,
		MontageWeight
	);
}

float ABasePawn::GetServerTimeSecondsSafe() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}

	if (const AGameStateBase* GameState = World->GetGameState())
	{
		return GameState->GetServerWorldTimeSeconds();
	}

	return World->GetTimeSeconds();
}

void ABasePawn::StartAbilityMontageVisual(
	UAnimMontage* Montage,
	float PlayRate,
	float StartPosition
)
{
	if (!HasAuthority() || !Montage)
	{
		return;
	}

	AbilityMontageVisualState.Montage = Montage;
	AbilityMontageVisualState.PlayRate = PlayRate;
	AbilityMontageVisualState.StartPosition = StartPosition;
	AbilityMontageVisualState.ServerStartTime = GetServerTimeSecondsSafe();
	AbilityMontageVisualState.bIsPlaying = true;
	++AbilityMontageVisualState.SequenceId;

	ForceNetUpdate();
}

void ABasePawn::StopAbilityMontageVisual(UAnimMontage* Montage)
{
	if (!HasAuthority())
	{
		return;
	}

	if (Montage && AbilityMontageVisualState.Montage != Montage)
	{
		return;
	}

	AbilityMontageVisualState.bIsPlaying = false;
	++AbilityMontageVisualState.SequenceId;

	ForceNetUpdate();
}

void ABasePawn::OnRep_AbilityMontageVisualState()
{
	ApplyAbilityMontageVisualState();
}

void ABasePawn::ApplyAbilityMontageVisualState()
{
	if (!MeshComponent)
	{
		return;
	}

	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	UAnimMontage* Montage = AbilityMontageVisualState.Montage;
	if (!Montage)
	{
		return;
	}

	if (!AbilityMontageVisualState.bIsPlaying)
	{
		if (AnimInstance->Montage_IsPlaying(Montage))
		{
			AnimInstance->Montage_Stop(0.15f, Montage);
		}

		return;
	}

	const float ServerNow = GetServerTimeSecondsSafe();

	const float DesiredPosition = FMath::Clamp(
		AbilityMontageVisualState.StartPosition +
		((ServerNow - AbilityMontageVisualState.ServerStartTime) * AbilityMontageVisualState.PlayRate),
		0.f,
		Montage->GetPlayLength()
	);

	if (!AnimInstance->Montage_IsPlaying(Montage))
	{
		AnimInstance->Montage_Play(Montage, AbilityMontageVisualState.PlayRate);

		if (FAnimMontageInstance* MontageInstance =
			AnimInstance->GetActiveInstanceForMontage(Montage))
		{
			MontageInstance->PushDisableRootMotion();
		}
	}

	AnimInstance->Montage_SetPosition(Montage, DesiredPosition);
}

void ABasePawn::EnsureAbilityMontageVisual()
{
	if (IsLocallyControlled())
	{
		return;
	}

	if (!AbilityMontageVisualState.bIsPlaying || !AbilityMontageVisualState.Montage)
	{
		return;
	}

	if (!AbilityAnimState.bAbilityRootMotionActive && !AbilityAnimState.bShouldBlendMontage)
	{
		return;
	}

	if (!MeshComponent)
	{
		return;
	}

	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	if (!AnimInstance->Montage_IsPlaying(AbilityMontageVisualState.Montage))
	{
		ApplyAbilityMontageVisualState();
	}
}
