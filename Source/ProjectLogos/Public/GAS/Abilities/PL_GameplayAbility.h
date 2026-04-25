#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
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

USTRUCT(BlueprintType)
struct FPLMontageActivationLockoutSettings
{
	GENERATED_BODY()

	// If true, another ability cannot interrupt this one until the montage reaches the required percent.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Montage Activation Lockout")
	bool bUseMontageActivationLockout = false;

	// Example: 45 means other abilities are blocked until this ability's montage reaches 45%.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Montage Activation Lockout",
		meta=(EditCondition="bUseMontageActivationLockout", ClampMin="0.0", ClampMax="100.0", Units="Percent"))
	float RequiredMontagePercentBeforeInterrupt = 0.f;
};

UCLASS(Abstract)
class PROJECTLOGOS_API UPL_GameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UPL_GameplayAbility();

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr
	) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

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

	UFUNCTION(BlueprintPure, Category="Ability|Activation Lockout")
	const FPLMontageActivationLockoutSettings& GetMontageActivationLockoutSettings() const
	{
		return MontageActivationLockoutSettings;
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

	// This ability protects itself while its montage is playing.
	// Other abilities cannot activate until this montage reaches the configured percent.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability|Activation Lockout")
	FPLMontageActivationLockoutSettings MontageActivationLockoutSettings;

	// This ability ignores the active montage lockout.
	// Use for dodge, death, knockdown, stagger, emergency reactions, etc.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability|Activation Lockout")
	bool bBypassMontageActivationLockout = false;

	// Old-project style behavior: after this ability activates, cancel other active abilities.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability|Interrupt")
	bool bCancelOtherAbilitiesOnActivate = false;

private:
	bool CanUseAbility(const FGameplayAbilityActorInfo* ActorInfo) const;

	bool CanInterruptActiveMontageAbility(
		const FGameplayAbilityActorInfo* ActorInfo,
		const UGameplayAbility* ActiveAbility,
		const UAnimMontage* ActiveMontage
	) const;

	void CancelOtherActiveAbilities() const;
	void ResetComboWindow();

	FTimerHandle ComboWindowTimerHandle;

	bool bComboWindowOpen = false;
};
