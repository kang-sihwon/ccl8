#include "CCLScreen.h"

#include "CCLUIContext.h"
#include "CommonInputSettings.h"
#include "ICommonInputModule.h"
#include "Input/CommonUIInputTypes.h"
#include "Input/UIActionBindingHandle.h"

UCCLScreen::UCCLScreen(const FObjectInitializer& Initializer) : Super(Initializer)
{
	bAutoRestoreFocus = true;
	bIsBackHandler = false;
	SetIsFocusable(true);
}

TOptional<FUIInputConfig> UCCLScreen::GetDesiredInputConfig() const
{
	if (InputPolicy == ECCLUIInputPolicy::Inherit)
	{
		return TOptional<FUIInputConfig>();
	}

	if (InputPolicy == ECCLUIInputPolicy::Game)
	{
		return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
	}

	FUIInputConfig Config(InputPolicy == ECCLUIInputPolicy::Menu ? ECommonInputMode::Menu : ECommonInputMode::All,
		EMouseCaptureMode::NoCapture, EMouseLockMode::DoNotLock, false);
	Config.bIgnoreMoveInput = InputPolicy == ECCLUIInputPolicy::Menu;
	Config.bIgnoreLookInput = InputPolicy == ECCLUIInputPolicy::Menu;
	return Config;
}

void UCCLScreen::NativeConstruct()
{
	// Context can be assigned after a pool has constructed the Slate widget.
	// Own this binding so a passive HUD never consumes a menu's Back action.
	bIsBackHandler = false;
	Super::NativeConstruct();
	RefreshBackBinding();
}

void UCCLScreen::NativeDestruct()
{
	Super::NativeDestruct();
	ReleaseContext();
}

UWidget* UCCLScreen::NativeGetDesiredFocusTarget() const
{
	if (UWidget* Target = Super::NativeGetDesiredFocusTarget())
	{
		return Target;
	}

	return const_cast<UCCLScreen*>(this);
}

bool UCCLScreen::NativeOnHandleBackAction()
{
	RequestClose();
	return true;
}

void UCCLScreen::BindContext(UCCLUIContext* Context, FCCLUIViewHandle Handle, ECCLUIInputPolicy Policy)
{
	ReleaseContext();
	ViewContext = Context;
	ViewHandle = Handle;
	InputPolicy = Policy;
	bSupportsActivationFocus = Policy != ECCLUIInputPolicy::Inherit;
	RefreshBackBinding();
	OnContextBound();
}

void UCCLScreen::ReleaseContext()
{
	BackBinding.Unregister();
	if (ViewHandle.IsValid())
	{
		OnContextReleased();
	}

	ViewContext = nullptr;
	ViewHandle = {};
	InputPolicy = ECCLUIInputPolicy::Inherit;
	OnCloseRequested.Clear();
	ClearFocusRestorationTarget();
}

void UCCLScreen::RequestClose()
{
	OnCloseRequested.Broadcast(ViewHandle);
}

void UCCLScreen::OnContextBound()
{
}

void UCCLScreen::OnContextReleased()
{
}

void UCCLScreen::RefreshBackBinding()
{
	BackBinding.Unregister();
	if (!GetCachedWidget().IsValid() || !ViewHandle.IsValid() ||
		(InputPolicy != ECCLUIInputPolicy::Menu && InputPolicy != ECCLUIInputPolicy::GameAndUI))
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
