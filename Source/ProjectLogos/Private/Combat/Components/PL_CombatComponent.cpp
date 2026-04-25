#include "Combat/Components/PL_CombatComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Combat/Runtime/PL_CombatTagReactionRuntime.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GAS/Component/PL_AbilitySystemComponent.h"
#include "GAS/Data/PL_AbilitySet.h"
#include "GameplayEffect.h"
#include "Pawn/BasePawn.h"

UPL_CombatComponent::UPL_CombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickEnabled(false);

	SetIsReplicatedByDefault(true);

	TagReactionRuntime = new FPLCombatTagReactionRuntime(*this);
}

UPL_CombatComponent::~UPL_CombatComponent()
{
	delete TagReactionRuntime;
	TagReactionRuntime = nullptr;
}

void UPL_CombatComponent::InitializeCombat(
	ABasePawn* InOwnerPawn,
	UPL_AbilitySystemComponent* InAbilitySystemComponent
)
{
	if (!InOwnerPawn || !InAbilitySystemComponent)
	{
		return;
	}

	OwnerPawn = InOwnerPawn;
	AbilitySystemComponent = InAbilitySystemComponent;

	if (OwnerPawn->HasAuthority())
	{
		GrantDefaultAbilities();
	}

	if (TagReactionRuntime)
	{
		TagReactionRuntime->Initialize(
			OwnerPawn,
			AbilitySystemComponent,
			TagReactionData,
			AnimBoolBindings
		);
	}
}

void UPL_CombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TagReactionRuntime)
	{
		TagReactionRuntime->Deinitialize();
	}

	ClearDefaultAbilities();

	Super::EndPlay(EndPlayReason);
}

void UPL_CombatComponent::NotifyCombatTagReaction(
	const FGameplayTag& TriggerTag,
	const bool bAdded)
{
	BP_OnCombatTagReaction(TriggerTag, bAdded);
}

void UPL_CombatComponent::GrantDefaultAbilities()
{
	if (bDefaultAbilitiesGranted || !AbilitySystemComponent)
	{
		return;
	}

	for (const UPL_AbilitySet* AbilitySet : DefaultAbilitySets)
	{
		if (!AbilitySet)
		{
			continue;
		}

		// SourceObject is the combat component, because it owns this combat loadout.
		AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, &GrantedHandles, this);
	}

	bDefaultAbilitiesGranted = true;
}

void UPL_CombatComponent::ClearDefaultAbilities()
{
	if (AbilitySystemComponent && GrantedHandles.HasAnyHandles())
	{
		GrantedHandles.TakeFromAbilitySystem(AbilitySystemComponent);
	}

	bDefaultAbilitiesGranted = false;
}

bool UPL_CombatComponent::BeginAttackOverlapWindow(
	const UAnimNotifyState* NotifyState,
	USkeletalMeshComponent* MeshComp,
	FName TraceSocketName,
	const FPLAttackOverlapWindowSettings& Settings)
{
	if (!NotifyState || !MeshComp)
	{
		return false;
	}

	AActor* OwnerActor = GetOwner();

	if (!OwnerActor)
	{
		return false;
	}

	if (Settings.bRequireServerAuthority && !OwnerActor->HasAuthority())
	{
		return false;
	}

	FPLActiveAttackOverlapWindow& NewWindow = ActiveAttackOverlapWindows.AddDefaulted_GetRef();

	NewWindow.NotifyState = NotifyState;
	NewWindow.MeshComp = MeshComp;
	NewWindow.TraceSocketName = TraceSocketName;
	NewWindow.Settings = Settings;
	NewWindow.bHasPreviousTransform = false;
	NewWindow.PreviousShapeTransform = FTransform::Identity;
	NewWindow.HitActors.Reset();

	SetComponentTickEnabled(true);

	UE_LOG(LogTemp, Verbose, TEXT("BeginAttackOverlapWindow. Owner=%s Mesh=%s Socket=%s"),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(MeshComp),
		*TraceSocketName.ToString());

	return true;
}

void UPL_CombatComponent::EndAttackOverlapWindow(
	const UAnimNotifyState* NotifyState,
	USkeletalMeshComponent* MeshComp)
{
	for (int32 Index = ActiveAttackOverlapWindows.Num() - 1; Index >= 0; --Index)
	{
		const FPLActiveAttackOverlapWindow& Window = ActiveAttackOverlapWindows[Index];

		if (Window.NotifyState == NotifyState && Window.MeshComp.Get() == MeshComp)
		{
			ActiveAttackOverlapWindows.RemoveAtSwap(Index);
		}
	}

	if (ActiveAttackOverlapWindows.IsEmpty())
	{
		SetComponentTickEnabled(false);
	}
}

