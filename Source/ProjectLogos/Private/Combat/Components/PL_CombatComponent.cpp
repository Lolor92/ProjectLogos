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
#include "GameFramework/Controller.h"
#include "GameplayEffect.h"
#include "Mover/PL_MoverPawnComponent.h"
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

	ClearCrowdControlTagEvent();

	OwnerPawn = InOwnerPawn;
	AbilitySystemComponent = InAbilitySystemComponent;

	BindCrowdControlTagEvent();

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

	ClearCrowdControlTagEvent();

	ClearDefaultAbilities();

	Super::EndPlay(EndPlayReason);
}

void UPL_CombatComponent::NotifyCombatTagReaction(
	const FGameplayTag& TriggerTag,
	const bool bAdded)
{
	BP_OnCombatTagReaction(TriggerTag, bAdded);
}

bool UPL_CombatComponent::IsBlockingActive() const
{
	return AbilitySystemComponent
		&& BlockingTag.IsValid()
		&& AbilitySystemComponent->HasMatchingGameplayTag(BlockingTag);
}

bool UPL_CombatComponent::IsDodgingActive() const
{
	return AbilitySystemComponent
		&& DodgingTag.IsValid()
		&& AbilitySystemComponent->HasMatchingGameplayTag(DodgingTag);
}

bool UPL_CombatComponent::IsParryActive() const
{
	return AbilitySystemComponent
		&& ParryTag.IsValid()
		&& AbilitySystemComponent->HasMatchingGameplayTag(ParryTag);
}

bool UPL_CombatComponent::IsCrowdControlActive() const
{
	return AbilitySystemComponent
		&& CrowdControlTag.IsValid()
		&& AbilitySystemComponent->HasMatchingGameplayTag(CrowdControlTag);
}

EPLAttackOverlapSuperArmorLevel UPL_CombatComponent::GetCurrentSuperArmorLevel() const
{
	if (!AbilitySystemComponent)
	{
		return EPLAttackOverlapSuperArmorLevel::None;
	}

	// Highest level wins.
	if (SuperArmor3Tag.IsValid() && AbilitySystemComponent->HasMatchingGameplayTag(SuperArmor3Tag))
	{
		return EPLAttackOverlapSuperArmorLevel::SuperArmor3;
	}

	if (SuperArmor2Tag.IsValid() && AbilitySystemComponent->HasMatchingGameplayTag(SuperArmor2Tag))
	{
		return EPLAttackOverlapSuperArmorLevel::SuperArmor2;
	}

	if (SuperArmor1Tag.IsValid() && AbilitySystemComponent->HasMatchingGameplayTag(SuperArmor1Tag))
	{
		return EPLAttackOverlapSuperArmorLevel::SuperArmor1;
	}

	return EPLAttackOverlapSuperArmorLevel::None;
}

void UPL_CombatComponent::BindCrowdControlTagEvent()
{
	if (!AbilitySystemComponent || !CrowdControlTag.IsValid() || CrowdControlTagDelegateHandle.IsValid())
	{
		return;
	}

	CrowdControlTagDelegateHandle = AbilitySystemComponent
		->RegisterGameplayTagEvent(CrowdControlTag, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ThisClass::OnCrowdControlTagChanged);

	BoundCrowdControlTag = CrowdControlTag;
}

void UPL_CombatComponent::ClearCrowdControlTagEvent()
{
	if (AbilitySystemComponent && BoundCrowdControlTag.IsValid() && CrowdControlTagDelegateHandle.IsValid())
	{
		AbilitySystemComponent
			->RegisterGameplayTagEvent(BoundCrowdControlTag, EGameplayTagEventType::NewOrRemoved)
			.Remove(CrowdControlTagDelegateHandle);
	}

	CrowdControlTagDelegateHandle.Reset();
	BoundCrowdControlTag = FGameplayTag();
}

