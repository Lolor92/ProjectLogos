#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Pawn.h"
#include "BasePawn.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UCharacterMoverComponent;
class UPL_MoverPawnComponent;
class UAbilitySystemComponent;
class UPL_AbilitySystemComponent;
class UPL_CombatComponent;
class UAnimMontage;

USTRUCT(BlueprintType)
struct FPLRepAbilityAnimState
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 bAbilityRootMotionActive : 1;

	UPROPERTY()
	uint8 bShouldBlendMontage : 1;

	FPLRepAbilityAnimState()
		: bAbilityRootMotionActive(false)
		, bShouldBlendMontage(false)
	{
	}

	bool operator==(const FPLRepAbilityAnimState& Other) const
	{
		return bAbilityRootMotionActive == Other.bAbilityRootMotionActive
			&& bShouldBlendMontage == Other.bShouldBlendMontage;
	}

	bool operator!=(const FPLRepAbilityAnimState& Other) const
	{
		return !(*this == Other);
	}
};

USTRUCT(BlueprintType)
struct FPLRepAbilityMontageVisualState
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY()
	float PlayRate = 1.f;

	UPROPERTY()
	float StartPosition = 0.f;

	UPROPERTY()
	float ServerStartTime = 0.f;

	UPROPERTY()
	uint8 bIsPlaying : 1 = false;

	UPROPERTY()
	uint8 SequenceId = 0;
};

/**
 * Shared base pawn body.
 * Player and NPC pawns inherit from this.
 */
UCLASS()
class PROJECTLOGOS_API ABasePawn : public APawn, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABasePawn();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category="AbilitySystem")
	UPL_AbilitySystemComponent* GetProjectAbilitySystemComponent() const { return AbilitySystemComponent; }

	UFUNCTION(BlueprintPure, Category="Components")
	UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComponent; }

	UFUNCTION(BlueprintPure, Category="Components")
	USkeletalMeshComponent* GetMeshComponent() const { return MeshComponent; }

	UFUNCTION(BlueprintPure, Category="Components")
	UCharacterMoverComponent* GetCharacterMoverComponent() const { return CharacterMoverComponent; }

	UFUNCTION(BlueprintPure, Category="Components")
	UPL_MoverPawnComponent* GetMoverPawnComponent() const { return MoverPawnComponent; }

	UFUNCTION(BlueprintPure, Category="Components")
	UPL_CombatComponent* GetCombatComponent() const { return CombatComponent; }

	UFUNCTION(BlueprintPure, Category="Mover")
	FVector GetMoverVelocity() const;

	UFUNCTION(BlueprintPure, Category="Mover")
	float GetGroundSpeed() const;

	UFUNCTION(BlueprintPure, Category="Mover")
	bool IsMoving() const;
	
	UFUNCTION(BlueprintCallable, Category="Ability|Animation")
	void SetAbilityAnimState(const FPLRepAbilityAnimState& NewState);

	UFUNCTION(BlueprintCallable, Category="Ability|Animation")
	void SetAbilityAnimStateValues(bool bNewAbilityRootMotionActive, bool bNewShouldBlendMontage);

	UFUNCTION(BlueprintCallable, Category="Ability|Animation")
	void ResetAbilityAnimState();

	UFUNCTION(BlueprintPure, Category="Ability|Animation")
	const FPLRepAbilityAnimState& GetAbilityAnimState() const { return AbilityAnimState; }

	UFUNCTION(BlueprintPure, Category="Ability|Animation")
	bool IsAbilityRootMotionActive() const { return AbilityAnimState.bAbilityRootMotionActive; }

	UFUNCTION(BlueprintPure, Category="Ability|Animation")
	bool ShouldBlendMontage() const { return AbilityAnimState.bShouldBlendMontage; }

	void StartAbilityMontageVisual(UAnimMontage* Montage, float PlayRate, float StartPosition);
	void StopAbilityMontageVisual(UAnimMontage* Montage);
	void EnsureAbilityMontageVisual();
	bool IsAbilityMontageVisualExpired() const;

protected:
	// Initializes this pawn as the avatar for an ASC.
	void InitializeAbilitySystem(UPL_AbilitySystemComponent* InAbilitySystemComponent, AActor* OwnerActor);
	
	UPROPERTY(ReplicatedUsing=OnRep_AbilityAnimState, BlueprintReadOnly, Category="Ability|Animation")
	FPLRepAbilityAnimState AbilityAnimState;

	UFUNCTION()
	void OnRep_AbilityAnimState();

	UFUNCTION(Server, Reliable)
	void ServerSetAbilityAnimState(const FPLRepAbilityAnimState& NewState);

	void ApplyAbilityAnimState(const FPLRepAbilityAnimState& NewState);

	UPROPERTY(ReplicatedUsing=OnRep_AbilityMontageVisualState)
	FPLRepAbilityMontageVisualState AbilityMontageVisualState;

	UFUNCTION()
	void OnRep_AbilityMontageVisualState();

	void ApplyAbilityMontageVisualState();

	float GetServerTimeSecondsSafe() const;

	// Main collision body. Mover moves this.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	// Visible character mesh.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	// Mover movement simulation component.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCharacterMoverComponent> CharacterMoverComponent;

	// Produces input for Mover.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UPL_MoverPawnComponent> MoverPawnComponent;

	// Owns combat setup and combat ability grants.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UPL_CombatComponent> CombatComponent;

	// ASC currently used by this pawn.
	// For players, this comes from PlayerState.
	UPROPERTY(BlueprintReadOnly, Category="AbilitySystem")
	TObjectPtr<UPL_AbilitySystemComponent> AbilitySystemComponent;
};
