#include "Mover/PL_PlayMoverMontageAndWait.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GAS/Abilities/PL_GameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "GameFramework/Actor.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Mover/PL_ScaledAnimRootMotionLayeredMove.h"
#include "Pawn/BasePawn.h"


UPL_PlayMoverMontageAndWait* UPL_PlayMoverMontageAndWait::PlayMoverMontageAndWait(
	UGameplayAbility* OwningAbility,
	FName TaskInstanceName,
	UAnimMontage* MontageToPlay,
	float PlayRate,
	FName StartSection,
	float RootMotionTranslationScale,
	EPLRootMotionCollisionStopMode CollisionStopMode,
	bool bStopWhenAbilityEnds,
	bool bDisableAnimRootMotion
)
{
	UPL_PlayMoverMontageAndWait* Task =
		NewAbilityTask<UPL_PlayMoverMontageAndWait>(OwningAbility, TaskInstanceName);

	Task->MontageToPlay = MontageToPlay;
	Task->PlayRate = PlayRate;
	Task->StartSection = StartSection;
	Task->RootMotionTranslationScale = RootMotionTranslationScale;
	Task->CollisionStopMode = CollisionStopMode;
	if (const UPL_GameplayAbility* PLAbility = Cast<UPL_GameplayAbility>(OwningAbility))
	{
		const FPLRootMotionReleaseSettings& ReleaseSettings =
			PLAbility->GetRootMotionReleaseSettings();

		Task->bUseRootMotionRelease = ReleaseSettings.bUseRootMotionRelease;
		Task->RootMotionReleasePercent = ReleaseSettings.RootMotionReleasePercent;
		Task->bRequireMoveInputForRootMotionRelease =
			ReleaseSettings.bRequireMovementInputForRelease;
	}
	Task->bStopWhenAbilityEnds = bStopWhenAbilityEnds;
	Task->bDisableAnimRootMotion = bDisableAnimRootMotion;

	return Task;
}

void UPL_PlayMoverMontageAndWait::Activate()
{
	if (!Ability || !MontageToPlay)
	{
		OnFailed.Broadcast();
		EndTask();
		return;
	}

	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!ASC)
	{
		OnFailed.Broadcast();
		EndTask();
		return;
	}

	MeshComponent = ASC->GetAvatarActor()
	? ASC->GetAvatarActor()->FindComponentByClass<USkeletalMeshComponent>()
	: nullptr;

	AnimInstance = MeshComponent
		? MeshComponent->GetAnimInstance()
		: nullptr;

	if (!AnimInstance)
	{
		OnFailed.Broadcast();
		EndTask();
		return;
	}

	CharacterMoverComponent = FindCharacterMoverComponent();

	if (!CharacterMoverComponent)
	{
		OnFailed.Broadcast();
		EndTask();
		return;
	}

	if (!PlayScaledMoverMontage())
	{
		OnFailed.Broadcast();
		EndTask();
		return;
	}
}

bool UPL_PlayMoverMontageAndWait::PlayScaledMoverMontage()
{
	if (!AnimInstance || !MontageToPlay) return false;
	
	if (ABasePawn* BasePawn = GetAvatarBasePawn())
	{
		BasePawn->SetAbilityAnimStateValues(true, false);
	}
	
	const float MontageLength = AnimInstance->Montage_Play(MontageToPlay, PlayRate);

	if (MontageLength <= 0.f) return false;

	if (StartSection != NAME_None)
	{
		AnimInstance->Montage_JumpToSection(StartSection, MontageToPlay);
	}
	
	FAnimMontageInstance* MontageInstance =
	AnimInstance->GetActiveInstanceForMontage(MontageToPlay);

	if (!MontageInstance) return false;

	if (ABasePawn* BasePawn = GetAvatarBasePawn())
	{
		if (BasePawn->HasAuthority())
		{
			BasePawn->StartAbilityMontageVisual(
				MontageToPlay,
				PlayRate,
				MontageInstance->GetPosition()
			);
		}
	}

	if (bDisableAnimRootMotion && PlayRate != 0.f && MontageToPlay->HasRootMotion())
	{
		// Disable normal montage root motion. Mover will apply it instead.
		MontageInstance->PushDisableRootMotion();

		// Queue Mover root motion starting from the current montage position.
		QueueScaledRootMotionMove(MontageInstance->GetPosition());
	}

	// Listen for montage blend-out and end so the task can finish cleanly.
	FOnMontageBlendingOutStarted BlendOutDelegate;
	BlendOutDelegate.BindUObject(this, &UPL_PlayMoverMontageAndWait::OnMontageBlendingOut);
	AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, MontageToPlay);

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UPL_PlayMoverMontageAndWait::OnMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, MontageToPlay);

	bPlayedMontage = true;

	return true;
}

