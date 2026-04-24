#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "PL_PlayerState.generated.h"

class UAbilitySystemComponent;
class UPL_AbilitySystemComponent;

/**
 * PlayerState owns the player's ASC.
 * This lets GAS survive pawn respawns later.
 */
UCLASS()
class PROJECTLOGOS_API APL_PlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	APL_PlayerState();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category="AbilitySystem")
	UPL_AbilitySystemComponent* GetProjectAbilitySystemComponent() const { return AbilitySystemComponent; }

protected:
	// Player-owned GAS component.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UPL_AbilitySystemComponent> AbilitySystemComponent;
};
