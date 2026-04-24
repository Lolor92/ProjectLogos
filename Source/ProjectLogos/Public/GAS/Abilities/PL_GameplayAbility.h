#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "TimerManager.h"
#include "PL_GameplayAbility.generated.h"

/**
 * Project base gameplay ability.
 * Stores shared ability data like combo chaining.
 */
UCLASS(Abstract)
class PROJECTLOGOS_API UPL_GameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UPL_GameplayAbility();

	// Next ability to activate when this ability continues a combo.
	UFUNCTION(BlueprintPure, Category="Ability|Combo")
	TSubclassOf<UGameplayAbility> GetComboAbilityClass() const { return ComboAbilityClass; }

	// How long input has to continue into the next combo step.
	UFUNCTION(BlueprintPure, Category="Ability|Combo")
	float GetComboWindowDuration() const { return ComboWindowDuration; }

	// True while this ability allows combo continuation.
	UFUNCTION(BlueprintPure, Category="Ability|Combo")
	bool IsComboWindowOpen() const { return bComboWindowOpen; }

protected:
	// Ability class to activate when combo input is pressed again.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability|Combo")
	TSubclassOf<UGameplayAbility> ComboAbilityClass = nullptr;

	// How long the input component keeps the next combo step available.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability|Combo", meta=(ClampMin="0.0", Units="Seconds"))
	float ComboWindowDuration = 2.f;

	// Opens the combo window. Usually called from an anim notify / ability event.
	UFUNCTION(BlueprintCallable, Category="Ability|Combo")
	void OpenComboWindow();

	// Closes the combo window manually.
	UFUNCTION(BlueprintCallable, Category="Ability|Combo")
	void CloseComboWindow();

private:
	void ResetComboWindow();

	FTimerHandle ComboWindowTimerHandle;

	bool bComboWindowOpen = false;
};
