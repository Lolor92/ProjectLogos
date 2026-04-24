#include "Mover/PL_PlayMoverMontageAndWait.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"

UPL_PlayMoverMontageAndWait* UPL_PlayMoverMontageAndWait::PlayMoverMontageAndWait(
	UGameplayAbility* OwningAbility,
	FName TaskInstanceName,
	UAnimMontage* MontageToPlay,
	float PlayRate,
	FName StartSection,
	float RootMotionTranslationScale,
	EPLRootMotionCollisionStopMode CollisionStopMode,
	bool bStopWhenAbilityEnds
)
{
	UPL_PlayMoverMontageAndWait* Task =
		NewAbilityTask<UPL_PlayMoverMontageAndWait>(OwningAbility, TaskInstanceName);

	Task->MontageToPlay = MontageToPlay;
	Task->PlayRate = PlayRate;
	Task->StartSection = StartSection;
	Task->RootMotionTranslationScale = RootMotionTranslationScale;
	Task->CollisionStopMode = CollisionStopMode;
	Task->bStopWhenAbilityEnds = bStopWhenAbilityEnds;

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

	USkeletalMeshComponent* MeshComponent = ASC->GetAvatarActor()
		? ASC->GetAvatarActor()->FindComponentByClass<USkeletalMeshComponent>()
		: nullptr;

	UAnimInstance* AnimInstance = MeshComponent
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

	const float MontageLength = AnimInstance->Montage_Play(MontageToPlay, PlayRate);

	if (MontageLength <= 0.f)
	{
		OnFailed.Broadcast();
		EndTask();
		return;
	}

	if (StartSection != NAME_None)
	{
		AnimInstance->Montage_JumpToSection(StartSection, MontageToPlay);
	}

	bPlayedMontage = true;

	// For now, this only proves the task can play the montage.
	// Next chunk queues the Mover root-motion layered move.
}

void UPL_PlayMoverMontageAndWait::ExternalCancel()
{
	OnCancelled.Broadcast();
	Super::ExternalCancel();
}

void UPL_PlayMoverMontageAndWait::OnDestroy(bool bInOwnerFinished)
{
	if (bStopWhenAbilityEnds)
	{
		StopPlayingMontage();
	}

	Super::OnDestroy(bInOwnerFinished);
}

void UPL_PlayMoverMontageAndWait::StopPlayingMontage()
{
	if (!bPlayedMontage || !Ability || !MontageToPlay) return;

	UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	if (!ASC || !ASC->GetAvatarActor()) return;

	USkeletalMeshComponent* MeshComponent =
		ASC->GetAvatarActor()->FindComponentByClass<USkeletalMeshComponent>();

	UAnimInstance* AnimInstance = MeshComponent
		? MeshComponent->GetAnimInstance()
		: nullptr;

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
