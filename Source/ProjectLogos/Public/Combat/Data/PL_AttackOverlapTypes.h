#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "PL_AttackOverlapTypes.generated.h"

class UGameplayEffect;

UENUM(BlueprintType)
enum class EPLAttackOverlapShapeType : uint8
{
	Sphere UMETA(DisplayName="Sphere"),
	Capsule UMETA(DisplayName="Capsule"),
	Box UMETA(DisplayName="Box")
};

USTRUCT(BlueprintType)
struct FPLAttackOverlapShapeSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Shape")
	EPLAttackOverlapShapeType ShapeType = EPLAttackOverlapShapeType::Capsule;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Shape", meta=(ClampMin="0.0", EditCondition="ShapeType == EPLAttackOverlapShapeType::Sphere", EditConditionHides, HideEditConditionToggle))
	float SphereRadius = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Shape", meta=(ClampMin="0.0", EditCondition="ShapeType == EPLAttackOverlapShapeType::Capsule", EditConditionHides, HideEditConditionToggle))
	float CapsuleRadius = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Shape", meta=(ClampMin="0.0", EditCondition="ShapeType == EPLAttackOverlapShapeType::Capsule", EditConditionHides, HideEditConditionToggle))
	float CapsuleHalfHeight = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Shape", meta=(EditCondition="ShapeType == EPLAttackOverlapShapeType::Box", EditConditionHides, HideEditConditionToggle))
	FVector BoxHalfExtent = FVector(30.f, 30.f, 40.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Shape")
	FVector LocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Shape")
	FRotator LocalRotation = FRotator::ZeroRotator;
};

USTRUCT(BlueprintType)
struct FPLAttackOverlapDebugSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Debug")
	bool bDrawDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Debug", meta=(ClampMin="0.0"))
	float DrawTime = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Debug")
	FColor TraceColor = FColor::Yellow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Debug")
	FColor HitColor = FColor::Red;
};

USTRUCT(BlueprintType)
struct FPLAttackOverlapGameplayEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Effects")
	TSubclassOf<UGameplayEffect> GameplayEffectClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Effects", meta=(ClampMin="0.0"))
	float EffectLevel = 1.f;
};

UENUM(BlueprintType)
enum class EPLAttackOverlapMoveDirection : uint8
{
	None UMETA(DisplayName="None"),
	KeepCurrentDistance UMETA(DisplayName="Keep Current Distance"),
	MoveCloser UMETA(DisplayName="Move Closer"),
	MoveAway UMETA(DisplayName="Move Away"),
	SnapToDistance UMETA(DisplayName="Snap To Distance")
};

UENUM(BlueprintType)
enum class EPLAttackOverlapLateralOffsetMode : uint8
{
	KeepCurrent UMETA(DisplayName="Keep Current"),
	AddOffset UMETA(DisplayName="Add Offset"),
	SnapToOffset UMETA(DisplayName="Snap To Offset")
};

UENUM(BlueprintType)
enum class EPLAttackOverlapTransformTriggerTiming : uint8
{
	OnHit UMETA(DisplayName="On Hit"),
	OnActivation UMETA(DisplayName="On Activation"),
	Both UMETA(DisplayName="Both")
};

UENUM(BlueprintType)
enum class EPLAttackOverlapTransformRecipient : uint8
{
	Instigator UMETA(DisplayName="Instigator"),
	Target UMETA(DisplayName="Target"),
	Both UMETA(DisplayName="Both")
};

UENUM(BlueprintType)
enum class EPLAttackOverlapHitStopRecipient : uint8
{
	Instigator UMETA(DisplayName="Instigator"),
	Target UMETA(DisplayName="Target"),
	Both UMETA(DisplayName="Both")
};

UENUM(BlueprintType)
enum class EPLAttackOverlapHitStopApplicationMode : uint8
{
	OncePerNotify UMETA(DisplayName="Once Per Notify"),
	OncePerHitActor UMETA(DisplayName="Once Per Hit Actor")
};

UENUM(BlueprintType)
enum class EPLAttackOverlapReferenceActorSource : uint8
{
	Instigator UMETA(DisplayName="Instigator"),
	Target UMETA(DisplayName="Target")
};

UENUM(BlueprintType)
enum class EPLAttackOverlapTeleportType : uint8
{
	None UMETA(DisplayName="None"),
	ResetPhysics UMETA(DisplayName="Reset Physics"),
	TeleportPhysics UMETA(DisplayName="Teleport Physics")
};