void UPL_CombatComponent::OnCrowdControlTagChanged(
	const FGameplayTag Tag,
	const int32 NewCount)
{
	ABasePawn* Pawn = OwnerPawn.Get();
	if (!Pawn)
	{
		return;
	}

	if (NewCount > 0)
	{
		if (UPL_MoverPawnComponent* MoverPawnComponent = Pawn->GetMoverPawnComponent())
		{
			MoverPawnComponent->ClearMoveIntent();
		}
	}

	if (Pawn->IsLocallyControlled())
	{
		if (AController* Controller = Pawn->GetController())
		{
			Controller->SetIgnoreMoveInput(NewCount > 0);
		}
	}
}

bool UPL_CombatComponent::HasSuperArmorAtOrAbove(
	const EPLAttackOverlapSuperArmorLevel RequiredLevel) const
{
	if (RequiredLevel == EPLAttackOverlapSuperArmorLevel::None)
	{
		return false;
	}

	const EPLAttackOverlapSuperArmorLevel CurrentLevel = GetCurrentSuperArmorLevel();

	return static_cast<uint8>(CurrentLevel) >= static_cast<uint8>(RequiredLevel);
}

bool UPL_CombatComponent::IsWithinBlockAngle(
	const AActor* DefenderActor,
	const AActor* AttackerActor,
	const float BlockAngleDegrees
)
{
	if (!DefenderActor || !AttackerActor)
	{
		return false;
	}

	FVector ToAttacker = AttackerActor->GetActorLocation() - DefenderActor->GetActorLocation();
	ToAttacker.Z = 0.f;
	ToAttacker = ToAttacker.GetSafeNormal();

	FVector DefenderForward = DefenderActor->GetActorForwardVector();
	DefenderForward.Z = 0.f;
	DefenderForward = DefenderForward.GetSafeNormal();

	if (ToAttacker.IsNearlyZero() || DefenderForward.IsNearlyZero())
	{
		return false;
	}

	const float Dot = FVector::DotProduct(DefenderForward, ToAttacker);
	const float AngleDegrees = FMath::RadiansToDegrees(
		FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f))
	);

	return AngleDegrees <= BlockAngleDegrees;
}

bool UPL_CombatComponent::IsAttackBlocked(
	AActor* HitActor,
	const FPLAttackOverlapBlockSettings& BlockSettings
) const
{
	if (!BlockSettings.bBlockable || !HitActor)
	{
		return false;
	}

	if (!BlockingTag.IsValid())
	{
		return false;
	}

	const UAbilitySystemComponent* TargetASC = GetTargetAbilitySystemComponent(HitActor);
	if (!TargetASC || !TargetASC->HasMatchingGameplayTag(BlockingTag))
	{
		return false;
	}

	return IsWithinBlockAngle(
		HitActor,
		GetOwner(),
		BlockSettings.BlockAngleDegrees
	);
}

bool UPL_CombatComponent::IsAttackDodged(
	AActor* HitActor,
	const FPLAttackOverlapDodgeSettings& DodgeSettings
) const
{
	if (!DodgeSettings.bDodgeable || !HitActor)
	{
		return false;
	}

	if (!DodgingTag.IsValid())
	{
		return false;
	}

	const UAbilitySystemComponent* TargetASC = GetTargetAbilitySystemComponent(HitActor);
	if (!TargetASC)
	{
		return false;
	}

	return TargetASC->HasMatchingGameplayTag(DodgingTag);
}

bool UPL_CombatComponent::IsAttackParried(
	AActor* HitActor,
	const FPLAttackOverlapParrySettings& ParrySettings
) const
{
	if (!ParrySettings.bParryable || !HitActor)
	{
		return false;
	}

	if (!ParryTag.IsValid())
	{
		return false;
	}

	const UAbilitySystemComponent* TargetASC = GetTargetAbilitySystemComponent(HitActor);
	if (!TargetASC || !TargetASC->HasMatchingGameplayTag(ParryTag))
	{
		return false;
	}

	return IsWithinBlockAngle(
		HitActor,
		GetOwner(),
		ParrySettings.ParryAngleDegrees
	);
}

