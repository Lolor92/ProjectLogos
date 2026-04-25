#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PL_TagReactionData.generated.h"

class UAnimMontage;
class UGameplayEffect;

UENUM(BlueprintType)
enum class EPL_TagReactionPolicy : uint8
{
	OnAdd UMETA(DisplayName="On Add"),
	OnRemove UMETA(DisplayName="On Remove"),
	Both UMETA(DisplayName="Both")
};

USTRUCT(BlueprintType)
struct FPL_TagReactionAbility
{
	GENERATED_BODY()

	// Ability tag to activate when this reaction fires.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability")
	FGameplayTag AbilityTag;

	// Optional, for later client-side cosmetic prediction. We will not use it yet.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Prediction")
	TObjectPtr<UAnimMontage> PredictedReactionMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability", meta=(ClampMin="0.0", UIMin="0.0", Units="Seconds"))
	float DelaySeconds = 0.f;
};

USTRUCT(BlueprintType)
struct FPL_TagReactionEffects
{
	GENERATED_BODY()

	// Effects to apply to self when the reaction fires.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
	TArray<TSubclassOf<UGameplayEffect>> Apply;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects", meta=(ClampMin="0.0", UIMin="0.0", Units="Seconds"))
	float ApplyDelaySeconds = 0.f;

	// Effects to remove from self when the reaction fires.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
	TArray<TSubclassOf<UGameplayEffect>> Remove;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects", meta=(ClampMin="0.0", UIMin="0.0", Units="Seconds"))
	float RemoveDelaySeconds = 0.f;

	// Optional key so multiple tags can share the same removal timer.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
	FName RemoveTimerKey;
};

USTRUCT(BlueprintType)
struct FPL_TagReactionBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trigger")
	FGameplayTag TriggerTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trigger")
	EPL_TagReactionPolicy Policy = EPL_TagReactionPolicy::OnAdd;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ability", meta=(ShowOnlyInnerProperties))
	FPL_TagReactionAbility Ability;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects", meta=(ShowOnlyInnerProperties))
	FPL_TagReactionEffects Effects;
};

UCLASS(BlueprintType)
class PROJECTLOGOS_API UPL_TagReactionData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Tag Reactions", meta=(TitleProperty="TriggerTag"))
	TArray<FPL_TagReactionBinding> Reactions;
};
