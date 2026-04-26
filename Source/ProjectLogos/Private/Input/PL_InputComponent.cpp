#include "Input/PL_InputComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameplayAbilitySpec.h"
#include "GAS/Abilities/PL_GameplayAbility.h"
#include "Input/PL_InputConfig.h"
#include "Input/Tag/PL_InputTags.h"
#include "InputActionValue.h"
#include "Mover/PL_MoverPawnComponent.h"
#include "Pawn/BasePawn.h"

UPL_InputComponent::UPL_InputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	MoveInputTag = TAG_Input_Move;
	LookInputTag = TAG_Input_Look;
	ZoomInputTag = TAG_Input_Zoom;
}

void UPL_InputComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializePlayerInput();
}

void UPL_InputComponent::InitializePlayerInput()
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	if (InjectedEnhancedInputComponent)
	{
		return;
	}

	InstallInput();
}

void UPL_InputComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UninstallInput();

	Super::EndPlay(EndPlayReason);
}

void UPL_InputComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	USpringArmComponent* SpringArm = FindSpringArm();
	if (!SpringArm)
	{
		SetComponentTickEnabled(false);
		return;
	}

	SpringArm->TargetArmLength = FMath::FInterpTo(
		SpringArm->TargetArmLength,
		DesiredZoomArmLength,
		DeltaTime,
		ZoomInterpSpeed
	);

	if (FMath::IsNearlyEqual(SpringArm->TargetArmLength, DesiredZoomArmLength, 1.f))
	{
		SpringArm->TargetArmLength = DesiredZoomArmLength;
		SetComponentTickEnabled(false);
	}
}

void UPL_InputComponent::InstallInput()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController()) return;

	CachedPlayerController = PlayerController;
	CachedSpringArm = FindSpringArm();
	CachedCamera = FindCamera();

	if (USpringArmComponent* SpringArm = CachedSpringArm.Get())
	{
		DesiredZoomArmLength = SpringArm->TargetArmLength;
	}
	else
	{
		DesiredZoomArmLength = ZoomLevel * ZoomStepDistance;
	}

	AddMappingContexts();

	if (!InjectedEnhancedInputComponent)
	{
		// We inject our own Enhanced Input component into the controller stack.
		InjectedEnhancedInputComponent = NewObject<UEnhancedInputComponent>(
			PlayerController,
			UEnhancedInputComponent::StaticClass(),
			TEXT("PL_InjectedInputComponent")
		);

		InjectedEnhancedInputComponent->RegisterComponent();
		PlayerController->PushInputComponent(InjectedEnhancedInputComponent);
	}

	BindInputActions();
}

void UPL_InputComponent::UninstallInput()
{
	RemoveMappingContexts();
	ClearAllComboChains();

	if (APlayerController* PlayerController = CachedPlayerController.Get())
	{
		if (InjectedEnhancedInputComponent)
		{
			PlayerController->PopInputComponent(InjectedEnhancedInputComponent);
			InjectedEnhancedInputComponent->DestroyComponent();
			InjectedEnhancedInputComponent = nullptr;
		}
	}

	if (ABasePawn* BasePawn = Cast<ABasePawn>(GetOwner()))
	{
		if (UPL_MoverPawnComponent* MoverPawnComponent = BasePawn->GetMoverPawnComponent())
		{
			MoverPawnComponent->ClearMoveIntent();
		}
	}

	CachedAbilitySystemComponent = nullptr;
	CachedSpringArm = nullptr;
	CachedCamera = nullptr;
	DesiredZoomArmLength = 0.f;
	SetComponentTickEnabled(false);

	CachedPlayerController = nullptr;
}

void UPL_InputComponent::AddMappingContexts() const
{
	if (!InputConfig) return;

	const APlayerController* PlayerController = CachedPlayerController.Get();
	if (!PlayerController) return;

	const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer) return;

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

	if (!InputSubsystem) return;

	for (const FPLInputMappingContextEntry& Entry : InputConfig->MappingContexts)
	{
		if (!Entry.InputMappingContext) continue;

		InputSubsystem->AddMappingContext(Entry.InputMappingContext, Entry.Priority);
	}
}

void UPL_InputComponent::RemoveMappingContexts() const
{
	if (!InputConfig) return;

	const APlayerController* PlayerController = CachedPlayerController.Get();
	if (!PlayerController) return;

	const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer) return;

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

	if (!InputSubsystem) return;

	for (const FPLInputMappingContextEntry& Entry : InputConfig->MappingContexts)
	{
		if (!Entry.InputMappingContext) continue;

		InputSubsystem->RemoveMappingContext(Entry.InputMappingContext);
	}
}

