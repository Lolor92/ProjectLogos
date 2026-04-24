#include "GAS/Abilities/PL_GameplayAbility.h"
#include "Engine/World.h"

UPL_GameplayAbility::UPL_GameplayAbility()
{
	// We need ability instances because combo state lives on the ability.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UPL_GameplayAbility::OpenComboWindow()
{
	bComboWindowOpen = true;

	UWorld* World = GetWorld();
	if (!World) return;

	World->GetTimerManager().ClearTimer(ComboWindowTimerHandle);

	if (ComboWindowDuration > 0.f)
	{
		// Auto-close the combo window after the configured duration.
		World->GetTimerManager().SetTimer(
			ComboWindowTimerHandle,
			this,
			&ThisClass::ResetComboWindow,
			ComboWindowDuration,
			false
		);
	}
}

void UPL_GameplayAbility::CloseComboWindow()
{
	ResetComboWindow();
}

void UPL_GameplayAbility::ResetComboWindow()
{
	bComboWindowOpen = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ComboWindowTimerHandle);
	}
}