void UPL_PlayMoverMontageAndWait::QueueScaledRootMotionMove(float StartingMontagePosition)
{
	if (!CharacterMoverComponent || !MontageToPlay) return;
	
	TSharedPtr<FPL_ScaledAnimRootMotionLayeredMove> RootMotionMove =
			MakeShared<FPL_ScaledAnimRootMotionLayeredMove>();

	RootMotionMove->MontageState.Montage = MontageToPlay;
	RootMotionMove->MontageState.PlayRate = PlayRate;
	RootMotionMove->MontageState.StartingMontagePosition = StartingMontagePosition;
	RootMotionMove->MontageState.CurrentPosition = StartingMontagePosition;

	RootMotionMove->RootMotionTranslationScale = RootMotionTranslationScale;
	RootMotionMove->RootMotionCollisionStopMode = CollisionStopMode;

	// Root-motion release settings.
	// Release point does NOT stop root motion by itself.
	// It only allows the layered move to stop once movement input exists.
	const float SafePlayRate = FMath::Max(FMath::Abs(PlayRate), UE_KINDA_SMALL_NUMBER);
	const float MontageLength = MontageToPlay->GetPlayLength();

	const float ClampedReleasePercent = FMath::Clamp(RootMotionReleasePercent, 0.f, 100.f);
	const float ReleaseAlpha = ClampedReleasePercent / 100.f;
	const float RootMotionReleasePosition = MontageLength * ReleaseAlpha;

	RootMotionMove->bUseRootMotionRelease = bUseRootMotionRelease;
	RootMotionMove->RootMotionReleasePosition = RootMotionReleasePosition;
	RootMotionMove->bRequireMoveInputForRootMotionRelease = bRequireMoveInputForRootMotionRelease;

	// Important:
	// Keep the layered move alive for the FULL montage duration.
	// If we end it at the release point, root motion would stop even when the player is not moving.
	const float DurationSeconds = MontageLength / SafePlayRate;
	RootMotionMove->DurationMs = DurationSeconds * 1000.f;

	// Root motion should override normal movement while active.
	RootMotionMove->MixMode = EMoveMixMode::OverrideAll;

	CharacterMoverComponent->QueueLayeredMove(RootMotionMove);
}

void UPL_PlayMoverMontageAndWait::OnMontageBlendingOut(UAnimMontage* InMontage, bool bInterrupted)
{
	if (InMontage != MontageToPlay) return;

	if (!ShouldBroadcastAbilityTaskDelegates()) return;

	if (bInterrupted)
	{
		OnInterrupted.Broadcast();
		return;
	}

	OnBlendOut.Broadcast();
}

void UPL_PlayMoverMontageAndWait::OnMontageEnded(UAnimMontage* InMontage, bool bInterrupted)
{
	ABasePawn* BasePawn = GetAvatarBasePawn();

	if (InMontage != MontageToPlay)
	{
		return;
	}

	if (BasePawn)
	{
		if (BasePawn->HasAuthority())
		{
			BasePawn->StopAbilityMontageVisual(MontageToPlay);
		}

		BasePawn->ResetAbilityAnimState();
	}

	if (ShouldBroadcastAbilityTaskDelegates() && !bInterrupted)
	{
		OnCompleted.Broadcast();
	}

	EndTask();
}

void UPL_PlayMoverMontageAndWait::ExternalCancel()
{
	if (ABasePawn* BasePawn = GetAvatarBasePawn())
	{
		if (BasePawn->HasAuthority())
		{
			BasePawn->StopAbilityMontageVisual(MontageToPlay);
		}

		BasePawn->ResetAbilityAnimState();
	}

	OnCancelled.Broadcast();
	Super::ExternalCancel();
}

void UPL_PlayMoverMontageAndWait::OnDestroy(bool bInOwnerFinished)
{
	if (bStopWhenAbilityEnds)
	{
		StopPlayingMontage();
	}

	const bool bMontageStillPlaying =
		AnimInstance &&
		MontageToPlay &&
		AnimInstance->Montage_IsPlaying(MontageToPlay);

	if (!bMontageStillPlaying)
	{
		if (ABasePawn* BasePawn = GetAvatarBasePawn())
		{
			BasePawn->ResetAbilityAnimState();
		}
	}

	Super::OnDestroy(bInOwnerFinished);
}

void UPL_PlayMoverMontageAndWait::StopPlayingMontage()
{
	if (!bPlayedMontage || !MontageToPlay) return;

	if (!AnimInstance) return;

	if (AnimInstance->Montage_IsPlaying(MontageToPlay))
	{
		// Small blend-out for now.
		AnimInstance->Montage_Stop(0.2f, MontageToPlay);
	}
}

UCharacterMoverComponent* UPL_PlayMoverMontageAndWait::FindCharacterMoverComponent() const
{
	if (!Ability) return nullptr;

	const AActor* AvatarActor = Ability->GetAvatarActorFromActorInfo();
	if (!AvatarActor) return nullptr;

	return AvatarActor->FindComponentByClass<UCharacterMoverComponent>();
}

ABasePawn* UPL_PlayMoverMontageAndWait::GetAvatarBasePawn() const
{
	if (!Ability)
	{
		return nullptr;
	}

	return Cast<ABasePawn>(Ability->GetAvatarActorFromActorInfo());
}
