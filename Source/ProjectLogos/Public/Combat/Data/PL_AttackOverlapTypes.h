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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Effects", meta=(TitleProperty="GameplayEffectClass"))
	TArray<FPLAttackOverlapGameplayEffect> GameplayEffectsToApply;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap|Debug", meta=(ShowOnlyInnerProperties))
	FPLAttackOverlapDebugSettings Debug;
};
