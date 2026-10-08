#include "CCLUIFoundationSmoke.h"

#include "UI/Core/CCLUIContext.h"
#include "UI/Core/CCLUIRegistry.h"
#include "UI/Core/CCLUIRoot.h"
#include "UI/Core/CCLScreen.h"
#include "UI/Core/CCLUISubsystem.h"
#include "UI/CCLUIInputData.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NativeGameplayTags.h"
#include "Input/CommonUIActionRouterBase.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

namespace
{
UE_DEFINE_GAMEPLAY_TAG_STATIC(OverlayLayer, "UI.Layer.SmokeOverlay");
UE_DEFINE_GAMEPLAY_TAG_STATIC(StackLayer, "UI.Layer.SmokeStack");
UE_DEFINE_GAMEPLAY_TAG_STATIC(QueueLayer, "UI.Layer.SmokeQueue");
UE_DEFINE_GAMEPLAY_TAG_STATIC(CornerExtension, "UI.Extension.SmokeCorner");
UE_DEFINE_GAMEPLAY_TAG_STATIC(PanelView, "UI.View.SmokePanel");
UE_DEFINE_GAMEPLAY_TAG_STATIC(StackView, "UI.View.SmokeStack");
UE_DEFINE_GAMEPLAY_TAG_STATIC(QueueView, "UI.View.SmokeQueue");
UE_DEFINE_GAMEPLAY_TAG_STATIC(PersistentViewTag, "UI.View.SmokePersistent");
UE_DEFINE_GAMEPLAY_TAG_STATIC(PersistentStackTag, "UI.View.SmokePersistentStack");
UE_DEFINE_GAMEPLAY_TAG_STATIC(AsyncView, "UI.View.SmokeAsync");
UE_DEFINE_GAMEPLAY_TAG_STATIC(OwnedViewTag, "UI.View.SmokeOwned");
UE_DEFINE_GAMEPLAY_TAG_STATIC(StrategyResources, "UI.View.Example.Strategy.Resources");
UE_DEFINE_GAMEPLAY_TAG_STATIC(StrategySelection, "UI.View.Example.Strategy.Selection");
UE_DEFINE_GAMEPLAY_TAG_STATIC(StrategyMap, "UI.View.Example.Strategy.Minimap");
UE_DEFINE_GAMEPLAY_TAG_STATIC(StrategyMapGroup, "UI.Group.Example.Strategy.Map");
UE_DEFINE_GAMEPLAY_TAG_STATIC(NarrativeBody, "UI.View.Example.Narrative.Body");
UE_DEFINE_GAMEPLAY_TAG_STATIC(NarrativeChoices, "UI.View.Example.Narrative.Choices");
}

bool UCCLUIFoundationSmoke::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLUIFoundationSmoke"));
#endif
}

void UCCLUICloseOnHideProbe::OnPresentationChanged()
{
	if (!GetPresentation().bVisible)
	{
		RequestClose();
	}
}

TStatId UCCLUIFoundationSmoke::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLUIFoundationSmoke, STATGROUP_Tickables);
}

