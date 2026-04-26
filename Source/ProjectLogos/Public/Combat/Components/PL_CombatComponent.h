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

	UFUNCTION(BlueprintPure, Category="Combat|Block")
	bool IsBlockingActive() const;

	UFUNCTION(BlueprintPure, Category="Combat|Dodge")
	bool IsDodgingActive() const;

	UFUNCTION(BlueprintPure, Category="Combat|Crowd Control")
	bool IsCrowdControlActive() const;

	UFUNCTION(BlueprintPure, Category="Combat|Super Armor")
	bool HasSuperArmorAtOrAbove(EPLAttackOverlapSuperArmorLevel RequiredLevel) const;

	UFUNCTION(BlueprintPure, Category="Combat|Super Armor")
	EPLAttackOverlapSuperArmorLevel GetCurrentSuperArmorLevel() const;

	const FGameplayTag& GetBlockingTag() const { return BlockingTag; }
	const FGameplayTag& GetCrowdControlTag() const { return CrowdControlTag; }

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Block")
	FGameplayTag BlockingTag;

	// Applied to the attacker when their hit is blocked.
	// Example: GE_Trigger_AttackBlocked, GE_BlockRecoil, etc.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Block")
	TSubclassOf<UGameplayEffect> AttackerBlockedEffectClass;

	// Applied to the defender when they successfully block.
	// Example: GE_Trigger_BlockSuccess, GE_BlockImpact, etc.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Block")
	TSubclassOf<UGameplayEffect> DefenderBlockedEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Dodge")
	FGameplayTag DodgingTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Crowd Control")
	FGameplayTag CrowdControlTag;

	// Applied to attacker when their hit is dodged.
	// Example: GE_Trigger_AttackDodged, GE_WhiffRecovery, etc.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Dodge")
	TSubclassOf<UGameplayEffect> AttackerDodgedEffectClass;

	// Applied to defender when they successfully dodge.
	// Example: GE_Trigger_DodgeSuccess.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Dodge")
	TSubclassOf<UGameplayEffect> DefenderDodgedEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Super Armor")
	FGameplayTag SuperArmor1Tag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Super Armor")
	FGameplayTag SuperArmor2Tag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Super Armor")
	FGameplayTag SuperArmor3Tag;

	// Applied to attacker when their attack hits super armor.
	// Example: GE_Trigger_AttackSuperArmored.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Super Armor")
	TSubclassOf<UGameplayEffect> AttackerSuperArmoredEffectClass;

	// Applied to defender when their super armor absorbs the hit.
	// Example: GE_Trigger_SuperArmorSuccess.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Super Armor")
	TSubclassOf<UGameplayEffect> DefenderSuperArmoredEffectClass;

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
		const FHitResult& Hit,
		bool bWasBlocked,
		bool bWasDodged,
		bool bHasSuperArmor
	);
	void ApplyAttackOverlapDamageGameplayEffects(
		const FPLActiveAttackOverlapWindow& Window,
		AActor* HitActor,
		const FHitResult& Hit
	) const;
	void ApplyAttackOverlapReactionGameplayEffects(
		const FPLActiveAttackOverlapWindow& Window,
		AActor* HitActor,
		const FHitResult& Hit
	) const;
	void ApplyAttackOverlapHitStop(
		FPLActiveAttackOverlapWindow& Window,
		AActor* HitActor
	);
	void ApplyAttackOverlapRotation(
		const FPLAttackOverlapRotationSettings& RotationSettings,
		const FPLAttackOverlapBlockSettings& BlockSettings,
		AActor* InstigatorActor,
		AActor* TargetActor,
		bool bWasBlocked,
		bool bWasDodged,
		bool bHasSuperArmor
	);
	void ApplyAttackOverlapMovement(
		const FPLAttackOverlapMovementSettings& MovementSettings,
		const FPLAttackOverlapBlockSettings& BlockSettings,
		AActor* InstigatorActor,
		AActor* TargetActor,
		bool bWasBlocked,
		bool bWasDodged,
		bool bHasSuperArmor
	);
	bool IsAttackBlocked(
		AActor* HitActor,
		const FPLAttackOverlapBlockSettings& BlockSettings
	) const;
	bool IsAttackDodged(
		AActor* HitActor,
		const FPLAttackOverlapDodgeSettings& DodgeSettings
	) const;
	bool HasRequiredSuperArmor(
		AActor* HitActor,
		EPLAttackOverlapSuperArmorLevel RequiredLevel
	) const;
	static bool IsWithinBlockAngle(
		const AActor* DefenderActor,
		const AActor* AttackerActor,
		float BlockAngleDegrees
	);
	void ApplyBlockGameplayEffects(
		AActor* HitActor,
		const FHitResult& Hit
	) const;
	void ApplyDodgeGameplayEffects(
		AActor* HitActor,
		const FHitResult& Hit
	) const;
	void ApplySuperArmorGameplayEffects(
		AActor* HitActor,
		const FHitResult& Hit
	) const;
	void ApplyGameplayEffectToActor(
		AActor* RecipientActor,
		TSubclassOf<UGameplayEffect> GameplayEffectClass,
		float EffectLevel,
		const FHitResult* Hit
	) const;
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
	void BindCrowdControlTagEvent();
	void ClearCrowdControlTagEvent();
	void OnCrowdControlTagChanged(FGameplayTag Tag, int32 NewCount);
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

	FGameplayTag BoundCrowdControlTag;
	FDelegateHandle CrowdControlTagDelegateHandle;

	bool bDefaultAbilitiesGranted = false;
};
