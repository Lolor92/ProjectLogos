#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "PL_InputComponent.generated.h"

class APlayerController;
class UCameraComponent;
class UAbilitySystemComponent;
class UEnhancedInputComponent;
class UGameplayAbility;
class UPL_InputConfig;
class USpringArmComponent;
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

	void InitializePlayerInput();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	TObjectPtr<UPL_InputConfig> InputConfig = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Zoom")
	bool bEnableZoom = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Zoom", meta=(ClampMin="0"))
	int32 MinZoomLevel = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Zoom", meta=(ClampMin="0"))
	int32 MaxZoomLevel = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Zoom", meta=(ClampMin="0"))
	int32 ZoomLevel = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Zoom", meta=(ClampMin="0.0"))
	float ZoomStepDistance = 200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Zoom", meta=(ClampMin="0.0"))
	float ZoomInterpSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Zoom")
	FVector DefaultCameraOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Zoom")
	FVector ClosestZoomCameraOffset = FVector(50.f, 0.f, 70.f);

private:
	void InstallInput();
	void UninstallInput();

	void AddMappingContexts() const;
	void RemoveMappingContexts() const;
	void BindInputActions();

	void Move(const FInputActionValue& Value);
	void StopMove(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Zoom(const FInputActionValue& Value);
	void ApplyZoom();

	void HandleAbilityInputPressed(FGameplayTag InputTag);
	void HandleAbilityInputReleased(FGameplayTag InputTag);

	bool TryActivateComboAbility(const FGameplayAbilitySpec& RequestedAbilitySpec);
	void SnapOwnerFacingToControllerYawForAbility(const FGameplayAbilitySpec& AbilitySpec) const;
	void UpdateComboChain(FGameplayAbilitySpecHandle StarterHandle, const FGameplayAbilitySpec& CurrentAbilitySpec);
	void ClearComboChain(FGameplayAbilitySpecHandle StarterHandle);
	void ClearAllComboChains();

	bool IsLocallyControlledOwner() const;
	APlayerController* GetOwningPlayerController() const;
	UAbilitySystemComponent* GetAbilitySystemComponent() const;
	USpringArmComponent* FindSpringArm() const;
	UCameraComponent* FindCamera() const;

	FGameplayTag MoveInputTag;
	FGameplayTag LookInputTag;
	FGameplayTag ZoomInputTag;

	// Active combo chains by starter ability.
	TMap<FGameplayAbilitySpecHandle, FPLActiveComboChain> ActiveComboChains;

	UPROPERTY(Transient)
	TObjectPtr<UEnhancedInputComponent> InjectedEnhancedInputComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> CachedSpringArm = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> CachedCamera = nullptr;

	UPROPERTY(Transient)
	float DesiredZoomArmLength = 0.f;

	TWeakObjectPtr<APlayerController> CachedPlayerController;
};