bool UPL_CombatComponent::HasRequiredSuperArmor(
	AActor* HitActor,
	const EPLAttackOverlapSuperArmorLevel RequiredLevel
) const
{
	if (RequiredLevel == EPLAttackOverlapSuperArmorLevel::None || !HitActor)
	{
		return false;
	}

	UPL_CombatComponent* TargetCombatComponent = nullptr;

	if (ABasePawn* BasePawn = Cast<ABasePawn>(HitActor))
	{
		TargetCombatComponent = BasePawn->GetCombatComponent();
	}
	else
	{
		TargetCombatComponent = HitActor->FindComponentByClass<UPL_CombatComponent>();
	}

	return TargetCombatComponent
		&& TargetCombatComponent->HasSuperArmorAtOrAbove(RequiredLevel);
}

void UPL_CombatComponent::ApplyBlockGameplayEffects(
	AActor* HitActor,
	const FHitResult& Hit
) const
{
	AActor* AttackerActor = GetOwner();
	if (!AttackerActor || !HitActor || !AttackerActor->HasAuthority())
	{
		return;
	}

	ApplyGameplayEffectToActor(
		AttackerActor,
		AttackerBlockedEffectClass,
		1.f,
		&Hit
	);

	ApplyGameplayEffectToActor(
		HitActor,
		DefenderBlockedEffectClass,
		1.f,
		&Hit
	);
}

void UPL_CombatComponent::ApplyDodgeGameplayEffects(
	AActor* HitActor,
	const FHitResult& Hit
) const
{
	AActor* AttackerActor = GetOwner();
	if (!AttackerActor || !HitActor || !AttackerActor->HasAuthority())
	{
		return;
	}

	ApplyGameplayEffectToActor(
		AttackerActor,
		AttackerDodgedEffectClass,
		1.f,
		&Hit
	);

	ApplyGameplayEffectToActor(
		HitActor,
		DefenderDodgedEffectClass,
		1.f,
		&Hit
	);
}

void UPL_CombatComponent::ApplyParryGameplayEffects(
	AActor* HitActor,
	const FHitResult& Hit
) const
{
	AActor* AttackerActor = GetOwner();

	if (!AttackerActor || !HitActor || !AttackerActor->HasAuthority())
	{
		return;
	}

	ApplyGameplayEffectToActor(
		AttackerActor,
		AttackerParriedEffectClass,
		1.f,
		&Hit
	);

	ApplyGameplayEffectToActor(
		HitActor,
		DefenderParrySuccessEffectClass,
		1.f,
		&Hit
	);
}

void UPL_CombatComponent::ApplySuperArmorGameplayEffects(
	AActor* HitActor,
	const FHitResult& Hit
) const
{
	AActor* AttackerActor = GetOwner();

	if (!AttackerActor || !HitActor || !AttackerActor->HasAuthority())
	{
		return;
	}

	ApplyGameplayEffectToActor(
		AttackerActor,
		AttackerSuperArmoredEffectClass,
		1.f,
		&Hit
	);

	ApplyGameplayEffectToActor(
		HitActor,
		DefenderSuperArmoredEffectClass,
		1.f,
		&Hit
	);
}

