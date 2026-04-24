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

protected:
	// Initializes this pawn as the avatar for an ASC.
	void InitializeAbilitySystem(UPL_AbilitySystemComponent* InAbilitySystemComponent, AActor* OwnerActor);

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
