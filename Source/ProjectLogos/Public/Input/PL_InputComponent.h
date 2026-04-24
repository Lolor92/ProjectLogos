#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "PL_InputComponent.generated.h"

class APlayerController;
class UAbilitySystemComponent;
class UEnhancedInputComponent;
class UGameplayAbility;
class UPL_InputConfig;
struct FGameplayAbilitySpec;
struct FInputActionValue;

// Input-owned combo window for one starter ability.
struct FPLActiveComboChain
{
	// Starter ability that owns this input chain.
	FGameplayAbilitySpecHandle CurrentAbilityHandle;

	// Next ability to activate when the same input is pressed again.
	TSubclassOf<UGameplayAbility> NextAbilityClass = nullptr;

	// Separate timeout per starter ability.
	FTimerHandle TimerHandle;
};

/**
 * Pawn-owned input bridge.
 * Adds mapping contexts, binds tagged input actions, and forwards input to Mover/GAS.
 */
UCLASS(ClassGroup=(ProjectLogos), meta=(BlueprintSpawnableComponent))
class PROJECTLOGOS_API UPL_InputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPL_InputComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	TObjectPtr<UPL_InputConfig> InputConfig = nullptr;

private:
	void InstallInput();
	void UninstallInput();

	void AddMappingContexts() const;
	void RemoveMappingContexts() const;
	void BindInputActions();

	void Move(const FInputActionValue& Value);
	void StopMove(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	void HandleAbilityInputPressed(FGameplayTag InputTag);
	void HandleAbilityInputReleased(FGameplayTag InputTag);

	bool TryActivateComboAbility(const FGameplayAbilitySpec& RequestedAbilitySpec);
	void UpdateComboChain(FGameplayAbilitySpecHandle StarterHandle, const FGameplayAbilitySpec& CurrentAbilitySpec);
	void ClearComboChain(FGameplayAbilitySpecHandle StarterHandle);
	void ClearAllComboChains();

	bool IsLocallyControlledOwner() const;
	APlayerController* GetOwningPlayerController() const;
	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	FGameplayTag MoveInputTag;
	FGameplayTag LookInputTag;

	// Active combo chains by starter ability.
	TMap<FGameplayAbilitySpecHandle, FPLActiveComboChain> ActiveComboChains;

	UPROPERTY(Transient)
	TObjectPtr<UEnhancedInputComponent> InjectedEnhancedInputComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent = nullptr;

	TWeakObjectPtr<APlayerController> CachedPlayerController;
};
