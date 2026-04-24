#include "GAS/Data/PL_AbilitySet.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"

void UPL_AbilitySet::GiveToAbilitySystem(
	UAbilitySystemComponent* AbilitySystemComponent,
	UObject* SourceObject
) const
{
	if (!AbilitySystemComponent) return;

	for (const FPLAbilitySet_GameplayAbility& AbilityToGrant : GrantedGameplayAbilities)
	{
		if (!AbilityToGrant.Ability) continue;

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

		AbilitySystemComponent->GiveAbility(AbilitySpec);
	}
}
