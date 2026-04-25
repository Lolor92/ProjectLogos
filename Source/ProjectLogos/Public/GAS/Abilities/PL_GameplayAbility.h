#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "TimerManager.h"
#include "PL_GameplayAbility.generated.h"

/**
 * Project base gameplay ability.
 * Stores shared ability data like combo chaining.
 */

USTRUCT(BlueprintType)
struct FPLRootMotionReleaseSettings
{
	GENERATED_BODY()

	// If false, the montage keeps root motion for the whole montage unless collision stops it.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Root Motion Release")
	bool bUseRootMotionRelease = false;

	// Percent of the montage where root motion is allowed to release.
	// Example: 45 means the player can break into movement after 45% of the montage has played.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Root Motion Release",
		meta=(EditCondition="bUseRootMotionRelease", ClampMin="0.0", ClampMax="100.0", Units="Percent"))
	float RootMotionReleasePercent = 100.f;

	// If true, reaching the release point does not stop root motion by itself.
	// Root motion only stops when the player is actually trying to move.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Root Motion Release",
		meta=(EditCondition="bUseRootMotionRelease"))
	bool bRequireMovementInputForRelease = true;
};

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
	
	UFUNCTION(BlueprintPure, Category="Ability|Root Motion")
	const FPLRootMotionReleaseSettings& GetRootMotionReleaseSettings() const
	{
		return RootMotionReleaseSettings;
	}

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
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability|Root Motion")
	FPLRootMotionReleaseSettings RootMotionReleaseSettings;

private:
	void ResetComboWindow();

	FTimerHandle ComboWindowTimerHandle;

	bool bComboWindowOpen = false;
};
