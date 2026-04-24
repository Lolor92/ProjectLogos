#include "Pawn/BasePawn.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "GAS/Component/PL_AbilitySystemComponent.h"
#include "Mover/PL_MoverPawnComponent.h"
#include "GAS/Data/PL_AbilitySet.h"

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
