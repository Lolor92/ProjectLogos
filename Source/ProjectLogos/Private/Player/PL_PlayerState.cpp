#include "Player/PL_PlayerState.h"

#include "GAS/Component/PL_AbilitySystemComponent.h"
#include "GAS/Data/PL_AbilitySet.h"

APL_PlayerState::APL_PlayerState()
{
	// PlayerState should replicate because it owns player GAS.
	bReplicates = true;

	// PlayerState updates are low by default, so raise this for GAS.
	SetNetUpdateFrequency(100.f);

	AbilitySystemComponent = CreateDefaultSubobject<UPL_AbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	// Mixed is the normal mode for player-owned ASCs.
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

UAbilitySystemComponent* APL_PlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void APL_PlayerState::GiveStartupAbilities(UObject* SourceObject)
{
	if (bStartupAbilitiesGiven || !HasAuthority() || !AbilitySystemComponent) return;

	for (const UPL_AbilitySet* AbilitySet : AbilitySets)
	{
		if (!AbilitySet) continue;

		// Server grants abilities. Clients receive them through ASC replication.
		AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, SourceObject);
	}

	bStartupAbilitiesGiven = true;
}
