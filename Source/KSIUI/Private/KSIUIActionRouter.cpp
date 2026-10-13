#include "KSIUIActionRouter.h"

#include "KSIUISubsystem.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "Widgets/SViewport.h"
#include "Widgets/SWindow.h"

void UKSIUIActionRouter::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (FSlateApplication::IsInitialized())
	{
		FocusChangingHandle = FSlateApplication::Get().OnFocusChanging().AddUObject(this, &ThisClass::HandleFocusChanging);
	}

	RefreshTickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ThisClass::TickPendingRefresh));
}

void UKSIUIActionRouter::Deinitialize()
{
	bShuttingDown = 1;
	FTSTicker::GetCoreTicker().RemoveTicker(RefreshTickHandle);
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().OnFocusChanging().Remove(FocusChangingHandle);
	}

	Super::Deinitialize();
}

bool UKSIUIActionRouter::CanProcessNormalGameInput() const
{
	const auto* UI = GetLocalPlayer()->GetSubsystem<UKSIUISubsystem>();
	return (!UI || !UI->IsGameplayInputBlocked()) && Super::CanProcessNormalGameInput();
}

void UKSIUIActionRouter::ApplyUIInputConfig(const FUIInputConfig& NewConfig, bool bForceRefresh)
{
	// Do not cache an unapplied config: CommonUI uses it to balance input suppression.
	if (!IsInputTargetReady())
	{
		bRefreshPending = 1;
		return;
	}

	Super::ApplyUIInputConfig(NewConfig, bForceRefresh);
}

void UKSIUIActionRouter::RefreshPresentationRouting()
{
	if (!IsInputTargetReady())
	{
		bRefreshPending = 1;
		return;
	}

	if (auto ActiveRoot = GetActiveRoot().Pin())
	{
		// Re-evaluate the existing tree without deactivating/popping any screen.
		SetActiveRoot(ActiveRoot);
	}
}

void UKSIUIActionRouter::HandleFocusChanging(const FFocusEvent& Event, const FWeakWidgetPath& OldPath,
	const TSharedPtr<SWidget>& OldWidget, const FWidgetPath& NewPath, const TSharedPtr<SWidget>& NewWidget)
{
	if (Event.GetUser() == GetLocalPlayerIndex())
	{
		// Slate has not committed the new focus path yet.
		bRefreshPending = 1;
	}
}

bool UKSIUIActionRouter::TickPendingRefresh(float DeltaTime)
{
	if (!bRefreshPending || !IsInputTargetReady() || !GetActiveRoot().IsValid())
	{
		return true;
	}

#if WITH_EDITOR
	const auto Window = GetLocalPlayer()->ViewportClient->GetWindow();
	const auto User = FSlateApplication::Get().GetUser(GetLocalPlayerIndex());
	if (Window && (!User || (!User->HasFocus(Window) && !User->HasFocusedDescendants(Window.ToSharedRef()))))
	{
		return true;
	}
#endif

	bRefreshPending = 0;
	RefreshPresentationRouting();
	return true;
}

bool UKSIUIActionRouter::IsInputTargetReady() const
{
	const auto* World = GetWorld();
	const auto* Player = GetLocalPlayer();
	const auto* PC = Player && World ? Player->GetPlayerController(World) : nullptr;
	return !bShuttingDown && World && !World->bIsTearingDown && IsValid(PC) && IsValid(PC->PlayerInput) &&
		Player->ViewportClient && Player->ViewportClient->GetGameViewportWidget().IsValid() && FSlateApplication::IsInitialized();
}
