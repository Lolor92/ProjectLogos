#include "Pawn/BasePawn.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
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

void ABasePawn::MulticastPlayAbilityMontageVisual_Implementation(
	UAnimMontage* Montage,
	float InPlayRate,
	FName InStartSection,
	float InStartPosition,
	bool bDisableRootMotion
)
{
	if (!Montage || !MeshComponent)
	{
		return;
	}

	if (HasAuthority() || IsLocallyControlled())
	{
		return;
	}

	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	const float MontageLength = AnimInstance->Montage_Play(Montage, InPlayRate);
	if (MontageLength <= 0.f)
	{
		return;
	}

	if (InStartSection != NAME_None)
	{
		AnimInstance->Montage_JumpToSection(InStartSection, Montage);
	}

	AnimInstance->Montage_SetPosition(Montage, InStartPosition);

	if (bDisableRootMotion)
	{
		if (FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(Montage))
		{
			MontageInstance->PushDisableRootMotion();
		}
	}
}

void ABasePawn::MulticastStopAbilityMontageVisual_Implementation(
	UAnimMontage* Montage,
	float BlendOutTime
)
{
	if (!Montage || !MeshComponent)
	{
		return;
	}

	if (HasAuthority() || IsLocallyControlled())
	{
		return;
	}

	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	if (AnimInstance->Montage_IsPlaying(Montage))
	{
		AnimInstance->Montage_Stop(BlendOutTime, Montage);
	}
}
