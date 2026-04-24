#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AbilitySystemInterface.h"
#include "BasePawn.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UCharacterMoverComponent;
class UPL_MoverPawnComponent;
class UPL_AbilitySystemComponent;


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

	UFUNCTION(BlueprintPure, Category="Mover")
	FVector GetMoverVelocity() const;
	
	UFUNCTION(BlueprintPure, Category="Mover")
	float GetGroundSpeed() const;
	
	UFUNCTION(BlueprintPure, Category="Mover")
	bool IsMoving() const;
	
protected:
	virtual void BeginPlay() override;
	
	// Collision body used by Mover.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	// Main visible character mesh.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	// Unreal Mover movement component.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCharacterMoverComponent> CharacterMoverComponent;
	
	// Produces movement input for CharacterMoverComponent.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UPL_MoverPawnComponent> MoverPawnComponent;
	
	// Project GAS component used by this pawn.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UPL_AbilitySystemComponent> AbilitySystemComponent;
};
