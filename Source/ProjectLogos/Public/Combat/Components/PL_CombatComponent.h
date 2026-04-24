#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GAS/Data/PL_AbilitySet.h"
#include "PL_CombatComponent.generated.h"

class ABasePawn;
class UPL_AbilitySystemComponent;

/**
 * Combat component owns combat startup setup.
 * It receives the pawn's ASC and grants default combat abilities on the server.
 */
UCLASS(ClassGroup=(ProjectLogos), meta=(BlueprintSpawnableComponent))
class PROJECTLOGOS_API UPL_CombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPL_CombatComponent();

	void InitializeCombat(ABasePawn* InOwnerPawn, UPL_AbilitySystemComponent* InAbilitySystemComponent);

	UFUNCTION(BlueprintPure, Category="Combat")
	UPL_AbilitySystemComponent* GetProjectAbilitySystemComponent() const { return AbilitySystemComponent; }

	UFUNCTION(BlueprintPure, Category="Combat")
	ABasePawn* GetOwnerPawn() const { return OwnerPawn; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Combat abilities granted when combat is initialized on the server.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Abilities")
	TArray<TObjectPtr<UPL_AbilitySet>> DefaultAbilitySets;

private:
	void GrantDefaultAbilities();
	void ClearDefaultAbilities();

	UPROPERTY(Transient)
	TObjectPtr<ABasePawn> OwnerPawn = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UPL_AbilitySystemComponent> AbilitySystemComponent = nullptr;

	UPROPERTY(Transient)
	FPLAbilitySet_GrantedHandles GrantedHandles;

	bool bDefaultAbilitiesGranted = false;
};
