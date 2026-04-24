#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "PL_AbilitySet.generated.h"

class UGameplayAbility;
class UAbilitySystemComponent;

USTRUCT(BlueprintType)
struct FPLAbilitySet_GameplayAbility
{
	GENERATED_BODY()

	// Ability class to grant.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability")
	TSubclassOf<UGameplayAbility> Ability = nullptr;

	// Ability level.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability")
	int32 AbilityLevel = 1;

	// Optional input tag added to the granted spec.
	// Example: Input.Ability.MouseLeft
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability")
	FGameplayTag InputTag;
};

USTRUCT(BlueprintType)
struct FPLAbilitySet_GrantedHandles
{
	GENERATED_BODY()

public:
	void AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle);
	void TakeFromAbilitySystem(UAbilitySystemComponent* AbilitySystemComponent);
	bool HasAnyHandles() const { return AbilitySpecHandles.Num() > 0; }

private:
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;
};

/**
 * Data asset used to grant a group of abilities to an ASC.
 * Example: player starter abilities, enemy abilities, weapon abilities.
 */
UCLASS(BlueprintType)
class PROJECTLOGOS_API UPL_AbilitySet : public UDataAsset
{
	GENERATED_BODY()

public:
	void GiveToAbilitySystem(
		UAbilitySystemComponent* AbilitySystemComponent,
		FPLAbilitySet_GrantedHandles* OutGrantedHandles,
		UObject* SourceObject = nullptr
	) const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Abilities")
	TArray<FPLAbilitySet_GameplayAbility> GrantedGameplayAbilities;
};
