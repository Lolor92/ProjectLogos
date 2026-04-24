#include "Pawn/PL_PlayerPawn.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Input/PL_InputComponent.h"

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