void UCCLUIFoundationSmoke::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	if (Started == 0.)
	{
		Started = Now;
	}

	if (!Check(Now - Started < 120., TEXT("foundation test deadline")) || Now < Next)
	{
		return;
	}

	auto* World = GetWorld();
	auto* PC = World ? World->GetFirstPlayerController() : nullptr;
	auto* Local = PC ? PC->GetLocalPlayer() : nullptr;
	auto* UI = Local ? Local->GetSubsystem<UCCLUISubsystem>() : nullptr;
	if (!World || !World->HasBegunPlay() || !PC || !PC->GetPawn() || !UI)
	{
		return;
	}

	if (Step == 0)
	{
		UI->CloseAllViews();
		Registry = NewObject<UCCLUIRegistry>(this);
		Registry->InputMapping = GetDefault<UCCLUIInputData>()->GetMapping();
		for (const auto& Pair : {TPair<FGameplayTag, ECCLUILayerLayout>(OverlayLayer, ECCLUILayerLayout::Overlay),
			TPair<FGameplayTag, ECCLUILayerLayout>(StackLayer, ECCLUILayerLayout::Stack),
			TPair<FGameplayTag, ECCLUILayerLayout>(QueueLayer, ECCLUILayerLayout::Queue)})
		{
			FCCLUILayerDefinition Layer;
			Layer.Tag = Pair.Key;
			Layer.Layout = Pair.Value;
			Layer.ZOrder = Registry->Layers.Num();
			Registry->Layers.Add(Layer);
		}

		FCCLUIExtensionDefinition Extension;
		Extension.Tag = CornerExtension;
		Extension.Layer = OverlayLayer;
		Registry->Extensions.Add(Extension);
		FCCLUIViewDefinition Definition;
		Definition.Tag = PanelView;
		Definition.Layer = OverlayLayer;
		Definition.Extension = CornerExtension;
		Definition.WidgetClass = UCCLScreen::StaticClass();
		Definition.RequiredContextClass = UCCLUIContext::StaticClass();
		Definition.InstancePolicy = ECCLUIInstancePolicy::PerContext;
		Registry->Views.Add(Definition);
		Definition.Tag = PersistentViewTag;
		Definition.Scope = ECCLUIScope::LocalPlayer;
		Registry->Views.Add(Definition);
		Definition.Scope = ECCLUIScope::World;
		Definition.Extension = {};
		Definition.InstancePolicy = ECCLUIInstancePolicy::Multiple;
		Definition.Tag = StackView;
		Definition.Layer = StackLayer;
		Definition.InputPolicy = ECCLUIInputPolicy::Menu;
		Registry->Views.Add(Definition);
		Definition.Tag = PersistentStackTag;
		Definition.Scope = ECCLUIScope::LocalPlayer;
		Registry->Views.Add(Definition);
		Definition.Scope = ECCLUIScope::World;
		Definition.Tag = QueueView;
		Definition.Layer = QueueLayer;
		Definition.InputPolicy = ECCLUIInputPolicy::Inherit;
		Registry->Views.Add(Definition);
		Definition.Tag = AsyncView;
		Definition.Layer = OverlayLayer;
		Definition.WidgetClass = TSoftClassPtr<UCCLScreen>(FSoftObjectPath(TEXT("/Game/Tests/UI/BP_UIAsyncProbe.BP_UIAsyncProbe_C")));
		Registry->Views.Add(Definition);
		ContextA = NewObject<UCCLUIContext>(this);
		ContextB = NewObject<UCCLUIContext>(this);
		ContextC = NewObject<UCCLUIContext>(this);
		if (!Check(UI->ConfigureRegistry(Registry), TEXT("configure data-driven layers and views")))
		{
			return;
		}

		PanelA = UI->OpenView(PanelView, ContextA, this);
		PanelB = UI->OpenView(PanelView, ContextB, this);
		if (!Check(PanelA.IsValid() && PanelB.IsValid() && !(PanelA == PanelB) &&
			UI->OpenView(PanelView, ContextA, this) == PanelA && !UI->OpenView(PanelView, nullptr, this).IsValid(),
			TEXT("same view uses separate context instances and rejects wrong context")))
		{
			return;
		}

		RecycledScreen = UI->FindScreen(PanelA);
		StackA = UI->OpenView(StackView, ContextA, this);
		StackB = UI->OpenView(StackView, ContextB, this);
		QueueA = UI->OpenView(QueueView, ContextA, this);
		QueueB = UI->OpenView(QueueView, ContextB, this);
	}
	else if (Step == 1)
	{
		if (!Check(UI->FindScreen(PanelA) && UI->FindScreen(PanelB) && UI->FindScreen(StackA) && UI->FindScreen(StackB) &&
			UI->FindScreen(QueueA) && UI->FindScreen(QueueB), TEXT("all layer hosts retain their instances")))
		{
			return;
		}

		if (!Check(UI->FindScreen(PanelA)->IsActivated() && UI->FindScreen(PanelB)->IsActivated() &&
			!UI->FindScreen(StackA)->IsActivated() && UI->FindScreen(StackB)->IsActivated() &&
			UI->FindScreen(QueueA)->IsActivated() && !UI->FindScreen(QueueB)->IsActivated(),
			TEXT("simultaneous overlays, last-in stack and first-in queue")))
		{
			return;
		}

		UI->CloseView(PanelA);
		UI->CloseView(PanelA);
		PanelA = UI->OpenView(PanelView, ContextC, this);
		if (!Check(UI->FindScreen(PanelA) == RecycledScreen.Get() && UI->FindScreen(PanelA)->GetContext() == ContextC &&
			UI->FindScreen(PanelB)->GetContext() == ContextB, TEXT("pooled overlay rebind preserves the other instance")))
		{
			return;
		}

		Local->GetSubsystem<UCommonUIActionRouterBase>()->ProcessInput(EKeys::Escape, IE_Pressed);
		Local->GetSubsystem<UCommonUIActionRouterBase>()->ProcessInput(EKeys::Escape, IE_Released);
		if (!Check(!UI->IsViewOpen(StackB), TEXT("CommonUI enhanced back action closes the focused stack view")))
		{
			return;
		}

		UI->CloseView(QueueA);
	}
	else if (Step == 2)
	{
		if (!Check(UI->FindScreen(StackA) && UI->FindScreen(StackA)->IsActivated() && UI->FindScreen(QueueB) &&
			UI->FindScreen(QueueB)->IsActivated(), TEXT("closing top stack and queue head activates the next view")))
		{
			return;
		}

		UI->CloseAllViews();
		UI->OnRequestFinished.AddUObject(this, &ThisClass::RequestFinished);
		if (!Check(!UI->GetRegistry()->FindView(AsyncView)->WidgetClass.Get(), TEXT("async fixture starts unloaded")))
		{
			return;
		}

		CancelledRequest = UI->RequestOpenView(AsyncView, ContextA, this);
		UI->CancelRequest(CancelledRequest);
		UI->CancelRequest(CancelledRequest);
		LoadingRequest = UI->RequestOpenView(AsyncView, ContextB, this);
		if (!Check(CancelledRequest.IsValid() && LoadingRequest.IsValid(), TEXT("cold asynchronous requests accepted")))
		{
			return;
		}
	}
	else if (Step == 3)
	{
		if (CompletedRequests == 0)
		{
			return;
		}

		if (!Check(CompletedRequests == 1 && CancelledRequests == 1 && UI->GetPendingCount() == 0 && UI->GetViewCount() == 1 &&
			UI->FindScreen(LoadedView) && UI->FindScreen(LoadedView)->GetContext() == ContextB,
			TEXT("cancelled load cannot reopen a view; surviving request binds its context")))
		{
			return;
		}

		UI->CloseAllViews();
		FeatureOwner = World->SpawnActor<AActor>();
		FCCLUIViewDefinition Owned = *Registry->FindView(PanelView);
		Owned.Tag = OwnedViewTag;
		Owned.InstancePolicy = ECCLUIInstancePolicy::Single;
		OwnedRegistration = UI->RegisterView(Owned, FeatureOwner.Get());
		OwnedView = UI->OpenView(OwnedViewTag, ContextA, this);
		if (!Check(OwnedRegistration.IsValid() && OwnedView.IsValid() &&
			!UI->RegisterView(Owned, this).IsValid(), TEXT("feature registration owns its view type and rejects duplicates")))
		{
			return;
		}

		bool bReentrantOpenRejected = false;
		const auto ClosedBinding = UI->OnViewClosed.AddLambda([&](FCCLUIViewHandle Closed)
		{
			bReentrantOpenRejected = !UI->OpenView(OwnedViewTag, ContextC, this).IsValid();
		});
		OwnedView = UI->OpenView(OwnedViewTag, ContextB, this);
		UI->OnViewClosed.Remove(ClosedBinding);
		if (!Check(OwnedView.IsValid() && UI->GetViewCount() == 1 && bReentrantOpenRejected,
			TEXT("single-instance replacement rejects recursive reopen callbacks")))
		{
			return;
		}

		FeatureOwner->Destroy();
	}
	else if (Step == 4)
	{
		if (!Check(!UI->IsViewOpen(OwnedView) && !UI->FindRegistration(OwnedViewTag).IsValid(),
			TEXT("destroying registration owner removes its screens")))
		{
			return;
		}

		UI->UnregisterView(OwnedRegistration);
		WorldView = UI->OpenView(PanelView, ContextA, this);
		PersistentView = UI->OpenView(PersistentViewTag, ContextB, this);
		PersistentStackA = UI->OpenView(PersistentStackTag, ContextA, this);
		PersistentStackB = UI->OpenView(PersistentStackTag, ContextB, this);
		FCCLUIPresentationDefinition WorldRestriction;
		WorldRestriction.bBlockGameplay = 1;
		UI->PushPresentation(WorldRestriction, this);
		FCCLUIPresentationDefinition PersistentRestriction;
		PersistentRestriction.bAllViews = 1;
		PersistentRestriction.bHide = 1;
		PersistentRestriction.Scope = ECCLUIScope::LocalPlayer;
		PersistentPresentation = UI->PushPresentation(PersistentRestriction, this);
		FString Error;
		auto* OtherLocal = GetGameInstance()->CreateLocalPlayer(1, Error, true);
		auto* OtherUI = OtherLocal ? OtherLocal->GetSubsystem<UCCLUISubsystem>() : nullptr;
		if (!Check(OtherUI && OtherUI != UI && OtherUI->ConfigureRegistry(Registry), TEXT("second local player has an independent manager")))
		{
			return;
		}

		const auto OtherView = OtherUI->OpenView(PanelView, ContextA, this);
		OtherUI->ReleasePresentation(PersistentPresentation);
		if (!Check(OtherUI->GetPresentationCount() == 0 && !OtherUI->IsGameplayInputBlocked() &&
			UI->GetPresentationCount() == 2 && UI->IsGameplayInputBlocked(), TEXT("presentation handles and input restrictions are local-player isolated")))
		{
			return;
		}

		UI->CloseView(OtherView);
		if (!Check(OtherView.IsValid() && OtherUI->IsViewOpen(OtherView) && UI->IsViewOpen(WorldView) &&
			OtherUI->FindScreen(OtherView)->GetOwningLocalPlayer() == OtherLocal,
			TEXT("handles and owning player stay isolated across local players")))
		{
			return;
		}

		GetGameInstance()->RemoveLocalPlayer(OtherLocal);
		PreviousWorld = World;
		UGameplayStatics::OpenLevel(World, TEXT("/Game/Maps/Campaign"));
	}
	else if (Step == 5)
	{
		if (World == PreviousWorld.Get() || !UI->FindScreen(PersistentView))
		{
			return;
		}

		if (!Check(UI->GetPresentationCount() == 1 && !UI->IsGameplayInputBlocked() &&
			UI->FindScreen(PersistentView)->GetVisibility() == ESlateVisibility::Hidden,
			TEXT("travel expires world restrictions and reapplies player restrictions to reattached views")))
		{
			return;
		}

		UI->ReleasePresentation(PersistentPresentation);

		if (!Check(!UI->IsViewOpen(WorldView) && UI->IsViewOpen(PersistentView) &&
			UI->FindScreen(PersistentView)->GetOwningPlayer() == PC && UI->FindScreen(PersistentView)->GetContext() == ContextB,
			TEXT("map travel closes world views and reattaches local-player views")))
		{
			return;
		}

		if (!Check(UI->FindScreen(PersistentStackA) && UI->FindScreen(PersistentStackB) &&
			!UI->FindScreen(PersistentStackA)->IsActivated() && UI->FindScreen(PersistentStackB)->IsActivated(),
			TEXT("map travel preserves the opening order of persistent stack screens")))
		{
			return;
		}

		bool bCloseAllReopenRejected = false;
		const auto ClosingBinding = UI->OnViewClosed.AddLambda([&](FCCLUIViewHandle Closed)
		{
			bCloseAllReopenRejected = !UI->OpenView(PanelView, ContextA, this).IsValid();
		});
		UI->CloseAllViews();
		UI->OnViewClosed.Remove(ClosingBinding);
		if (!Check(bCloseAllReopenRejected && UI->GetViewCount() == 0, TEXT("close all cannot be undone by a close callback")))
		{
			return;
		}

		UI->OnRequestFinished.RemoveAll(this);
	}
	else if (Step == 6)
	{
		if (!ConfigureExample(true))
		{
			return;
		}

		PanelA = UI->OpenView(StrategyResources, ContextA, this);
		PanelB = UI->OpenView(StrategySelection, ContextB, this);
		WorldView = UI->OpenView(StrategyMap, ContextC, this);
	}
	else if (Step == 7)
	{
		auto* Router = Local->GetSubsystem<UCommonUIActionRouterBase>();
		if (!Check(UI->FindScreen(PanelA) && UI->FindScreen(PanelB) && UI->FindScreen(WorldView) &&
			UI->FindScreen(PanelA)->IsActivated() && UI->FindScreen(PanelB)->IsActivated() &&
			UI->FindScreen(WorldView)->IsActivated() && Router->CanProcessNormalGameInput(),
			TEXT("strategy registry keeps resources selection and minimap active with gameplay input")))
		{
			return;
		}

		FCCLUIPresentationDefinition HideMap;
		HideMap.Groups.AddTag(StrategyMapGroup);
		HideMap.bHide = 1;
		HideMap.FadeSeconds = 0.2f;
		PersistentPresentation = UI->PushPresentation(HideMap, this);
		if (!Check(UGameplayStatics::SetGamePaused(World, true), TEXT("pause the actual game world for presentation regression")))
		{
			return;
		}
	}
	else if (Step == 8)
	{
		if (!Check(UGameplayStatics::IsGamePaused(World) && UI->FindScreen(WorldView)->GetVisibility() == ESlateVisibility::Hidden &&
			UI->FindScreen(PanelA)->IsVisible() && UI->FindScreen(PanelB)->IsVisible(),
			TEXT("paused-world fade hides only the tagged strategy minimap")))
		{
			return;
		}

		UI->ReleasePresentation(PersistentPresentation);
	}
	else if (Step == 9)
	{
		if (!Check(UGameplayStatics::IsGamePaused(World) && UI->FindScreen(WorldView)->IsVisible() &&
			FMath::IsNearlyEqual(UI->FindScreen(WorldView)->GetRenderOpacity(), 1.f),
			TEXT("hidden minimap finishes its restore fade while the game remains paused")))
		{
			return;
		}

		UGameplayStatics::SetGamePaused(World, false);
		UI->CloseAllViews();
		if (!ConfigureExample(false))
		{
			return;
		}

		PanelA = UI->OpenView(NarrativeBody, ContextA, this);
		StackA = UI->OpenView(NarrativeChoices, ContextB, this);
		FCCLUIPresentationDefinition Conversation;
		Conversation.bBlockGameplay = 1;
		PersistentPresentation = UI->PushPresentation(Conversation, this);
	}
	else if (Step == 10)
	{
		auto* Router = Local->GetSubsystem<UCommonUIActionRouterBase>();
		if (!Check(UI->FindScreen(PanelA) && UI->FindScreen(StackA) && UI->FindScreen(PanelA)->IsActivated() &&
			Router->GetLeafmostActivatableWidget() == UI->FindScreen(StackA) && !Router->CanProcessNormalGameInput(),
			TEXT("narrative registry keeps dialogue body visible while choices own menu input")))
		{
			return;
		}

		Router->ProcessInput(EKeys::Escape, IE_Pressed);
		Router->ProcessInput(EKeys::Escape, IE_Released);
	}
	else if (Step == 11)
	{
		if (!Check(!UI->IsViewOpen(StackA) && UI->IsViewOpen(PanelA) && UI->FindScreen(PanelA)->IsActivated() &&
			!Local->GetSubsystem<UCommonUIActionRouterBase>()->CanProcessNormalGameInput(),
			TEXT("closing narrative choices preserves dialogue and its separate gameplay restriction")))
		{
			return;
		}

		UI->ReleasePresentation(PersistentPresentation);
		UI->CloseAllViews();
	}
	else if (Step == 12)
	{
		if (!Check(UI->GetViewCount() == 0 && UI->GetPresentationCount() == 0 &&
			Local->GetSubsystem<UCommonUIActionRouterBase>()->CanProcessNormalGameInput(),
			TEXT("ending narrative mode releases all views and restores gameplay")))
		{
			return;
		}

		FCCLUIViewDefinition ClosingView;
		ClosingView.Tag = OwnedViewTag;
		ClosingView.Layer = OverlayLayer;
		ClosingView.WidgetClass = UCCLUICloseOnHideProbe::StaticClass();
		const auto Registration = UI->RegisterView(ClosingView, this);
		FCCLUIPresentationDefinition Hide;
		Hide.bAllViews = 1;
		Hide.bHide = 1;
		const auto Request = UI->PushPresentation(Hide, this);
		if (!Check(Registration.IsValid() && Request.IsValid() && !UI->OpenView(OwnedViewTag, nullptr, this).IsValid() &&
			UI->GetViewCount() == 0, TEXT("a view closed by its initial presentation hook cannot return a stale open handle")))
		{
			return;
		}

		UI->ReleasePresentation(Request);
		UI->UnregisterView(Registration);
		bComplete = 1;
		UE_LOG(LogTemp, Display, TEXT("CCL_UI_FOUNDATION PASS contexts layers pooling async owners players travel strategy narrative paused-fades"));
		FPlatformMisc::RequestExitWithStatus(false, 0);
	}

	++Step;
	Next = Now + 1.;
}