void UPL_CombatComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	for (int32 Index = ActiveAttackOverlapWindows.Num() - 1; Index >= 0; --Index)
	{
		FPLActiveAttackOverlapWindow& Window = ActiveAttackOverlapWindows[Index];

		if (!Window.MeshComp.IsValid())
		{
			ActiveAttackOverlapWindows.RemoveAtSwap(Index);
			continue;
		}

		ProcessAttackOverlapWindow(Window);
	}

	if (ActiveAttackOverlapWindows.IsEmpty())
	{
		SetComponentTickEnabled(false);
	}
}

void UPL_CombatComponent::ProcessAttackOverlapWindow(FPLActiveAttackOverlapWindow& Window)
{
	USkeletalMeshComponent* MeshComp = Window.MeshComp.Get();

	if (!MeshComp)
	{
		return;
	}

	UWorld* World = MeshComp->GetWorld();

	if (!World)
	{
		return;
	}

	const FTransform SocketTransform =
		!Window.TraceSocketName.IsNone() && MeshComp->DoesSocketExist(Window.TraceSocketName)
			? MeshComp->GetSocketTransform(Window.TraceSocketName, RTS_World)
			: MeshComp->GetComponentTransform();

	const FTransform CurrentShapeTransform =
		MakeAttackOverlapShapeTransform(SocketTransform, Window.Settings.Shape);

	const FCollisionShape CollisionShape =
		MakeAttackOverlapCollisionShape(Window.Settings.Shape);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PLAttackOverlap), false);

	if (Window.Settings.bIgnoreOwner && GetOwner())
	{
		QueryParams.AddIgnoredActor(GetOwner());
	}

	TArray<FHitResult> Hits;

	if (Window.bHasPreviousTransform)
	{
		World->SweepMultiByChannel(
			Hits,
			Window.PreviousShapeTransform.GetLocation(),
			CurrentShapeTransform.GetLocation(),
			CurrentShapeTransform.GetRotation(),
			Window.Settings.TraceChannel.GetValue(),
			CollisionShape,
			QueryParams
		);
	}
	else
	{
		TArray<FOverlapResult> Overlaps;

		World->OverlapMultiByChannel(
			Overlaps,
			CurrentShapeTransform.GetLocation(),
			CurrentShapeTransform.GetRotation(),
			Window.Settings.TraceChannel.GetValue(),
			CollisionShape,
			QueryParams
		);

		for (const FOverlapResult& Overlap : Overlaps)
		{
			FHitResult Hit;
			Hit.HitObjectHandle = Overlap.OverlapObjectHandle;
			Hit.Component = Overlap.Component;
			Hit.Location = CurrentShapeTransform.GetLocation();
			Hit.ImpactPoint = CurrentShapeTransform.GetLocation();
			Hit.TraceStart = CurrentShapeTransform.GetLocation();
			Hit.TraceEnd = CurrentShapeTransform.GetLocation();
			Hits.Add(Hit);
		}
	}

	bool bAnyHit = false;

	for (const FHitResult& Hit : Hits)
	{
		if (Hit.GetActor())
		{
			bAnyHit = true;
			HandleAttackOverlapHit(Window, Hit);
		}
	}

	DrawAttackOverlapDebug(Window, CurrentShapeTransform, bAnyHit);

	Window.PreviousShapeTransform = CurrentShapeTransform;
	Window.bHasPreviousTransform = true;
}