static FORCEINLINE ETeleportType ToTeleportType(const EPLAttackOverlapTeleportType InTeleportType)
{
	switch (InTeleportType)
	{
	case EPLAttackOverlapTeleportType::ResetPhysics:
		return ETeleportType::ResetPhysics;

	case EPLAttackOverlapTeleportType::TeleportPhysics:
		return ETeleportType::TeleportPhysics;

	case EPLAttackOverlapTeleportType::None:
	default:
		return ETeleportType::None;
	}
}

USTRUCT(BlueprintType)
struct FPLAttackOverlapMovementSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Movement")
	EPLAttackOverlapMoveDirection MoveDirection = EPLAttackOverlapMoveDirection::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Movement",
		meta=(EditCondition="MoveDirection != EPLAttackOverlapMoveDirection::None", EditConditionHides, HideEditConditionToggle))
	EPLAttackOverlapTransformTriggerTiming TriggerTiming = EPLAttackOverlapTransformTriggerTiming::OnHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Movement",
		meta=(EditCondition="MoveDirection != EPLAttackOverlapMoveDirection::None", EditConditionHides, HideEditConditionToggle))
	EPLAttackOverlapTransformRecipient Recipient = EPLAttackOverlapTransformRecipient::Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Movement",
		meta=(EditCondition="MoveDirection != EPLAttackOverlapMoveDirection::None", EditConditionHides, HideEditConditionToggle))
	EPLAttackOverlapReferenceActorSource ReferenceActorSource = EPLAttackOverlapReferenceActorSource::Instigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Movement",
		meta=(ClampMin="0.0", EditCondition="MoveDirection != EPLAttackOverlapMoveDirection::None && MoveDirection != EPLAttackOverlapMoveDirection::KeepCurrentDistance", EditConditionHides, HideEditConditionToggle))
	float MoveDistance = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Movement",
		meta=(EditCondition="MoveDirection != EPLAttackOverlapMoveDirection::None", EditConditionHides, HideEditConditionToggle))
	EPLAttackOverlapLateralOffsetMode LateralOffsetMode = EPLAttackOverlapLateralOffsetMode::KeepCurrent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Movement",
		meta=(EditCondition="MoveDirection != EPLAttackOverlapMoveDirection::None && LateralOffsetMode != EPLAttackOverlapLateralOffsetMode::KeepCurrent", EditConditionHides, HideEditConditionToggle))
	float LateralOffset = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Movement",
		meta=(EditCondition="MoveDirection != EPLAttackOverlapMoveDirection::None", EditConditionHides, HideEditConditionToggle))
	bool bSweep = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Movement",
		meta=(EditCondition="MoveDirection != EPLAttackOverlapMoveDirection::None", EditConditionHides, HideEditConditionToggle))
	EPLAttackOverlapTeleportType TeleportType = EPLAttackOverlapTeleportType::ResetPhysics;
};

UENUM(BlueprintType)
enum class EPLAttackOverlapRotationDirection : uint8
{
	None UMETA(DisplayName="None"),
	FaceReferenceActor UMETA(DisplayName="Face Reference Actor"),
	FaceAwayFromReference UMETA(DisplayName="Face Away From Reference"),
	FaceOppositeReferenceForward UMETA(DisplayName="Face Opposite Reference Forward"),
	FaceDirection UMETA(DisplayName="Face Direction")
};

USTRUCT(BlueprintType)
struct FPLAttackOverlapRotationSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Rotation")
	EPLAttackOverlapRotationDirection RotationDirection = EPLAttackOverlapRotationDirection::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Rotation",
		meta=(EditCondition="RotationDirection != EPLAttackOverlapRotationDirection::None", EditConditionHides, HideEditConditionToggle))
	EPLAttackOverlapTransformTriggerTiming TriggerTiming = EPLAttackOverlapTransformTriggerTiming::OnHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Rotation",
		meta=(EditCondition="RotationDirection != EPLAttackOverlapRotationDirection::None", EditConditionHides, HideEditConditionToggle))
	EPLAttackOverlapTransformRecipient Recipient = EPLAttackOverlapTransformRecipient::Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Rotation",
		meta=(EditCondition="RotationDirection != EPLAttackOverlapRotationDirection::None", EditConditionHides, HideEditConditionToggle))
	EPLAttackOverlapReferenceActorSource ReferenceActorSource = EPLAttackOverlapReferenceActorSource::Instigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Rotation",
		meta=(EditCondition="RotationDirection == EPLAttackOverlapRotationDirection::FaceDirection", EditConditionHides, HideEditConditionToggle))
	FRotator DirectionToFace = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Rotation",
		meta=(EditCondition="RotationDirection != EPLAttackOverlapRotationDirection::None", EditConditionHides, HideEditConditionToggle))
	EPLAttackOverlapTeleportType TeleportType = EPLAttackOverlapTeleportType::ResetPhysics;
};

