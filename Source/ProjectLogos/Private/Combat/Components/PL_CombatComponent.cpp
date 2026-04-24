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
	if (bDefaultAbilitiesGranted || !AbilitySystemComponent) return;

	for (const UPL_AbilitySet* AbilitySet : DefaultAbilitySets)
	{
		if (!AbilitySet) continue;

		// SourceObject is the combat component, because it owns this combat loadout.
		AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, this);
	}

	bDefaultAbilitiesGranted = true;
}

void UPL_CombatComponent::ClearDefaultAbilities()
{
	// We are not removing granted specs yet because PL_AbilitySet does not return handles.
	// Later we can add FPLAbilitySet_GrantedHandles if we need clean removal.

	bDefaultAbilitiesGranted = false;
}
