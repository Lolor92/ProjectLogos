#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Mover/PL_ScaledAnimRootMotionLayeredMove.h"
#include "PL_PlayMoverMontageAndWait.generated.h"

class UAnimMontage;
class UCharacterMoverComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPLPlayMoverMontageDelegate);

/**
 * GAS ability task that plays a montage visually,
 * while Mover applies the montage root motion.
 */
UCLASS()
class PROJECTLOGOS_API UPL_PlayMoverMontageAndWait : public UAbilityTask
{
	GENERATED_BODY()

public:
	// Called when the montage finishes normally.
	UPROPERTY(BlueprintAssignable)
	FPLPlayMoverMontageDelegate OnCompleted;
	
	// Called when the montage starts blending out normally.
	UPROPERTY(BlueprintAssignable)
	FPLPlayMoverMontageDelegate OnBlendOut;

	// Called when the montage is interrupted.
	UPROPERTY(BlueprintAssignable)
	FPLPlayMoverMontageDelegate OnInterrupted;

	// Called when the montage is cancelled by the ability.
	UPROPERTY(BlueprintAssignable)
	FPLPlayMoverMontageDelegate OnCancelled;

	// Called if the task could not start.
	UPROPERTY(BlueprintAssignable)
	FPLPlayMoverMontageDelegate OnFailed;

	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta=(
		DisplayName="Play Mover Montage And Wait",
		HidePin="OwningAbility",
		DefaultToSelf="OwningAbility",
		BlueprintInternalUseOnly="true"
	))
	static UPL_PlayMoverMontageAndWait* PlayMoverMontageAndWait(
		UGameplayAbility* OwningAbility,
		FName TaskInstanceName,
		UAnimMontage* MontageToPlay,
		float PlayRate = 1.f,
		FName StartSection = NAME_None,
		float RootMotionTranslationScale = 1.f,
		EPLRootMotionCollisionStopMode CollisionStopMode = EPLRootMotionCollisionStopMode::None,
		bool bStopWhenAbilityEnds = true
	);

	virtual void Activate() override;
	virtual void ExternalCancel() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

protected:
	UPROPERTY()
	TObjectPtr<UAnimMontage> MontageToPlay = nullptr;

	UPROPERTY()
	TObjectPtr<UCharacterMoverComponent> CharacterMoverComponent = nullptr;

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> MeshComponent = nullptr;

	UPROPERTY()
	TObjectPtr<UAnimInstance> AnimInstance = nullptr;

	float PlayRate = 1.f;
	float RootMotionTranslationScale = 1.f;

	FName StartSection = NAME_None;

	EPLRootMotionCollisionStopMode CollisionStopMode = EPLRootMotionCollisionStopMode::None;

	bool bStopWhenAbilityEnds = true;
	bool bPlayedMontage = false;

	void OnMontageBlendingOut(UAnimMontage* InMontage, bool bInterrupted);
	void OnMontageEnded(UAnimMontage* InMontage, bool bInterrupted);

	void StopPlayingMontage();

	UCharacterMoverComponent* FindCharacterMoverComponent() const;
};
