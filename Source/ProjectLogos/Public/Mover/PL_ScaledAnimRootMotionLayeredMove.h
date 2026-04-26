#pragma once

#include "CoreMinimal.h"
#include "DefaultMovementSet/LayeredMoves/AnimRootMotionLayeredMove.h"
#include "PL_ScaledAnimRootMotionLayeredMove.generated.h"

// What kind of pawn collision should stop scaled root motion.
UENUM(BlueprintType)
enum class EPLRootMotionCollisionStopMode : uint8
{
	None UMETA(DisplayName="None"),
	Capsule UMETA(DisplayName="Capsule"),
	Mesh UMETA(DisplayName="Mesh"),
	CapsuleOrMesh UMETA(DisplayName="Capsule Or Mesh")
};

/**
 * Mover layered move that extracts root motion from a montage.
 * Allows translation scaling for GAS montage abilities.
 */
USTRUCT(BlueprintType)
struct PROJECTLOGOS_API FPL_ScaledAnimRootMotionLayeredMove : public FLayeredMove_AnimRootMotion
{
	GENERATED_BODY()

	FPL_ScaledAnimRootMotionLayeredMove();
	virtual ~FPL_ScaledAnimRootMotionLayeredMove() {}

	// Multiplies montage root motion translation.
	UPROPERTY(BlueprintReadWrite, Category="Mover")
	float RootMotionTranslationScale = 1.f;

	// Optional pawn collision check for stopping root motion translation.
	UPROPERTY(BlueprintReadWrite, Category="Mover")
	EPLRootMotionCollisionStopMode RootMotionCollisionStopMode = EPLRootMotionCollisionStopMode::None;

	// Only stop root motion if the blocking pawn is inside this forward cone.
	// 40 means +/-40 degrees from character forward.
	UPROPERTY(BlueprintReadWrite, Category="Mover")
	float RootMotionCollisionForwardAngleDegrees = 40.f;
	
	UPROPERTY(BlueprintReadWrite, Category="Mover")
	bool bUseRootMotionRelease = false;

	UPROPERTY(BlueprintReadWrite, Category="Mover")
	float RootMotionReleasePosition = 0.f;

	UPROPERTY(BlueprintReadWrite, Category="Mover")
	bool bRequireMoveInputForRootMotionRelease = true;

	UPROPERTY()
	float HitStopAdjustedMontagePosition = -1.f;

	virtual bool GenerateMove(
		const FMoverTickStartData& StartState,
		const FMoverTimeStep& TimeStep,
		const UMoverComponent* MoverComp,
		UMoverBlackboard* SimBlackboard,
		FProposedMove& OutProposedMove
	) override;

	virtual FLayeredMoveBase* Clone() const override;
	virtual void NetSerialize(FArchive& Ar) override;
	virtual UScriptStruct* GetScriptStruct() const override;
	virtual FString ToSimpleString() const override;

private:
	bool HasMovementInput(const FMoverTickStartData& StartState) const;
	
	bool HasBlockingPawnCollision(
		const UMoverComponent* MoverComp,
		const FVector& WorldTranslation
	) const;
};

template<>
struct TStructOpsTypeTraits<FPL_ScaledAnimRootMotionLayeredMove>
	: public TStructOpsTypeTraitsBase2<FPL_ScaledAnimRootMotionLayeredMove>
{
	enum
	{
		WithCopy = true
	};
};
