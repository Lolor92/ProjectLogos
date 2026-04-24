#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "PL_InputComponent.generated.h"

class APlayerController;
class UAbilitySystemComponent;
class UEnhancedInputComponent;
class UPL_InputConfig;
struct FInputActionValue;

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

	bool IsLocallyControlledOwner() const;
	APlayerController* GetOwningPlayerController() const;
	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	FGameplayTag MoveInputTag;
	FGameplayTag LookInputTag;

	UPROPERTY(Transient)
	TObjectPtr<UEnhancedInputComponent> InjectedEnhancedInputComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent = nullptr;

	TWeakObjectPtr<APlayerController> CachedPlayerController;
};