void UPL_CombatComponent::HandleAttackOverlapHit(
	FPLActiveAttackOverlapWindow& Window,
	const FHitResult& Hit)
{
	AActor* HitActor = Hit.GetActor();

	if (!HitActor)
	{
		return;
	}

	if (HitActor == GetOwner())
	{
		return;
	}

	if (Window.Settings.bHitEachActorOnce)
	{
		const TWeakObjectPtr<AActor> HitActorKey(HitActor);

		if (Window.HitActors.Contains(HitActorKey))
		{
			return;
		}

		Window.HitActors.Add(HitActorKey);
	}

	UE_LOG(LogTemp, Log, TEXT("Attack overlap detected. Source=%s Target=%s"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(HitActor));

	ApplyAttackOverlapGameplayEffects(Window, HitActor, Hit);

	BP_OnAttackOverlapDetected(HitActor, Hit, Window.MeshComp.Get());
}

void UPL_CombatComponent::DrawAttackOverlapDebug(
	const FPLActiveAttackOverlapWindow& Window,
	const FTransform& ShapeTransform,
	bool bHit) const
{
	if (!Window.Settings.Debug.bDrawDebug)
	{
		return;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	const FColor DrawColor = bHit
		? Window.Settings.Debug.HitColor
		: Window.Settings.Debug.TraceColor;

	const float DrawTime = Window.Settings.Debug.DrawTime;

	switch (Window.Settings.Shape.ShapeType)
	{
	case EPLAttackOverlapShapeType::Sphere:
		DrawDebugSphere(
			World,
			ShapeTransform.GetLocation(),
			Window.Settings.Shape.SphereRadius,
			16,
			DrawColor,
			false,
			DrawTime
		);
		break;

	case EPLAttackOverlapShapeType::Capsule:
		DrawDebugCapsule(
			World,
			ShapeTransform.GetLocation(),
			Window.Settings.Shape.CapsuleHalfHeight,
			Window.Settings.Shape.CapsuleRadius,
			ShapeTransform.GetRotation(),
			DrawColor,
			false,
			DrawTime
		);
		break;

	case EPLAttackOverlapShapeType::Box:
		DrawDebugBox(
			World,
			ShapeTransform.GetLocation(),
			Window.Settings.Shape.BoxHalfExtent,
			ShapeTransform.GetRotation(),
			DrawColor,
			false,
			DrawTime
		);
		break;

	default:
		break;
	}
}

FCollisionShape UPL_CombatComponent::MakeAttackOverlapCollisionShape(
	const FPLAttackOverlapShapeSettings& ShapeSettings)
{
	switch (ShapeSettings.ShapeType)
	{
	case EPLAttackOverlapShapeType::Sphere:
		return FCollisionShape::MakeSphere(ShapeSettings.SphereRadius);

	case EPLAttackOverlapShapeType::Capsule:
		return FCollisionShape::MakeCapsule(
			ShapeSettings.CapsuleRadius,
			ShapeSettings.CapsuleHalfHeight
		);

	case EPLAttackOverlapShapeType::Box:
		return FCollisionShape::MakeBox(ShapeSettings.BoxHalfExtent);

	default:
		return FCollisionShape::MakeSphere(35.f);
	}
}

FTransform UPL_CombatComponent::MakeAttackOverlapShapeTransform(
	const FTransform& SocketTransform,
	const FPLAttackOverlapShapeSettings& ShapeSettings)
{
	const FTransform LocalShapeTransform(
		ShapeSettings.LocalRotation,
		ShapeSettings.LocalOffset,
		FVector::OneVector
	);

	return LocalShapeTransform * SocketTransform;
}

void UPL_CombatComponent::ApplyAttackOverlapGameplayEffects(
	const FPLActiveAttackOverlapWindow& Window,
	AActor* HitActor,
	const FHitResult& Hit)
{
	if (!HitActor)
	{
		return;
	}

	if (Window.Settings.GameplayEffectsToApply.IsEmpty())
	{
		return;
	}

	AActor* SourceActor = GetOwner();

	if (!SourceActor || !SourceActor->HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = AbilitySystemComponent;
	UAbilitySystemComponent* TargetASC = GetTargetAbilitySystemComponent(HitActor);

	if (!SourceASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attack overlap could not apply effects: source ASC missing. Source=%s Target=%s"),
			*GetNameSafe(SourceActor),
			*GetNameSafe(HitActor));
		return;
	}

	if (!TargetASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("Attack overlap could not apply effects: target ASC missing. Source=%s Target=%s"),
			*GetNameSafe(SourceActor),
			*GetNameSafe(HitActor));
		return;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	EffectContext.AddInstigator(SourceActor, SourceActor);
	EffectContext.AddHitResult(Hit);

	for (const FPLAttackOverlapGameplayEffect& EffectToApply : Window.Settings.GameplayEffectsToApply)
	{
		if (!EffectToApply.GameplayEffectClass)
		{
			continue;
		}

		const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
			EffectToApply.GameplayEffectClass,
			EffectToApply.EffectLevel,
			EffectContext
		);

		if (!SpecHandle.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Attack overlap failed to make GE spec. Source=%s Target=%s Effect=%s"),
				*GetNameSafe(SourceActor),
				*GetNameSafe(HitActor),
				*GetNameSafe(EffectToApply.GameplayEffectClass));
			continue;
		}

		SourceASC->ApplyGameplayEffectSpecToTarget(
			*SpecHandle.Data.Get(),
			TargetASC
		);

		UE_LOG(LogTemp, Log, TEXT("Attack overlap applied GE. Source=%s Target=%s Effect=%s Level=%.2f"),
			*GetNameSafe(SourceActor),
			*GetNameSafe(HitActor),
			*GetNameSafe(EffectToApply.GameplayEffectClass),
			EffectToApply.EffectLevel);
	}
}

UAbilitySystemComponent* UPL_CombatComponent::GetTargetAbilitySystemComponent(AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return nullptr;
	}

	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
}