bool UCCLUIFoundationSmoke::ConfigureExample(bool bStrategy)
{
	Registry = NewObject<UCCLUIRegistry>(this);
	Registry->InputMapping = GetDefault<UCCLUIInputData>()->GetMapping();
	FCCLUILayerDefinition Layer;
	Layer.Tag = OverlayLayer;
	Layer.Layout = ECCLUILayerLayout::Overlay;
	Registry->Layers.Add(Layer);
	Layer.Tag = StackLayer;
	Layer.Layout = ECCLUILayerLayout::Stack;
	Layer.ZOrder = 1;
	Registry->Layers.Add(Layer);
	FCCLUIViewDefinition View;
	View.Layer = OverlayLayer;
	View.WidgetClass = UCCLScreen::StaticClass();
	View.RequiredContextClass = UCCLUIContext::StaticClass();
	View.Tag = bStrategy ? StrategyResources : NarrativeBody;
	Registry->Views.Add(View);
	View.Tag = bStrategy ? StrategySelection : NarrativeChoices;
	View.Layer = bStrategy ? OverlayLayer : StackLayer;
	View.InputPolicy = bStrategy ? ECCLUIInputPolicy::GameAndUI : ECCLUIInputPolicy::Menu;
	Registry->Views.Add(View);
	if (bStrategy)
	{
		View.Tag = StrategyMap;
		View.InputPolicy = ECCLUIInputPolicy::Inherit;
		View.Groups.AddTag(StrategyMapGroup);
		Registry->Views.Add(View);
	}

	auto* UI = GetWorld()->GetFirstPlayerController()->GetLocalPlayer()->GetSubsystem<UCCLUISubsystem>();
	return Check(UI->ConfigureRegistry(Registry), bStrategy ? TEXT("configure independent strategy profile") : TEXT("configure independent narrative profile"));
}

bool UCCLUIFoundationSmoke::Check(bool bCondition, const TCHAR* Message)
{
	if (!bCondition)
	{
		bComplete = 1;
		UE_LOG(LogTemp, Error, TEXT("CCL_UI_FOUNDATION FAIL %s"), Message);
		FPlatformMisc::RequestExitWithStatus(false, 1);
	}
	else if (FCString::Strcmp(Message, TEXT("foundation test deadline")) != 0)
	{
		UE_LOG(LogTemp, Display, TEXT("CCL_UI_FOUNDATION CHECK %s"), Message);
	}

	return bCondition;
}

void UCCLUIFoundationSmoke::RequestFinished(FCCLUIRequestHandle Request, FCCLUIViewHandle View, bool bSuccess)
{
	if (Request == CancelledRequest && !bSuccess)
	{
		++CancelledRequests;
	}
	else if (Request == LoadingRequest && bSuccess)
	{
		++CompletedRequests;
		LoadedView = View;
	}
	else
	{
		Check(false, TEXT("unexpected request completion"));
	}
}
