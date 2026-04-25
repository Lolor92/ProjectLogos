#include "Pawn/BasePawn.h"
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

	DOREPLIFETIME(ABasePawn, bShouldBlendMontage);
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

void ABasePawn::SetShouldBlendMontage(bool bNewShouldBlendMontage)
{
	if (bShouldBlendMontage == bNewShouldBlendMontage)
	{
		return;
	}

	bShouldBlendMontage = bNewShouldBlendMontage;

	// Immediate local update.
	ApplyShouldBlendMontage();

	if (HasAuthority())
	{
		ForceNetUpdate();
		return;
	}

	// Only the owning client should ask the server to replicate this.
	// Simulated proxies should wait for OnRep.
	if (IsLocallyControlled())
	{
		ServerSetShouldBlendMontage(bNewShouldBlendMontage);
	}
}

void ABasePawn::ServerSetShouldBlendMontage_Implementation(bool bNewShouldBlendMontage)
{
	if (bShouldBlendMontage == bNewShouldBlendMontage)
	{
		return;
	}

	bShouldBlendMontage = bNewShouldBlendMontage;

	ApplyShouldBlendMontage();
	ForceNetUpdate();
}

void ABasePawn::OnRep_ShouldBlendMontage()
{
	ApplyShouldBlendMontage();
}

void ABasePawn::ApplyShouldBlendMontage()
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

	PLAnimInstance->SetShouldBlendMontage(bShouldBlendMontage);
}
