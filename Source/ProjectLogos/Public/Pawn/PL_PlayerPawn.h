#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "Pawn/BasePawn.h"
#include "PL_PlayerPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;

/**
 * Player-controlled pawn.
 * Input will live here, while shared body setup stays in BasePawn.
 */
UCLASS()
class PROJECTLOGOS_API APL_PlayerPawn : public ABasePawn
{
	GENERATED_BODY()

public:
	APL_PlayerPawn();

protected:
	virtual void BeginPlay() override;
	
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);


	// Keeps the camera behind the player.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	// Main player camera.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCameraComponent> FollowCamera;
	
	// Input mapping added when this pawn is controlled by a local player.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	// Movement input action. Usually WASD / left stick.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> MoveAction;

	// Camera look input action. Usually mouse / right stick.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> LookAction;
};
