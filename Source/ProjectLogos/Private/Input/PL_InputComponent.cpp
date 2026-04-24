#include "Input/PL_InputComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayAbilitySpec.h"
#include "Input/PL_InputConfig.h"
#include "InputActionValue.h"
#include "Mover/PL_MoverPawnComponent.h"
#include "Pawn/BasePawn.h"

UPL_InputComponent::UPL_InputComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// These tags must exist in Project Settings -> Gameplay Tags.
	MoveInputTag = FGameplayTag::RequestGameplayTag(TEXT("Input.Move"));
	LookInputTag = FGameplayTag::RequestGameplayTag(TEXT("Input.Look"));
}

void UPL_InputComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocallyControlledOwner()) return;

	InstallInput();
}

void UPL_InputComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UninstallInput();

	Super::EndPlay(EndPlayReason);
}

void UPL_InputComponent::InstallInput()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController()) return;

	CachedPlayerController = PlayerController;

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

void UPL_InputComponent::HandleAbilityInputPressed(FGameplayTag InputTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;

	for (FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
	{
		const bool bMatchesInputTag =
			AbilitySpec.GetDynamicSpecSourceTags().HasTag(InputTag) ||
			(AbilitySpec.Ability && AbilitySpec.Ability->GetAssetTags().HasTag(InputTag));

		if (!bMatchesInputTag) continue;

		ASC->AbilitySpecInputPressed(AbilitySpec);

		const bool bActivated = ASC->TryActivateAbility(AbilitySpec.Handle);

		if (bActivated)
		{
			FPredictionKey PredictionKey;

			if (UGameplayAbility* PrimaryInstance = AbilitySpec.GetPrimaryInstance())
			{
				PredictionKey = PrimaryInstance->GetCurrentActivationInfo().GetActivationPredictionKey();
			}

			ASC->InvokeReplicatedEvent(
				EAbilityGenericReplicatedEvent::InputPressed,
				AbilitySpec.Handle,
				PredictionKey
			);
		}
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
