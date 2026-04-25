#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "PL_AbilitySystemComponent.generated.h"

class UPL_GameplayAbility;
class UAnimMontage;

UCLASS()
class PROJECTLOGOS_API UPL_AbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UPL_AbilitySystemComponent();

	void SetActiveMoverMontageAbility(UPL_GameplayAbility* InAbility, UAnimMontage* InMontage);
	void ClearActiveMoverMontageAbility(UPL_GameplayAbility* InAbility, UAnimMontage* InMontage);

	UPL_GameplayAbility* GetActiveMoverMontageAbility() const { return ActiveMoverMontageAbility.Get(); }
	UAnimMontage* GetActiveMoverMontage() const { return ActiveMoverMontage.Get(); }

private:
	UPROPERTY()
	TObjectPtr<UPL_GameplayAbility> ActiveMoverMontageAbility = nullptr;

	UPROPERTY()
	TObjectPtr<UAnimMontage> ActiveMoverMontage = nullptr;
};
