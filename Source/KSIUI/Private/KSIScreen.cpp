#include "KSIScreen.h"

#include "KSIUIContext.h"
#include "KSIUISubsystem.h"
#include "Engine/LocalPlayer.h"
#include "CommonInputSettings.h"
#include "ICommonInputModule.h"
#include "Input/CommonUIInputTypes.h"
#include "Input/UIActionBindingHandle.h"

UKSIScreen::UKSIScreen(const FObjectInitializer& Initializer) : Super(Initializer)
{
	bAutoRestoreFocus = true;
	bIsBackHandler = false;
	SetIsFocusable(true);
}

TOptional<FUIInputConfig> UKSIScreen::GetDesiredInputConfig() const
{
	if (InputPolicy == EKSIUIInputPolicy::Inherit)
	{
		return TOptional<FUIInputConfig>();
	}

	const auto* Local = GetOwningLocalPlayer();
	const auto* UI = Local ? Local->GetSubsystem<UKSIUISubsystem>() : nullptr;
	const bool bBlockGameplay = UI && UI->IsGameplayInputBlocked();
	if (InputPolicy == EKSIUIInputPolicy::Game && !bBlockGameplay)
	{
		return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
	}

	const bool bMenuInput = InputPolicy == EKSIUIInputPolicy::Menu || bBlockGameplay;
	FUIInputConfig Config(bMenuInput ? ECommonInputMode::Menu : ECommonInputMode::All,
		EMouseCaptureMode::NoCapture, EMouseLockMode::DoNotLock, false);
	Config.bIgnoreMoveInput = bMenuInput;
	Config.bIgnoreLookInput = bMenuInput;
	return Config;
}

void UKSIScreen::NativeConstruct()
{
	// Context can be assigned after a pool has constructed the Slate widget.
	// Own this binding so a passive HUD never consumes a menu's Back action.
	bIsBackHandler = false;
	Super::NativeConstruct();
	RefreshBackBinding();
}

void UKSIScreen::NativeDestruct()
{
	Super::NativeDestruct();
	ReleaseContext();
}

UWidget* UKSIScreen::NativeGetDesiredFocusTarget() const
{
	if (UWidget* Target = Super::NativeGetDesiredFocusTarget())
	{
		return Target;
	}

	return const_cast<UKSIScreen*>(this);
}

bool UKSIScreen::NativeOnHandleBackAction()
{
	RequestClose();
	return true;
}

void UKSIScreen::BindContext(UKSIUIContext* Context, FKSIUIViewHandle Handle, EKSIUIInputPolicy Policy)
{
	ReleaseContext();
	OriginalVisibility = GetVisibility();
	OriginalOpacity = GetRenderOpacity();
	bOriginalEnabled = GetIsEnabled();
	ViewContext = Context;
	ViewHandle = Handle;
	InputPolicy = Policy;
	bSupportsActivationFocus = Policy != EKSIUIInputPolicy::Inherit;
	RefreshBackBinding();
	OnContextBound();
}

void UKSIScreen::ReleaseContext()
{
	BackBinding.Unregister();
	if (ViewHandle.IsValid())
	{
		OnContextReleased();
		SetVisibility(OriginalVisibility);
		SetRenderOpacity(OriginalOpacity);
		SetIsEnabled(bOriginalEnabled != 0);
	}

	ViewContext = nullptr;
	ViewHandle = {};
	InputPolicy = EKSIUIInputPolicy::Inherit;
	OnCloseRequested.Clear();
	ClearFocusRestorationTarget();
	Presentation = {};
	CurrentOpacity = StartOpacity = TargetOpacity = 1.f;
	FadeDuration = FadeElapsed = 0.f;
	bPresentationInteractive = 1;
}

void UKSIScreen::RequestClose()
{
	OnCloseRequested.Broadcast(ViewHandle);
}

void UKSIScreen::OnContextBound()
{
}

void UKSIScreen::OnContextReleased()
{
}

void UKSIScreen::OnPresentationChanged()
{
}

void UKSIScreen::OnManagedTick(float DeltaTime)
{
}

void UKSIScreen::ApplyPresentation(const FKSIUIViewPresentation& State, float FadeSeconds)
{
	const bool bChanged = Presentation.bVisible != State.bVisible || Presentation.bInputEnabled != State.bInputEnabled ||
		Presentation.bUpdatesEnabled != State.bUpdatesEnabled || Presentation.Opacity != State.Opacity;
	Presentation = State;
	const float NewTarget = State.bVisible ? State.Opacity : 0.f;
	if (!FMath::IsNearlyEqual(NewTarget, TargetOpacity))
	{
		StartOpacity = CurrentOpacity;
		TargetOpacity = NewTarget;
		FadeDuration = FadeSeconds;
		FadeElapsed = 0.f;
		if (FadeDuration <= 0.f)
		{
			CurrentOpacity = TargetOpacity;
		}
	}

	UpdatePresentationVisuals();
	if (bChanged && GetViewHandle().IsValid())
	{
		OnPresentationChanged();
	}
}

void UKSIScreen::TickManagedPresentation(float DeltaTime)
{
	if (FadeDuration > 0.f && FadeElapsed < FadeDuration)
	{
		FadeElapsed = FMath::Min(FadeElapsed + FMath::Max(0.f, DeltaTime), FadeDuration);
		CurrentOpacity = FMath::Lerp(StartOpacity, TargetOpacity, FadeElapsed / FadeDuration);
		UpdatePresentationVisuals();
	}

	if (GetViewHandle().IsValid())
	{
		OnManagedTick(DeltaTime);
	}
}

void UKSIScreen::UpdatePresentationVisuals()
{
	SetRenderOpacity(OriginalOpacity * CurrentOpacity);
	SetVisibility(CurrentOpacity <= 0.f && TargetOpacity <= 0.f ? ESlateVisibility::Hidden : OriginalVisibility);
	const bool bInteractive = Presentation.bVisible && Presentation.bInputEnabled && Presentation.Opacity > 0.f &&
		(FadeDuration <= 0.f || FadeElapsed >= FadeDuration);
	SetIsEnabled(bOriginalEnabled && bInteractive);
	bSupportsActivationFocus = bInteractive && InputPolicy != EKSIUIInputPolicy::Inherit;
	if ((bPresentationInteractive != 0) != bInteractive)
	{
		bPresentationInteractive = bInteractive;
		RefreshBackBinding();
		if (auto* Local = GetOwningLocalPlayer())
		{
			if (auto* UI = Local->GetSubsystem<UKSIUISubsystem>())
			{
				UI->InvalidatePresentationRouting();
			}
		}
	}
}

void UKSIScreen::RefreshBackBinding()
{
	BackBinding.Unregister();
	if (!GetCachedWidget().IsValid() || !ViewHandle.IsValid() || !bPresentationInteractive ||
		(InputPolicy != EKSIUIInputPolicy::Menu && InputPolicy != EKSIUIInputPolicy::GameAndUI))
	{
		return;
	}

	const auto& Settings = ICommonInputModule::GetSettings();
	const FSimpleDelegate Back = FSimpleDelegate::CreateWeakLambda(this, [this] { NativeOnHandleBackAction(); });
	if (Settings.GetEnableEnhancedInputSupport() && Settings.GetEnhancedInputBackAction())
	{
		BackBinding = RegisterUIActionBinding(FBindUIActionArgs(Settings.GetEnhancedInputBackAction(), Back));
	}
	else if (!Settings.GetDefaultBackAction().IsNull())
	{
		BackBinding = RegisterUIActionBinding(FBindUIActionArgs(Settings.GetDefaultBackAction(), Back));
	}
}
