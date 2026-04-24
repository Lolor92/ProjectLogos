#include "Player/PL_PlayerState.h"

#include "GAS/Component/PL_AbilitySystemComponent.h"

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
