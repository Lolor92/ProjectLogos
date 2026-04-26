#pragma once

#include "CoreMinimal.h"
#include "Combat/Data/PL_AttackOverlapTypes.h"
#include "Combat/Data/PL_TagReactionData.h"
#include "Components/ActorComponent.h"
#include "GAS/Data/PL_AbilitySet.h"
#include "GameplayTagContainer.h"
#include "PL_CombatComponent.generated.h"

class ABasePawn;
class UAbilitySystemComponent;
class UAnimNotifyState;
class UPL_AbilitySystemComponent;
class USkeletalMeshComponent;
class FBoolProperty;
class FPLCombatTagReactionRuntime;

struct FPLActiveAttackOverlapWindow
{
	const UAnimNotifyState* NotifyState = nullptr;

	TWeakObjectPtr<USkeletalMeshComponent> MeshComp;

	FName TraceSocketName = NAME_None;

	FPLAttackOverlapWindowSettings Settings;

	bool bHasPreviousTransform = false;

	FTransform PreviousShapeTransform = FTransform::Identity;

	bool bHitStopAppliedThisWindow = false;

	TSet<TWeakObjectPtr<AActor>> HitActors;

	TSet<TWeakObjectPtr<AActor>> HitStopAppliedActors;
};

// Drives an AnimInstance bool from one or more gameplay tags.
USTRUCT(BlueprintType)
struct FPL_AnimBoolBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim Bool")
	FGameplayTagContainer Tags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anim Bool")
	FName AnimBoolName;

	FBoolProperty* CachedBoolProperty = nullptr;
};

/**
 * Combat component owns combat startup setup.
 * It receives the pawn's ASC and grants default combat abilities on the server.
 */
UCLASS(ClassGroup=(ProjectLogos), meta=(BlueprintSpawnableComponent))
class PROJECTLOGOS_API UPL_CombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPL_CombatComponent();
	virtual ~UPL_CombatComponent() override;

	void InitializeCombat(ABasePawn* InOwnerPawn, UPL_AbilitySystemComponent* InAbilitySystemComponent);

	void NotifyCombatTagReaction(const FGameplayTag& TriggerTag, bool bAdded);
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category="Combat")
	UPL_AbilitySystemComponent* GetProjectAbilitySystemComponent() const { return AbilitySystemComponent; }

	UFUNCTION(BlueprintPure, Category="Combat")
	ABasePawn* GetOwnerPawn() const { return OwnerPawn; }

	bool BeginAttackOverlapWindow(
		const UAnimNotifyState* NotifyState,
		USkeletalMeshComponent* MeshComp,
		FName TraceSocketName,
		const FPLAttackOverlapWindowSettings& Settings
	);

	void EndAttackOverlapWindow(
		const UAnimNotifyState* NotifyState,
		USkeletalMeshComponent* MeshComp
	);

protected:
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	UFUNCTION(BlueprintImplementableEvent, Category="Combat|Attack Overlap")
	void BP_OnAttackOverlapDetected(
		AActor* HitActor,
		const FHitResult& HitResult,
		USkeletalMeshComponent* SourceMesh
	);

	// Combat abilities granted when combat is initialized on the server.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Abilities")
	TArray<TObjectPtr<UPL_AbilitySet>> DefaultAbilitySets;

	// Gameplay tag driven reactions.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tag Reactions")
	TObjectPtr<UPL_TagReactionData> TagReactionData = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tag Reactions", meta=(TitleProperty="AnimBoolName"))
	TArray<FPL_AnimBoolBinding> AnimBoolBindings;

	UFUNCTION(BlueprintImplementableEvent, Category="Combat|Tag Reactions")
	void BP_OnCombatTagReaction(FGameplayTag TriggerTag, bool bAdded);

private:
	void ProcessAttackOverlapWindow(FPLActiveAttackOverlapWindow& Window);
	void HandleAttackOverlapHit(FPLActiveAttackOverlapWindow& Window, const FHitResult& Hit);
	void ApplyAttackOverlapTransformEffects(
		const FPLActiveAttackOverlapWindow& Window,
		AActor* HitActor,
		const FHitResult& Hit
	);
	void ApplyAttackOverlapGameplayEffects(
		const FPLActiveAttackOverlapWindow& Window,
		AActor* HitActor,
		const FHitResult& Hit
	);
	void ApplyAttackOverlapHitStop(
		FPLActiveAttackOverlapWindow& Window,
		AActor* HitActor
	);
	void ApplyAttackOverlapRotation(
		const FPLAttackOverlapRotationSettings& RotationSettings,
		AActor* InstigatorActor,
		AActor* TargetActor
	);
	void ApplyAttackOverlapMovement(
		const FPLAttackOverlapMovementSettings& MovementSettings,
		AActor* InstigatorActor,
		AActor* TargetActor
	);
	AActor* ResolveTransformRecipient(
		EPLAttackOverlapTransformRecipient Recipient,
		AActor* InstigatorActor,
		AActor* TargetActor
	) const;
	AActor* ResolveReferenceActor(
		EPLAttackOverlapReferenceActorSource Source,
		AActor* InstigatorActor,
		AActor* TargetActor
	) const;
	void ApplyMoverAwareTransformToActor(
		AActor* Actor,
		const FVector& NewLocation,
		const FRotator& NewRotation,
		bool bApplyLocation,
		bool bApplyRotation,
		bool bSweep,
		ETeleportType TeleportType
	) const;
	void ApplyHitStopToActor(
		AActor* Actor,
		const FPLAttackOverlapHitStopSettings& HitStopSettings
	) const;
	UAbilitySystemComponent* GetTargetAbilitySystemComponent(AActor* TargetActor) const;
	void DrawAttackOverlapDebug(const FPLActiveAttackOverlapWindow& Window, const FTransform& ShapeTransform, bool bHit) const;

	static FCollisionShape MakeAttackOverlapCollisionShape(const FPLAttackOverlapShapeSettings& ShapeSettings);
	static FTransform MakeAttackOverlapShapeTransform(
		const FTransform& SocketTransform,
		const FPLAttackOverlapShapeSettings& ShapeSettings
	);

	void GrantDefaultAbilities();
	void ClearDefaultAbilities();

	FPLCombatTagReactionRuntime* TagReactionRuntime = nullptr;

	TArray<FPLActiveAttackOverlapWindow> ActiveAttackOverlapWindows;

	UPROPERTY(Transient)
	TObjectPtr<ABasePawn> OwnerPawn = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UPL_AbilitySystemComponent> AbilitySystemComponent = nullptr;

	UPROPERTY(Transient)
	FPLAbilitySet_GrantedHandles GrantedHandles;

	bool bDefaultAbilitiesGranted = false;
};
