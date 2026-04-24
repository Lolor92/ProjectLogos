#include "Player/PL_PlayerController.h"
#include "EnhancedInputSubsystems.h"

APL_PlayerController::APL_PlayerController()
{
}

void APL_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer) return;

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

	if (!InputSubsystem || !DefaultMappingContext) return;

	// PlayerController owns local input mapping setup.
	InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
}
