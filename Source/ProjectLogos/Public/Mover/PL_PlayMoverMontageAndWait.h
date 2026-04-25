#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Mover/PL_ScaledAnimRootMotionLayeredMove.h"
#include "PL_PlayMoverMontageAndWait.generated.h"

class UAnimMontage;
class UCharacterMoverComponent;
class USkeletalMeshComponent;
class ABasePawn;

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
	UPROPERTY(BlueprintAssignable)
	FPLPlayMoverMontageDelegate OnCompleted;

	// Called when the montage starts blending out normally.
	UPROPERTY(BlueprintAssignable)
	FPLPlayMoverMontageDelegate OnBlendOut;

	UPROPERTY(BlueprintAssignable)
	FPLPlayMoverMontageDelegate OnInterrupted;

	UPROPERTY(BlueprintAssignable)
	FPLPlayMoverMontageDelegate OnCancelled;

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
	bool bStopWhenAbilityEnds = true,
	bool bDisableAnimRootMotion = true
	);

	virtual void Activate() override;
	virtual void ExternalCancel() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;
	
	ABasePawn* GetAvatarBasePawn() const;
	void SetAvatarShouldBlendMontage(bool bNewValue, const TCHAR* Source) const;

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
	
	// If true, AnimInstance plays the pose but Mover handles movement.
	bool bDisableAnimRootMotion = true;
	
	bool bUseRootMotionRelease = false;
	float RootMotionReleasePercent = 100.f;
	bool bRequireMoveInputForRootMotionRelease = true;
	
	void OnMontageBlendingOut(UAnimMontage* InMontage, bool bInterrupted);
	void OnMontageEnded(UAnimMontage* InMontage, bool bInterrupted);
	
	bool PlayScaledMoverMontage();
	void QueueScaledRootMotionMove(float StartingMontagePosition);
	
	void StopPlayingMontage();

	UCharacterMoverComponent* FindCharacterMoverComponent() const;
};
