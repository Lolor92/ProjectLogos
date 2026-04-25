#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Combat/Data/PL_AttackOverlapTypes.h"
#include "PL_AttackOverlapNotifyState.generated.h"

class UPL_CombatComponent;

UCLASS(meta=(DisplayName="Attack Overlap Window"))
class PROJECTLOGOS_API UPL_AttackOverlapNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference
	) override;

	virtual void NotifyEnd(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference
	) override;

	virtual FString GetNotifyName_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap")
	FName TraceSocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attack Overlap", meta=(ShowOnlyInnerProperties))
	FPLAttackOverlapWindowSettings AttackOverlapSettings;

private:
	static UPL_CombatComponent* ResolveCombatComponent(USkeletalMeshComponent* MeshComp);
};
