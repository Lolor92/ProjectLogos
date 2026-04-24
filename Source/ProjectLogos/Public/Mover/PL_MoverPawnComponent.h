#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MoverSimulationTypes.h"
#include "PL_MoverPawnComponent.generated.h"

class UCharacterMoverComponent;
class USceneComponent;

/**
 * Reusable bridge between pawn input and Mover.
 * The pawn owns this, but this component produces the actual Mover input.
 */
UCLASS(ClassGroup=(ProjectLogos), meta=(BlueprintSpawnableComponent))
class PROJECTLOGOS_API UPL_MoverPawnComponent : public UActorComponent, public IMoverInputProducerInterface
{
	GENERATED_BODY()

public:
	UPL_MoverPawnComponent();

	// Store the movement direction requested by input code.
	UFUNCTION(BlueprintCallable, Category="Mover")
	void RequestMoveIntent(const FVector& MoveIntent);

	// Stop movement when input is released.
	UFUNCTION(BlueprintCallable, Category="Mover")
	void ClearMoveIntent();

protected:
	// Finds and configures the owner pawn's Mover pieces.
	virtual void BeginPlay() override;

	void ResolveOwnerComponents();

	// Mover calls this when it needs movement input.
	virtual void ProduceInput_Implementation(
		int32 SimTimeMs,
		FMoverInputCmdContext& InputCmdResult
	) override;

	// Mover component on the owner pawn.
	UPROPERTY(Transient)
	TObjectPtr<UCharacterMoverComponent> CharacterMoverComponent = nullptr;

	// Component that Mover should physically move. Usually the capsule/root.
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> UpdatedComponent = nullptr;

	// Main visual component. Usually the skeletal mesh.
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> PrimaryVisualComponent = nullptr;

private:
	// Local input direction. Usually X = forward, Y = right.
	FVector CachedMoveInputIntent = FVector::ZeroVector;
};