void UPL_InputComponent::BindInputActions()
{
	if (!InjectedEnhancedInputComponent || !InputConfig) return;

	for (const FPLInputActionEntry& Entry : InputConfig->InputActions)
	{
		if (!Entry.InputAction || !Entry.InputTag.IsValid()) continue;

		if (Entry.InputTag.MatchesTagExact(MoveInputTag))
		{
			InjectedEnhancedInputComponent->BindAction(
				Entry.InputAction,
				ETriggerEvent::Triggered,
				this,
				&ThisClass::Move
			);

			InjectedEnhancedInputComponent->BindAction(
				Entry.InputAction,
				ETriggerEvent::Completed,
				this,
				&ThisClass::StopMove
			);

			InjectedEnhancedInputComponent->BindAction(
				Entry.InputAction,
				ETriggerEvent::Canceled,
				this,
				&ThisClass::StopMove
			);

			continue;
		}

		if (Entry.InputTag.MatchesTagExact(LookInputTag))
		{
			InjectedEnhancedInputComponent->BindAction(
				Entry.InputAction,
				ETriggerEvent::Triggered,
				this,
				&ThisClass::Look
			);

			continue;
		}

		if (Entry.InputTag.MatchesTagExact(ZoomInputTag))
		{
			InjectedEnhancedInputComponent->BindAction(
				Entry.InputAction,
				ETriggerEvent::Triggered,
				this,
				&ThisClass::Zoom
			);

			continue;
		}

		// Any other tagged input is treated as GAS ability input.
		InjectedEnhancedInputComponent->BindAction(
			Entry.InputAction,
			ETriggerEvent::Started,
			this,
			&ThisClass::HandleAbilityInputPressed,
			Entry.InputTag
		);

		InjectedEnhancedInputComponent->BindAction(
			Entry.InputAction,
			ETriggerEvent::Completed,
			this,
			&ThisClass::HandleAbilityInputReleased,
			Entry.InputTag
		);

		InjectedEnhancedInputComponent->BindAction(
			Entry.InputAction,
			ETriggerEvent::Canceled,
			this,
			&ThisClass::HandleAbilityInputReleased,
			Entry.InputTag
		);
	}
}

void UPL_InputComponent::Move(const FInputActionValue& Value)
{
	const FVector2D MoveInput = Value.Get<FVector2D>();

	ABasePawn* BasePawn = Cast<ABasePawn>(GetOwner());
	if (!BasePawn) return;

	UPL_MoverPawnComponent* MoverPawnComponent = BasePawn->GetMoverPawnComponent();
	if (!MoverPawnComponent) return;

	if (MoveInput.IsNearlyZero())
	{
		MoverPawnComponent->ClearMoveIntent();
		return;
	}

	// Current IMC convention: X = forward/back, Y = right/left.
	const FVector MoveIntent(MoveInput.X, MoveInput.Y, 0.f);
	MoverPawnComponent->RequestMoveIntent(MoveIntent);
}

void UPL_InputComponent::StopMove(const FInputActionValue& Value)
{
	ABasePawn* BasePawn = Cast<ABasePawn>(GetOwner());
	if (!BasePawn) return;

	UPL_MoverPawnComponent* MoverPawnComponent = BasePawn->GetMoverPawnComponent();
	if (!MoverPawnComponent) return;

	MoverPawnComponent->ClearMoveIntent();
}

void UPL_InputComponent::Look(const FInputActionValue& Value)
{
	const FVector2D LookInput = Value.Get<FVector2D>();
	if (LookInput.IsNearlyZero()) return;

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController) return;

	PlayerController->AddYawInput(LookInput.X);
	PlayerController->AddPitchInput(LookInput.Y);
}

void UPL_InputComponent::Zoom(const FInputActionValue& Value)
{
	if (!bEnableZoom || MaxZoomLevel < MinZoomLevel)
	{
		return;
	}

	const float InputAxisValue = Value.Get<float>();
	if (FMath::IsNearlyZero(InputAxisValue))
	{
		return;
	}

	if (InputAxisValue > 0.f && ZoomLevel > MinZoomLevel)
	{
		--ZoomLevel;
		ApplyZoom();
	}
	else if (InputAxisValue < 0.f && ZoomLevel < MaxZoomLevel)
	{
		++ZoomLevel;
		ApplyZoom();
	}
}

