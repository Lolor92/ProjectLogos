#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PL_AnimInstance.generated.h"

class ABasePawn;

/**
 * Shared animation instance for Project Logos pawns.
 * Reads movement data from BasePawn/Mover.
 */
UCLASS()
class PROJECTLOGOS_API UPL_AnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
	UFUNCTION(BlueprintCallable, Category="Anim|Ability")
	void SetAbilityAnimState(bool bNewAbilityRootMotionActive, bool bNewShouldBlendMontage);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim|State");
	bool bIsBlocking = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim|State");
	bool bIsKnockdown = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim|State");
	bool bIsFlinching = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim|State");
	bool bIsFrozen = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim|State");
	bool bIsStunned = false;

protected:
	// Pawn that owns this animation instance.
	UPROPERTY(BlueprintReadOnly, Category="Animation")
	TObjectPtr<ABasePawn> OwningBasePawn = nullptr;

	// Horizontal movement speed from Mover.
	UPROPERTY(BlueprintReadOnly, Category="Animation")
	float GroundSpeed = 0.f;

	// True when the pawn has meaningful horizontal velocity.
	UPROPERTY(BlueprintReadOnly, Category="Animation")
	bool bIsMoving = false;

	// True when the pawn has movement input / acceleration intent.
	UPROPERTY(BlueprintReadOnly, Category="Animation")
	bool bIsAccelerating = false;

	// True when Mover says the pawn is falling.
	UPROPERTY(BlueprintReadOnly, Category="Animation")
	bool bIsInAir = false;

	// True when Mover says the pawn is airborne, e.g. falling/flying.
	UPROPERTY(BlueprintReadOnly, Category="Animation")
	bool bIsAirborne = false;
	
	// Movement direction relative to pawn facing. Useful for 2D blendspaces.
	UPROPERTY(BlueprintReadOnly, Category="Animation")
	float MovementDirectionYaw = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim|Ability")
	bool bAbilityRootMotionActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim|Ability")
	bool bShouldBlendMontage = false;
};
