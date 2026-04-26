#include "GAS/Abilities/PL_GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GAS/Component/PL_AbilitySystemComponent.h"
#include "Mover/PL_MoverPawnComponent.h"
#include "Pawn/BasePawn.h"

UPL_GameplayAbility::UPL_GameplayAbility()
{
	// We need ability instances because combo state and lockout state live on the ability.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Useful if you want input reactivation / combo chaining on the same ability instance.
	bRetriggerInstancedAbility = true;
}

bool UPL_GameplayAbility::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	return Super::CanActivateAbility(
		Handle,
		ActorInfo,
		SourceTags,
		TargetTags,
		OptionalRelevantTags
	) && CanUseAbility(ActorInfo);
}

void UPL_GameplayAbility::CommitExecute(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::CommitExecute(Handle, ActorInfo, ActivationInfo);

	CancelOtherActiveAbilities();
}

void UPL_GameplayAbility::CancelAvatarHitStop() const
{
	if (const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo())
	{
		if (ABasePawn* BasePawn = Cast<ABasePawn>(Info->AvatarActor.Get()))
		{
			if (UPL_MoverPawnComponent* MoverPawnComponent = BasePawn->GetMoverPawnComponent())
			{
				MoverPawnComponent->CancelHitStop();
			}
		}
	}
}

bool UPL_GameplayAbility::CanUseAbility(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!ActorInfo)
	{
		return true;
	}

	// This is the bypass.
	// Use it for abilities that must always be allowed to activate.
	if (bBypassMontageActivationLockout)
	{
		return true;
	}

	const UAbilitySystemComponent* AbilitySystemComponent = ActorInfo->AbilitySystemComponent.Get();

	if (!AbilitySystemComponent)
	{
		return true;
	}

	const UPL_AbilitySystemComponent* PLASC = Cast<UPL_AbilitySystemComponent>(AbilitySystemComponent);

	const UGameplayAbility* ActiveAbility = nullptr;
	const UAnimMontage* ActiveMontage = nullptr;

	if (PLASC)
	{
		ActiveAbility = PLASC->GetActiveMoverMontageAbility();
		ActiveMontage = PLASC->GetActiveMoverMontage();
	}

	// Fallback for normal GAS montage tasks.
	if (!ActiveAbility)
	{
		ActiveAbility = AbilitySystemComponent->GetAnimatingAbility();
	}

	if (!ActiveMontage && ActiveAbility)
	{
		ActiveMontage = ActiveAbility->GetCurrentMontage();
	}

	if (!ActiveAbility || !ActiveMontage)
	{
		return true;
	}

	return CanInterruptActiveMontageAbility(ActorInfo, ActiveAbility, ActiveMontage);
}

bool UPL_GameplayAbility::CanInterruptActiveMontageAbility(
	const FGameplayAbilityActorInfo* ActorInfo,
	const UGameplayAbility* ActiveAbility,
	const UAnimMontage* ActiveMontage) const
{
	if (!ActorInfo || !ActiveAbility || !ActiveMontage)
	{
		return true;
	}

	const UPL_GameplayAbility* ActivePLAbility = Cast<UPL_GameplayAbility>(ActiveAbility);

	// Non-PL abilities do not use our lockout rule.
	if (!ActivePLAbility)
	{
		return true;
	}

	const FPLMontageActivationLockoutSettings& ActiveLockout =
		ActivePLAbility->GetMontageActivationLockoutSettings();

	if (!ActiveLockout.bUseMontageActivationLockout)
	{
		return true;
	}

	USkeletalMeshComponent* MeshComp = ActorInfo->SkeletalMeshComponent.Get();

	if (!MeshComp && ActorInfo->AvatarActor.IsValid())
	{
		MeshComp = ActorInfo->AvatarActor->FindComponentByClass<USkeletalMeshComponent>();
	}

	UAnimInstance* AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;

	if (!AnimInstance)
	{
		// If we cannot read montage progress, fail closed.
		// Better to block than allow a random early cancel.
		return false;
	}

	if (!AnimInstance->Montage_IsPlaying(ActiveMontage))
	{
		return true;
	}

	const float MontageLength = ActiveMontage->GetPlayLength();

	if (MontageLength <= 0.f)
	{
		return false;
	}

	const float MontagePosition = AnimInstance->Montage_GetPosition(ActiveMontage);
	const float MontagePercent = (MontagePosition / MontageLength) * 100.f;

	return MontagePercent >= ActiveLockout.RequiredMontagePercentBeforeInterrupt;
}

void UPL_GameplayAbility::CancelOtherActiveAbilities() const
{
	if (!bCancelOtherAbilitiesOnActivate)
	{
		return;
	}

	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();

	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->CancelAllAbilities(const_cast<ThisClass*>(this));
}

void UPL_GameplayAbility::OpenComboWindow()
{
	bComboWindowOpen = true;

	UWorld* World = GetWorld();
	if (!World) return;

	World->GetTimerManager().ClearTimer(ComboWindowTimerHandle);

	if (ComboWindowDuration > 0.f)
	{
		// Auto-close the combo window after the configured duration.
		World->GetTimerManager().SetTimer(
			ComboWindowTimerHandle,
			this,
			&ThisClass::ResetComboWindow,
			ComboWindowDuration,
			false
		);
	}
}

void UPL_GameplayAbility::CloseComboWindow()
{
	ResetComboWindow();
}

void UPL_GameplayAbility::ResetComboWindow()
{
	bComboWindowOpen = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ComboWindowTimerHandle);
	}
}