USTRUCT(BlueprintType)
struct FPLAttackOverlapHitStopSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Hit Stop")
	bool bEnableHitStop = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Hit Stop",
		meta=(EditCondition="bEnableHitStop", EditConditionHides, ClampMin="0.0", UIMin="0.0"))
	float Duration = 0.05f;

	// 0.0 = freeze, 0.1 = very slow, 1.0 = normal speed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Hit Stop",
		meta=(EditCondition="bEnableHitStop", EditConditionHides, ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float TimeScale = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Hit Stop",
		meta=(EditCondition="bEnableHitStop", EditConditionHides))
	EPLAttackOverlapHitStopRecipient Recipient = EPLAttackOverlapHitStopRecipient::Both;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Hit Stop",
		meta=(EditCondition="bEnableHitStop", EditConditionHides))
	EPLAttackOverlapHitStopApplicationMode ApplicationMode = EPLAttackOverlapHitStopApplicationMode::OncePerNotify;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Hit Stop",
		meta=(EditCondition="bEnableHitStop", EditConditionHides))
	bool bAffectAnimation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Hit Stop",
		meta=(EditCondition="bEnableHitStop", EditConditionHides))
	bool bAffectMoverRootMotion = true;

	bool IsEnabled() const
	{
		return bEnableHitStop && Duration > 0.f && TimeScale < 1.f;
	}
};

USTRUCT(BlueprintType)
struct FPLAttackOverlapBlockSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Defense|Block")
	bool bBlockable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Defense|Block",
		meta=(EditCondition="bBlockable", EditConditionHides, ClampMin="0.0", ClampMax="180.0"))
	float BlockAngleDegrees = 70.f;

	// If false, blocked hits do not move the target/instigator using the notify movement settings.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Defense|Block",
		meta=(EditCondition="bBlockable", EditConditionHides))
	bool bAllowMovementWhenBlocked = false;

	// If false, blocked hits do not rotate the target/instigator using the notify rotation settings.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Defense|Block",
		meta=(EditCondition="bBlockable", EditConditionHides))
	bool bAllowRotationWhenBlocked = false;
};

USTRUCT(BlueprintType)
struct FPLAttackOverlapDodgeSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Defense|Dodge")
	bool bDodgeable = false;
};

USTRUCT(BlueprintType)
struct FPLAttackOverlapDefenseSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Defense|Block", meta=(ShowOnlyInnerProperties))
	FPLAttackOverlapBlockSettings Block;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Defense|Dodge", meta=(ShowOnlyInnerProperties))
	FPLAttackOverlapDodgeSettings Dodge;
};

USTRUCT(BlueprintType)
struct FPLAttackOverlapWindowSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap", meta=(ShowOnlyInnerProperties))
	FPLAttackOverlapShapeSettings Shape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	// Server-only by default, because applying Gameplay Effects later should be authoritative.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap")
	bool bRequireServerAuthority = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap")
	bool bIgnoreOwner = true;

	// Prevents one attack window from hitting the same actor every tick.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap")
	bool bHitEachActorOnce = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Movement", meta=(ShowOnlyInnerProperties))
	FPLAttackOverlapMovementSettings Movement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Rotation", meta=(ShowOnlyInnerProperties))
	FPLAttackOverlapRotationSettings Rotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Hit Stop", meta=(ShowOnlyInnerProperties))
	FPLAttackOverlapHitStopSettings HitStop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Defense", meta=(ShowOnlyInnerProperties))
	FPLAttackOverlapDefenseSettings Defense;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Effects", meta=(TitleProperty="GameplayEffectClass"))
	TArray<FPLAttackOverlapGameplayEffect> GameplayEffectsToApply;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Debug", meta=(ShowOnlyInnerProperties))
	FPLAttackOverlapDebugSettings Debug;
};