void UPL_InputComponent::ApplyZoom()
{
	USpringArmComponent* SpringArm = FindSpringArm();
	if (!SpringArm)
	{
		return;
	}

	DesiredZoomArmLength = ZoomLevel * ZoomStepDistance;
	SetComponentTickEnabled(true);

	if (UCameraComponent* Camera = FindCamera())
	{
		const FVector CameraOffset =
			ZoomLevel == MinZoomLevel
				? ClosestZoomCameraOffset
				: DefaultCameraOffset;

		Camera->SetRelativeLocation(CameraOffset);
	}
}

void UPL_InputComponent::HandleAbilityInputPressed(FGameplayTag InputTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !InputTag.IsValid()) return;

	for (FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
	{
		const bool bMatchesInputTag =
			AbilitySpec.GetDynamicSpecSourceTags().HasTag(InputTag) ||
			(AbilitySpec.Ability && AbilitySpec.Ability->GetAssetTags().HasTag(InputTag));

		if (!bMatchesInputTag) continue;

		// If this starter ability has an active combo chain, trigger the next ability.
		if (TryActivateComboAbility(AbilitySpec)) return;

		// Otherwise activate the starter ability normally.
		const bool bActivated = ASC->TryActivateAbility(AbilitySpec.Handle);

		if (bActivated)
		{
			SnapOwnerFacingToControllerYawForAbility(AbilitySpec);
			UpdateComboChain(AbilitySpec.Handle, AbilitySpec);
		}

		ASC->AbilitySpecInputPressed(AbilitySpec);

		FPredictionKey PredictionKey;

		if (UGameplayAbility* PrimaryInstance = AbilitySpec.GetPrimaryInstance())
		{
			PredictionKey = PrimaryInstance->GetCurrentActivationInfo().GetActivationPredictionKey();
		}

		// Keep server-side ability input state in sync.
		ASC->InvokeReplicatedEvent(
			EAbilityGenericReplicatedEvent::InputPressed,
			AbilitySpec.Handle,
			PredictionKey
		);
	}
}

void UPL_InputComponent::SnapOwnerFacingToControllerYawForAbility(
	const FGameplayAbilitySpec& AbilitySpec) const
{
	const UPL_GameplayAbility* PLAbility = Cast<UPL_GameplayAbility>(AbilitySpec.GetPrimaryInstance());

	if (!PLAbility)
	{
		PLAbility = Cast<UPL_GameplayAbility>(AbilitySpec.Ability);
	}

	if (!PLAbility || !PLAbility->ShouldRotateToControllerYawOnActivate())
	{
		return;
	}

	ABasePawn* BasePawn = Cast<ABasePawn>(GetOwner());
	if (!BasePawn || !BasePawn->IsLocallyControlled())
	{
		return;
	}

	const APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController)
	{
		return;
	}

	const float DesiredYaw = PlayerController->GetControlRotation().Yaw;

	FRotator NewRotation = BasePawn->GetActorRotation();
	NewRotation.Yaw = DesiredYaw;
	BasePawn->SetActorRotation(NewRotation, ETeleportType::ResetPhysics);

	if (UPL_MoverPawnComponent* MoverPawnComponent = BasePawn->GetMoverPawnComponent())
	{
		MoverPawnComponent->RequestForcedFacingYaw(DesiredYaw);
	}
}

void UPL_InputComponent::HandleAbilityInputReleased(FGameplayTag InputTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;

	for (FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
	{
		const bool bMatchesInputTag =
			AbilitySpec.GetDynamicSpecSourceTags().HasTag(InputTag) ||
			(AbilitySpec.Ability && AbilitySpec.Ability->GetAssetTags().HasTag(InputTag));

		if (!bMatchesInputTag || !AbilitySpec.IsActive()) continue;

		ASC->AbilitySpecInputReleased(AbilitySpec);

		FPredictionKey PredictionKey;

		if (UGameplayAbility* PrimaryInstance = AbilitySpec.GetPrimaryInstance())
		{
			PredictionKey = PrimaryInstance->GetCurrentActivationInfo().GetActivationPredictionKey();
		}

		ASC->InvokeReplicatedEvent(
			EAbilityGenericReplicatedEvent::InputReleased,
			AbilitySpec.Handle,
			PredictionKey
		);
	}
}

bool UPL_InputComponent::TryActivateComboAbility(const FGameplayAbilitySpec& RequestedAbilitySpec)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return false;

	// The pressed starter ability is the key for this chain.
	FPLActiveComboChain* ComboChain = ActiveComboChains.Find(RequestedAbilitySpec.Handle);

	if (!ComboChain || !ComboChain->NextAbilityClass)
	{
		return false;
	}

	for (FGameplayAbilitySpec& ComboSpec : ASC->GetActivatableAbilities())
	{
		if (!ComboSpec.Ability) continue;

		const bool bIsNextComboAbility =
			ComboSpec.Ability->GetClass()->IsChildOf(ComboChain->NextAbilityClass);

		if (!bIsNextComboAbility) continue;

		const bool bActivated = ASC->TryActivateAbility(ComboSpec.Handle);

		if (bActivated)
		{
			SnapOwnerFacingToControllerYawForAbility(ComboSpec);
			// Keep the original starter handle, but advance the combo target.
			UpdateComboChain(RequestedAbilitySpec.Handle, ComboSpec);
		}

		// Consume the input even if cooldown/cost blocks the combo target.
		return true;
	}

	ClearComboChain(RequestedAbilitySpec.Handle);
	return true;
}

