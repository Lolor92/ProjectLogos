#pragma once

#include "CoreMinimal.h"
#include "Pawn/BasePawn.h"
#include "PL_PlayerPawn.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UPL_InputComponent;

/**
 * Player-controlled pawn.
 * Owns camera/body components. Input is handled by PL_InputComponent.
 */
UCLASS()
class PROJECTLOGOS_API APL_PlayerPawn : public ABasePawn
{
	GENERATED_BODY()

public:
	APL_PlayerPawn();

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	void InitializePlayerAbilitySystem();

	// Keeps the camera behind the player.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	// Main player camera.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCameraComponent> FollowCamera;

	// Handles movement, look, and ability input.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UPL_InputComponent> PlayerInputComponent;
};
