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
	
	// Movement direction relative to pawn facing. Useful for 2D blendspaces.
	UPROPERTY(BlueprintReadOnly, Category="Animation")
	float MovementDirectionYaw = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim|Ability")
	bool bAbilityRootMotionActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim|Ability")
	bool bShouldBlendMontage = false;
};
