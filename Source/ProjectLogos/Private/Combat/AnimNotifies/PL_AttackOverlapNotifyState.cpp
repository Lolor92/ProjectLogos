#include "Combat/AnimNotifies/PL_AttackOverlapNotifyState.h"

#include "Combat/Components/PL_CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Pawn/BasePawn.h"

void UPL_AttackOverlapNotifyState::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	UPL_CombatComponent* CombatComponent = ResolveCombatComponent(MeshComp);

	if (!CombatComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("AttackOverlap NotifyBegin failed: no combat component. Mesh=%s"),
			*GetNameSafe(MeshComp));
		return;
	}

	CombatComponent->BeginAttackOverlapWindow(this, MeshComp, TraceSocketName, AttackOverlapSettings);
}

void UPL_AttackOverlapNotifyState::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (UPL_CombatComponent* CombatComponent = ResolveCombatComponent(MeshComp))
	{
		CombatComponent->EndAttackOverlapWindow(this, MeshComp);
	}
}

FString UPL_AttackOverlapNotifyState::GetNotifyName_Implementation() const
{
	return TraceSocketName.IsNone()
		? TEXT("Attack Overlap")
		: FString::Printf(TEXT("Attack Overlap: %s"), *TraceSocketName.ToString());
}

UPL_CombatComponent* UPL_AttackOverlapNotifyState::ResolveCombatComponent(USkeletalMeshComponent* MeshComp)
{
	if (!MeshComp)
	{
		return nullptr;
	}

	AActor* Owner = MeshComp->GetOwner();

	if (ABasePawn* BasePawn = Cast<ABasePawn>(Owner))
	{
		return BasePawn->GetCombatComponent();
	}

	return Owner ? Owner->FindComponentByClass<UPL_CombatComponent>() : nullptr;
}
