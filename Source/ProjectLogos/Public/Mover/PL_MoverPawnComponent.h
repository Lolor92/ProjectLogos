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

	UFUNCTION(BlueprintCallable, Category="Mover|Facing")
	void RequestForcedFacingYaw(float Yaw);

	// Store the movement direction requested by input code.
	UFUNCTION(BlueprintCallable, Category="Mover")
	void RequestMoveIntent(const FVector& MoveIntent);

	// Stop movement when input is released.
	UFUNCTION(BlueprintCallable, Category="Mover")
	void ClearMoveIntent();

	// Returns the currently cached movement input.
	UFUNCTION(BlueprintPure, Category="Mover")
	FVector GetMoveIntent() const { return CachedMoveInputIntent; }

	// True when we have meaningful 2D movement input.
	UFUNCTION(BlueprintPure, Category="Mover")
	bool HasMoveInput() const { return CachedMoveInputIntent.SizeSquared2D() > KINDA_SMALL_NUMBER; }

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
	
	// If true, player-controlled pawns face camera yaw while moving.
	// If false, pawns face their movement direction.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mover")
	bool bOrientToCameraYaw = true;

private:
	void ApplyFacingSnapOnce(float Yaw) const;
	void WriteCurrentTransformToMoverSyncState() const;

	UFUNCTION(Server, Reliable)
	void ServerRequestForcedFacingYaw(float Yaw);

	bool bHasForcedFacingIntent = false;

	FVector ForcedFacingIntent = FVector::ZeroVector;

	// Local input direction. Usually X = forward, Y = right.
	FVector CachedMoveInputIntent = FVector::ZeroVector;
};
