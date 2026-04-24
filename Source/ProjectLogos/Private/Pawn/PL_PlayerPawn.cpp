#include "Pawn/PL_PlayerPawn.h"

#include "Camera/CameraComponent.h"
#include "Combat/Components/PL_CombatComponent.h"
#include "GAS/Component/PL_AbilitySystemComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Input/PL_InputComponent.h"
#include "Player/PL_PlayerState.h"

APL_PlayerPawn::APL_PlayerPawn()
{
	// Lets the controller rotate the camera, not the pawn body directly.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Camera boom keeps the camera at a clean distance.
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 500.f;
	CameraBoom->bUsePawnControlRotation = true;

	// Main player camera.
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom);
	FollowCamera->bUsePawnControlRotation = false;

	// Input lives in a reusable component, not directly in the pawn.
	PlayerInputComponent = CreateDefaultSubobject<UPL_InputComponent>(TEXT("PlayerInputComponent"));
}

void APL_PlayerPawn::BeginPlay()
{
	Super::BeginPlay();
}

void APL_PlayerPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Server initializes the PlayerState-owned ASC.
	InitializePlayerAbilitySystem();

	if (PlayerInputComponent)
	{
		PlayerInputComponent->InitializePlayerInput();
	}
}

void APL_PlayerPawn::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Client initializes the PlayerState-owned ASC when PlayerState arrives.
	InitializePlayerAbilitySystem();
}

void APL_PlayerPawn::PawnClientRestart()
{
	Super::PawnClientRestart();

	if (PlayerInputComponent)
	{
		PlayerInputComponent->InitializePlayerInput();
	}
}

void APL_PlayerPawn::InitializePlayerAbilitySystem()
{
	APL_PlayerState* PLPlayerState = GetPlayerState<APL_PlayerState>();
	if (!PLPlayerState) return;

	UPL_AbilitySystemComponent* PlayerASC = PLPlayerState->GetProjectAbilitySystemComponent();
	if (!PlayerASC) return;

	InitializeAbilitySystem(PlayerASC, PLPlayerState);

	if (CombatComponent)
	{
		CombatComponent->InitializeCombat(this, PlayerASC);
	}
}
