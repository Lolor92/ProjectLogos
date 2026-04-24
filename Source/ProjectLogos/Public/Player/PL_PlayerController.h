#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PL_PlayerController.generated.h"

class UInputMappingContext;

/**
 * Project player controller.
 * Owns local player input mapping setup.
 */
UCLASS()
class PROJECTLOGOS_API APL_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	APL_PlayerController();

protected:
	virtual void BeginPlay() override;

	// Input mapping used by the local player.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
};
