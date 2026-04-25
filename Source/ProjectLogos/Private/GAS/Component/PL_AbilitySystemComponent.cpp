#include "GAS/Component/PL_AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "GAS/Abilities/PL_GameplayAbility.h"

UPL_AbilitySystemComponent::UPL_AbilitySystemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPL_AbilitySystemComponent::SetActiveMoverMontageAbility(
	UPL_GameplayAbility* InAbility,
	UAnimMontage* InMontage)
{
	ActiveMoverMontageAbility = InAbility;
	ActiveMoverMontage = InMontage;
}

void UPL_AbilitySystemComponent::ClearActiveMoverMontageAbility(
	UPL_GameplayAbility* InAbility,
	UAnimMontage* InMontage)
{
	if (ActiveMoverMontageAbility == InAbility && ActiveMoverMontage == InMontage)
	{
		ActiveMoverMontageAbility = nullptr;
		ActiveMoverMontage = nullptr;
	}
}
