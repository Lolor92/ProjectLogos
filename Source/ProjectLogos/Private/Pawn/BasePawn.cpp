#include "Pawn/BasePawn.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "Mover/PL_MoverPawnComponent.h"

ABasePawn::ABasePawn()
{
	PrimaryActorTick.bCanEverTick = false;

	// Mover handles movement replication, so the actor itself should replicate.
	bReplicates = true;

	// Do not use old Actor movement replication with Mover.
	SetReplicateMovement(false);

	// Main collision body. Mover will move this.
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	SetRootComponent(CapsuleComponent);

	// Visible character mesh.
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CapsuleComponent);

	// Mover movement component.
	CharacterMoverComponent = CreateDefaultSubobject<UCharacterMoverComponent>(TEXT("CharacterMoverComponent"));

	// Handles input production for Mover.
	MoverPawnComponent = CreateDefaultSubobject<UPL_MoverPawnComponent>(TEXT("MoverPawnComponent"));
}
