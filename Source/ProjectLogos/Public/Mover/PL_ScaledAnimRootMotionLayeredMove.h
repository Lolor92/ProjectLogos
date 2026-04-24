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