void UPL_CombatComponent::ApplyGameplayEffectToActor(
	AActor* RecipientActor,
	const TSubclassOf<UGameplayEffect> GameplayEffectClass,
	const float EffectLevel,
	const FHitResult* Hit
) const
{
	if (!RecipientActor || !GameplayEffectClass || !AbilitySystemComponent)
	{
		return;
	}

	UAbilitySystemComponent* RecipientASC = GetTargetAbilitySystemComponent(RecipientActor);
	if (!RecipientASC)
	{
		return;
	}

	AActor* SourceActor = GetOwner();

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(const_cast<UPL_CombatComponent*>(this));

	if (SourceActor)
	{
		EffectContext.AddInstigator(SourceActor, SourceActor);
	}

	if (Hit)
	{
		EffectContext.AddHitResult(*Hit);
	}

	const FGameplayEffectSpecHandle SpecHandle =
		AbilitySystemComponent->MakeOutgoingSpec(
			GameplayEffectClass,
			EffectLevel,
			EffectContext
		);

	if (!SpecHandle.IsValid())
	{
		return;
	}

	AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(
		*SpecHandle.Data.Get(),
		RecipientASC
	);
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

	const bool bWasDodged = IsAttackDodged(
		HitActor,
		Window.Settings.Defense.Dodge
	);

	const bool bWasParried = IsAttackParried(
		HitActor,
		Window.Settings.Defense.Parry
	);

	const bool bWasBlocked = IsAttackBlocked(
		HitActor,
		Window.Settings.Defense.Block
	);

	const bool bHasSuperArmor = HasRequiredSuperArmor(
		HitActor,
		Window.Settings.Defense.RequiredSuperArmor
	);

	if (bWasDodged)
	{
		ApplyDodgeGameplayEffects(HitActor, Hit);

		UE_LOG(
			LogTemp,
			Log,
			TEXT("Attack overlap dodged. Attacker=%s Defender=%s"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(HitActor)
		);

		BP_OnAttackOverlapDetected(HitActor, Hit, Window.MeshComp.Get());
		return;
	}

	if (bWasParried)
	{
		// Parry is also a successful block.
		// So apply block movement/rotation rules first.
		ApplyAttackOverlapTransformEffects(
			Window,
			HitActor,
			Hit,
			true,
			false,
			false,
			false
		);

		// Apply normal block result effects.
		ApplyBlockGameplayEffects(HitActor, Hit);

		// Then apply parry-specific result effects on top.
		ApplyParryGameplayEffects(HitActor, Hit);

		UE_LOG(
			LogTemp,
			Log,
			TEXT("Attack overlap parried. Attacker=%s Defender=%s"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(HitActor)
		);

		BP_OnAttackOverlapDetected(HitActor, Hit, Window.MeshComp.Get());
		return;
	}

	if (bWasBlocked)
	{
		ApplyAttackOverlapTransformEffects(
			Window,
			HitActor,
			Hit,
			true,
			false,
			false,
			false
		);

		ApplyBlockGameplayEffects(HitActor, Hit);

		UE_LOG(
			LogTemp,
			Log,
			TEXT("Attack overlap blocked. Attacker=%s Defender=%s"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(HitActor)
		);

		BP_OnAttackOverlapDetected(HitActor, Hit, Window.MeshComp.Get());
		return;
	}

	if (bHasSuperArmor)
	{
		// Super armor still takes damage/status effects.
		ApplyAttackOverlapDamageGameplayEffects(Window, HitActor, Hit);

		// But it skips stagger/knockback/reaction effects.
		ApplySuperArmorGameplayEffects(HitActor, Hit);

		UE_LOG(
			LogTemp,
			Log,
			TEXT("Attack overlap super armored. Attacker=%s Defender=%s Required=%d"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(HitActor),
			static_cast<int32>(Window.Settings.Defense.RequiredSuperArmor)
		);

		BP_OnAttackOverlapDetected(HitActor, Hit, Window.MeshComp.Get());
		return;
	}

	// Clean hit.
	ApplyAttackOverlapTransformEffects(
		Window,
		HitActor,
		Hit,
		false,
		false,
		false,
		false
	);

	ApplyAttackOverlapHitStop(Window, HitActor);
	ApplyAttackOverlapDamageGameplayEffects(Window, HitActor, Hit);
	ApplyAttackOverlapReactionGameplayEffects(Window, HitActor, Hit);

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

void UPL_CombatComponent::ApplyAttackOverlapTransformEffects(
	const FPLActiveAttackOverlapWindow& Window,
	AActor* HitActor,
	const FHitResult& Hit,
	const bool bWasBlocked,
	const bool bWasDodged,
	const bool bWasParried,
	const bool bHasSuperArmor)
{
	AActor* InstigatorActor = GetOwner();
	if (!InstigatorActor || !HitActor)
	{
		return;
	}

	// Target movement/rotation must be server authoritative.
	if (!InstigatorActor->HasAuthority())
	{
		return;
	}

	ApplyAttackOverlapRotation(
		Window.Settings.Rotation,
		Window.Settings.Defense.Block,
		InstigatorActor,
		HitActor,
		bWasBlocked,
		bWasDodged,
		bWasParried,
		bHasSuperArmor
	);
	ApplyAttackOverlapMovement(
		Window.Settings.Movement,
		Window.Settings.Defense.Block,
		InstigatorActor,
		HitActor,
		bWasBlocked,
		bWasDodged,
		bWasParried,
		bHasSuperArmor
	);
}

void UPL_CombatComponent::ApplyAttackOverlapHitStop(
	FPLActiveAttackOverlapWindow& Window,
	AActor* HitActor
)
{
	if (!HitActor)
	{
		return;
	}

	AActor* InstigatorActor = GetOwner();
	if (!InstigatorActor || !InstigatorActor->HasAuthority())
	{
		return;
	}

	const FPLAttackOverlapHitStopSettings& HitStopSettings = Window.Settings.HitStop;

	if (!HitStopSettings.IsEnabled())
	{
		return;
	}

	switch (HitStopSettings.ApplicationMode)
	{
	case EPLAttackOverlapHitStopApplicationMode::OncePerNotify:
	{
		if (Window.bHitStopAppliedThisWindow)
		{
			return;
		}

		Window.bHitStopAppliedThisWindow = true;
		break;
	}

	case EPLAttackOverlapHitStopApplicationMode::OncePerHitActor:
	{
		const TWeakObjectPtr<AActor> HitActorKey(HitActor);

		if (Window.HitStopAppliedActors.Contains(HitActorKey))
		{
			return;
		}

		Window.HitStopAppliedActors.Add(HitActorKey);
		break;
	}

	default:
		break;
	}

	switch (HitStopSettings.Recipient)
	{
	case EPLAttackOverlapHitStopRecipient::Instigator:
		ApplyHitStopToActor(InstigatorActor, HitStopSettings);
		break;

	case EPLAttackOverlapHitStopRecipient::Target:
		ApplyHitStopToActor(HitActor, HitStopSettings);
		break;

	case EPLAttackOverlapHitStopRecipient::Both:
		ApplyHitStopToActor(InstigatorActor, HitStopSettings);
		ApplyHitStopToActor(HitActor, HitStopSettings);
		break;

	default:
		break;
	}
}

void UPL_CombatComponent::ApplyHitStopToActor(
	AActor* Actor,
	const FPLAttackOverlapHitStopSettings& HitStopSettings
) const
{
	if (!Actor || !Actor->HasAuthority())
	{
		return;
	}

	if (ABasePawn* BasePawn = Cast<ABasePawn>(Actor))
	{
		if (UPL_MoverPawnComponent* MoverPawnComponent = BasePawn->GetMoverPawnComponent())
		{
			MoverPawnComponent->ApplyAuthoritativeHitStop(
				HitStopSettings.Duration,
				HitStopSettings.TimeScale,
				HitStopSettings.bAffectAnimation,
				HitStopSettings.bAffectMoverRootMotion
			);
		}
	}
}

void UPL_CombatComponent::ApplyAttackOverlapMovement(
	const FPLAttackOverlapMovementSettings& MovementSettings,
	const FPLAttackOverlapBlockSettings& BlockSettings,
	AActor* InstigatorActor,
	AActor* TargetActor,
	const bool bWasBlocked,
	const bool bWasDodged,
	const bool bWasParried,
	const bool bHasSuperArmor
)
{
	if (MovementSettings.MoveDirection == EPLAttackOverlapMoveDirection::None)
	{
		return;
	}

	if (bWasDodged || bWasParried || bHasSuperArmor)
	{
		return;
	}

	if (bWasBlocked && !BlockSettings.bAllowMovementWhenBlocked)
	{
		return;
	}

	AActor* RecipientActor = ResolveTransformRecipient(
		MovementSettings.Recipient,
		InstigatorActor,
		TargetActor
	);

	AActor* ReferenceActor = ResolveReferenceActor(
		MovementSettings.ReferenceActorSource,
		InstigatorActor,
		TargetActor
	);

	if (!RecipientActor || !ReferenceActor)
	{
		return;
	}

	const FVector RecipientLocation = RecipientActor->GetActorLocation();
	const FVector ReferenceLocation = ReferenceActor->GetActorLocation();

	FVector Direction = RecipientLocation - ReferenceLocation;
	Direction.Z = 0.f;

	if (Direction.IsNearlyZero())
	{
		Direction = ReferenceActor->GetActorForwardVector();
		Direction.Z = 0.f;
	}

	Direction = Direction.GetSafeNormal();

	FVector NewLocation = RecipientLocation;

	switch (MovementSettings.MoveDirection)
	{
	case EPLAttackOverlapMoveDirection::MoveAway:
		NewLocation = RecipientLocation + Direction * MovementSettings.MoveDistance;
		break;

	case EPLAttackOverlapMoveDirection::MoveCloser:
		NewLocation = RecipientLocation - Direction * MovementSettings.MoveDistance;
		break;

	case EPLAttackOverlapMoveDirection::SnapToDistance:
		NewLocation = ReferenceLocation + Direction * MovementSettings.MoveDistance;
		break;

	case EPLAttackOverlapMoveDirection::KeepCurrentDistance:
		NewLocation = RecipientLocation;
		break;

	default:
		return;
	}

	if (MovementSettings.LateralOffsetMode != EPLAttackOverlapLateralOffsetMode::KeepCurrent)
	{
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Direction).GetSafeNormal();

		if (MovementSettings.LateralOffsetMode == EPLAttackOverlapLateralOffsetMode::AddOffset)
		{
			NewLocation += Right * MovementSettings.LateralOffset;
		}
		else if (MovementSettings.LateralOffsetMode == EPLAttackOverlapLateralOffsetMode::SnapToOffset)
		{
			NewLocation = ReferenceLocation
				+ Direction * MovementSettings.MoveDistance
				+ Right * MovementSettings.LateralOffset;
		}
	}

	ApplyMoverAwareTransformToActor(
		RecipientActor,
		NewLocation,
		RecipientActor->GetActorRotation(),
		true,
		false,
		MovementSettings.bSweep,
		ToTeleportType(MovementSettings.TeleportType)
	);
}

void UPL_CombatComponent::ApplyAttackOverlapRotation(
	const FPLAttackOverlapRotationSettings& RotationSettings,
	const FPLAttackOverlapBlockSettings& BlockSettings,
	AActor* InstigatorActor,
	AActor* TargetActor,
	const bool bWasBlocked,
	const bool bWasDodged,
	const bool bWasParried,
	const bool bHasSuperArmor
)
{
	if (RotationSettings.RotationDirection == EPLAttackOverlapRotationDirection::None)
	{
		return;
	}

	if (bWasDodged || bWasParried || bHasSuperArmor)
	{
		return;
	}

	if (bWasBlocked && !BlockSettings.bAllowRotationWhenBlocked)
	{
		return;
	}

	AActor* RecipientActor = ResolveTransformRecipient(
		RotationSettings.Recipient,
		InstigatorActor,
		TargetActor
	);

	AActor* ReferenceActor = ResolveReferenceActor(
		RotationSettings.ReferenceActorSource,
		InstigatorActor,
		TargetActor
	);

	if (!RecipientActor || !ReferenceActor)
	{
		return;
	}

	FRotator DesiredRotation = RecipientActor->GetActorRotation();

	switch (RotationSettings.RotationDirection)
	{
	case EPLAttackOverlapRotationDirection::FaceReferenceActor:
		{
			const FVector Direction =
				ReferenceActor->GetActorLocation() - RecipientActor->GetActorLocation();

			if (!Direction.IsNearlyZero())
			{
				DesiredRotation = Direction.ToOrientationRotator();
			}
			break;
		}

	case EPLAttackOverlapRotationDirection::FaceAwayFromReference:
		{
			const FVector Direction =
				RecipientActor->GetActorLocation() - ReferenceActor->GetActorLocation();

			if (!Direction.IsNearlyZero())
			{
				DesiredRotation = Direction.ToOrientationRotator();
			}
			break;
		}

	case EPLAttackOverlapRotationDirection::FaceOppositeReferenceForward:
		{
			const FVector Direction = -ReferenceActor->GetActorForwardVector();

			if (!Direction.IsNearlyZero())
			{
				DesiredRotation = Direction.ToOrientationRotator();
			}
			break;
		}

	case EPLAttackOverlapRotationDirection::FaceDirection:
		{
			DesiredRotation = RotationSettings.DirectionToFace;
			break;
		}

	default:
		return;
	}

	DesiredRotation.Pitch = 0.f;
	DesiredRotation.Roll = 0.f;

	ApplyMoverAwareTransformToActor(
		RecipientActor,
		RecipientActor->GetActorLocation(),
		DesiredRotation,
		false,
		true,
		false,
		ToTeleportType(RotationSettings.TeleportType)
	);
}

void UPL_CombatComponent::ApplyAttackOverlapDamageGameplayEffects(
	const FPLActiveAttackOverlapWindow& Window,
	AActor* HitActor,
	const FHitResult& Hit
) const
{
	if (!HitActor || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	for (const FPLAttackOverlapGameplayEffect& EffectEntry : Window.Settings.DamageGameplayEffectsToApply)
	{
		ApplyGameplayEffectToActor(
			HitActor,
			EffectEntry.GameplayEffectClass,
			EffectEntry.EffectLevel,
			&Hit
		);
	}
}

void UPL_CombatComponent::ApplyAttackOverlapReactionGameplayEffects(
	const FPLActiveAttackOverlapWindow& Window,
	AActor* HitActor,
	const FHitResult& Hit
) const
{
	if (!HitActor || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	// This uses the old variable name intentionally.
	// Existing notify data stays valid and is now considered "reaction effects."
	for (const FPLAttackOverlapGameplayEffect& EffectEntry : Window.Settings.GameplayEffectsToApply)
	{
		ApplyGameplayEffectToActor(
			HitActor,
			EffectEntry.GameplayEffectClass,
			EffectEntry.EffectLevel,
			&Hit
		);
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

AActor* UPL_CombatComponent::ResolveTransformRecipient(
	const EPLAttackOverlapTransformRecipient Recipient,
	AActor* InstigatorActor,
	AActor* TargetActor
) const
{
	switch (Recipient)
	{
	case EPLAttackOverlapTransformRecipient::Instigator:
		return InstigatorActor;

	case EPLAttackOverlapTransformRecipient::Target:
		return TargetActor;

	default:
		return nullptr;
	}
}

AActor* UPL_CombatComponent::ResolveReferenceActor(
	const EPLAttackOverlapReferenceActorSource Source,
	AActor* InstigatorActor,
	AActor* TargetActor
) const
{
	switch (Source)
	{
	case EPLAttackOverlapReferenceActorSource::Instigator:
		return InstigatorActor;

	case EPLAttackOverlapReferenceActorSource::Target:
		return TargetActor;

	default:
		return nullptr;
	}
}

void UPL_CombatComponent::ApplyMoverAwareTransformToActor(
	AActor* Actor,
	const FVector& NewLocation,
	const FRotator& NewRotation,
	const bool bApplyLocation,
	const bool bApplyRotation,
	const bool bSweep,
	const ETeleportType TeleportType
) const
{
	if (!Actor || !Actor->HasAuthority())
	{
		return;
	}

	if (ABasePawn* BasePawn = Cast<ABasePawn>(Actor))
	{
		if (UPL_MoverPawnComponent* MoverPawnComponent = BasePawn->GetMoverPawnComponent())
		{
			MoverPawnComponent->ApplyAuthoritativeExternalTransformSnap(
				NewLocation,
				NewRotation,
				bApplyLocation,
				bApplyRotation,
				bSweep,
				TeleportType
			);
			return;
		}
	}

	if (bApplyLocation && bApplyRotation)
	{
		Actor->SetActorLocationAndRotation(NewLocation, NewRotation, bSweep, nullptr, TeleportType);
	}
	else if (bApplyLocation)
	{
		Actor->SetActorLocation(NewLocation, bSweep, nullptr, TeleportType);
	}
	else if (bApplyRotation)
	{
		Actor->SetActorRotation(NewRotation, TeleportType);
	}

	Actor->ForceNetUpdate();
}
