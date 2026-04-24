#include "Pawn/PL_PlayerPawn.h"

#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputMappingContext.h"
#include "Mover/PL_MoverPawnComponent.h"

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
}

void APL_PlayerPawn::BeginPlay()
{
	Super::BeginPlay();

	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController) return;

	const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer) return;

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

	if (!InputSubsystem || !DefaultMappingContext) return;

	// Add the pawn's default input mapping for the local player.
	InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
}

void APL_PlayerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent =
		Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (!EnhancedInputComponent) return;

	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(
			MoveAction,
			ETriggerEvent::Triggered,
			this,
			&APL_PlayerPawn::Move
		);

		EnhancedInputComponent->BindAction(
			MoveAction,
			ETriggerEvent::Completed,
			this,
			&APL_PlayerPawn::Move
		);
	}

	if (LookAction)
	{
		EnhancedInputComponent->BindAction(
			LookAction,
			ETriggerEvent::Triggered,
			this,
			&APL_PlayerPawn::Look
		);
	}
}

void APL_PlayerPawn::Move(const FInputActionValue& Value)
{
	const FVector2D MoveInput = Value.Get<FVector2D>();

	if (!MoverPawnComponent) return;

	if (MoveInput.IsNearlyZero())
	{
		// Stop movement when the input action completes.
		MoverPawnComponent->ClearMoveIntent();
		return;
	}

	// X = forward/back, Y = right/left.
	const FVector MoveIntent(MoveInput.X, MoveInput.Y, 0.f);

	MoverPawnComponent->RequestMoveIntent(MoveIntent);
}

void APL_PlayerPawn::Look(const FInputActionValue& Value)
{
	const FVector2D LookInput = Value.Get<FVector2D>();

	if (LookInput.IsNearlyZero()) return;

	// Yaw turns the camera left/right.
	AddControllerYawInput(LookInput.X);

	// Pitch tilts the camera up/down.
	AddControllerPitchInput(LookInput.Y);
}
