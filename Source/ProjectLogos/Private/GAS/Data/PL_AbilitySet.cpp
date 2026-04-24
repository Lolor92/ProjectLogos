#include "GAS/Data/PL_AbilitySet.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"

void FPLAbilitySet_GrantedHandles::AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	if (Handle.IsValid())
	{
		AbilitySpecHandles.Add(Handle);
	}
}

void FPLAbilitySet_GrantedHandles::TakeFromAbilitySystem(UAbilitySystemComponent* AbilitySystemComponent)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
	{
		if (Handle.IsValid())
		{
			AbilitySystemComponent->ClearAbility(Handle);
		}
	}

	AbilitySpecHandles.Reset();
}

void UPL_AbilitySet::GiveToAbilitySystem(
	UAbilitySystemComponent* AbilitySystemComponent,
	FPLAbilitySet_GrantedHandles* OutGrantedHandles,
	UObject* SourceObject
) const
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	for (const FPLAbilitySet_GameplayAbility& AbilityToGrant : GrantedGameplayAbilities)
	{
		if (!AbilityToGrant.Ability)
		{
			continue;
		}

		FGameplayAbilitySpec AbilitySpec(
			AbilityToGrant.Ability,
			AbilityToGrant.AbilityLevel,
			INDEX_NONE,
			SourceObject
		);

		if (AbilityToGrant.InputTag.IsValid())
		{
			// This is what PL_InputComponent checks when matching input.
			AbilitySpec.GetDynamicSpecSourceTags().AddTag(AbilityToGrant.InputTag);
		}

		const FGameplayAbilitySpecHandle AbilitySpecHandle =
			AbilitySystemComponent->GiveAbility(AbilitySpec);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAbilitySpecHandle(AbilitySpecHandle);
		}
	}
}
