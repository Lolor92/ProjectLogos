#include "Combat/Components/PL_CombatComponent.h"
#include "GAS/Component/PL_AbilitySystemComponent.h"
#include "GAS/Data/PL_AbilitySet.h"
#include "Pawn/BasePawn.h"

UPL_CombatComponent::UPL_CombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPL_CombatComponent::InitializeCombat(
	ABasePawn* InOwnerPawn,
	UPL_AbilitySystemComponent* InAbilitySystemComponent
)
{
	if (!InOwnerPawn || !InAbilitySystemComponent) return;

	OwnerPawn = InOwnerPawn;
	AbilitySystemComponent = InAbilitySystemComponent;

	if (OwnerPawn->HasAuthority())
	{
		GrantDefaultAbilities();
	}
}

void UPL_CombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearDefaultAbilities();

	Super::EndPlay(EndPlayReason);
}

void UPL_CombatComponent::GrantDefaultAbilities()
{
	if (bDefaultAbilitiesGranted || !AbilitySystemComponent)
	{
		return;
	}

	for (const UPL_AbilitySet* AbilitySet : DefaultAbilitySets)
	{
		if (!AbilitySet)
		{
			continue;
		}

		// SourceObject is the combat component, because it owns this combat loadout.
		AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, &GrantedHandles, this);
	}

	bDefaultAbilitiesGranted = true;
}

void UPL_CombatComponent::ClearDefaultAbilities()
{
	if (AbilitySystemComponent && GrantedHandles.HasAnyHandles())
	{
		GrantedHandles.TakeFromAbilitySystem(AbilitySystemComponent);
	}

	bDefaultAbilitiesGranted = false;
}