void UPL_InputComponent::UpdateComboChain(
	FGameplayAbilitySpecHandle StarterHandle,
	const FGameplayAbilitySpec& CurrentAbilitySpec
)
{
	// Prefer the live instance, then fall back to the class default object.
	UPL_GameplayAbility* CurrentAbility = Cast<UPL_GameplayAbility>(CurrentAbilitySpec.GetPrimaryInstance());

	if (!CurrentAbility)
	{
		CurrentAbility = Cast<UPL_GameplayAbility>(CurrentAbilitySpec.Ability);
	}

	if (!CurrentAbility ||
		!CurrentAbility->GetComboAbilityClass() ||
		CurrentAbility->GetComboWindowDuration() <= 0.f)
	{
		ClearComboChain(StarterHandle);
		return;
	}

	FPLActiveComboChain& ComboChain = ActiveComboChains.FindOrAdd(StarterHandle);
	ComboChain.CurrentAbilityHandle = CurrentAbilitySpec.Handle;
	ComboChain.NextAbilityClass = CurrentAbility->GetComboAbilityClass();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ComboChain.TimerHandle);

		FTimerDelegate TimerDelegate =
			FTimerDelegate::CreateUObject(this, &ThisClass::ClearComboChain, StarterHandle);

		World->GetTimerManager().SetTimer(
			ComboChain.TimerHandle,
			TimerDelegate,
			CurrentAbility->GetComboWindowDuration(),
			false
		);
	}
}

void UPL_InputComponent::ClearComboChain(FGameplayAbilitySpecHandle StarterHandle)
{
	if (FPLActiveComboChain* ComboChain = ActiveComboChains.Find(StarterHandle))
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ComboChain->TimerHandle);
		}
	}

	ActiveComboChains.Remove(StarterHandle);
}

void UPL_InputComponent::ClearAllComboChains()
{
	UWorld* World = GetWorld();

	for (TPair<FGameplayAbilitySpecHandle, FPLActiveComboChain>& ComboChainPair : ActiveComboChains)
	{
		if (World)
		{
			World->GetTimerManager().ClearTimer(ComboChainPair.Value.TimerHandle);
		}
	}

	ActiveComboChains.Reset();
}

bool UPL_InputComponent::IsLocallyControlledOwner() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return false;

	const APlayerController* PlayerController = Cast<APlayerController>(OwnerPawn->GetController());
	return PlayerController && PlayerController->IsLocalController();
}

APlayerController* UPL_InputComponent::GetOwningPlayerController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
}

UAbilitySystemComponent* UPL_InputComponent::GetAbilitySystemComponent() const
{
	if (CachedAbilitySystemComponent)
	{
		return CachedAbilitySystemComponent;
	}

	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(GetOwner());
	if (!AbilitySystemInterface) return nullptr;

	UAbilitySystemComponent* ASC = AbilitySystemInterface->GetAbilitySystemComponent();
	const_cast<UPL_InputComponent*>(this)->CachedAbilitySystemComponent = ASC;

	return ASC;
}

USpringArmComponent* UPL_InputComponent::FindSpringArm() const
{
	if (CachedSpringArm)
	{
		return CachedSpringArm;
	}

	if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		return OwnerPawn->FindComponentByClass<USpringArmComponent>();
	}

	return nullptr;
}

UCameraComponent* UPL_InputComponent::FindCamera() const
{
	if (CachedCamera)
	{
		return CachedCamera;
	}

	if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		return OwnerPawn->FindComponentByClass<UCameraComponent>();
	}

	return nullptr;
}
