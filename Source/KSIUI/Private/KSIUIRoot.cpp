#include "KSIUIRoot.h"

#include "KSIScreen.h"
#include "KSIUIRegistry.h"
#include "KSIUISubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SafeZone.h"
#include "Input/UIActionBindingHandle.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

UKSIUIRoot::UKSIUIRoot(const FObjectInitializer& Initializer) : Super(Initializer), OverlayPool(*this)
{
	bAutoActivate = true;
	bSupportsActivationFocus = true;
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

TOptional<FUIInputConfig> UKSIUIRoot::GetDesiredInputConfig() const
{
	const auto* Local = GetOwningLocalPlayer();
	const auto* UI = Local ? Local->GetSubsystem<UKSIUISubsystem>() : nullptr;
	if (UI && UI->IsGameplayInputBlocked())
	{
		FUIInputConfig Config(ECommonInputMode::Menu, EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
		Config.bIgnoreMoveInput = true;
		Config.bIgnoreLookInput = true;
		return Config;
	}

	return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
}

void UKSIUIRoot::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	OverlayPool.ReleaseAllSlateResources();
}

bool UKSIUIRoot::Configure(UKSIUIRegistry* Registry)
{
	TArray<FText> Errors;
	if (Layout || !Registry || !Registry->ValidateRegistry(Errors) || !WidgetTree)
	{
		return false;
	}

	Layout = Registry;
	InputMapping = Registry->InputMapping;
	InputMappingPriority = Registry->InputMappingPriority;
	auto* SafeZone = WidgetTree->ConstructWidget<USafeZone>();
	auto* LayerPanel = WidgetTree->ConstructWidget<UOverlay>();
	SafeZone->AddChild(LayerPanel);
	WidgetTree->RootWidget = SafeZone;
	SafeZone->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	LayerPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	TArray<FKSIUILayerDefinition> OrderedLayers = Registry->Layers;
	OrderedLayers.Sort([](const auto& A, const auto& B) { return A.ZOrder < B.ZOrder; });
	for (const auto& Layer : OrderedLayers)
	{
		UWidget* Host = nullptr;
		if (Layer.Layout == EKSIUILayerLayout::Overlay)
		{
			auto* Panel = WidgetTree->ConstructWidget<UCanvasPanel>();
			Panel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			Overlays.Add(Layer.Tag, Panel);
			Host = Panel;
		}
		else
		{
			UCommonActivatableWidgetContainerBase* Container = Layer.Layout == EKSIUILayerLayout::Stack
				? static_cast<UCommonActivatableWidgetContainerBase*>(WidgetTree->ConstructWidget<UCommonActivatableWidgetStack>())
				: static_cast<UCommonActivatableWidgetContainerBase*>(WidgetTree->ConstructWidget<UCommonActivatableWidgetQueue>());
			Container->SetTransitionDuration(0.f);
			Containers.Add(Layer.Tag, Container);
			Host = Container;
		}

		auto* LayerSlot = LayerPanel->AddChildToOverlay(Host);
		LayerSlot->SetHorizontalAlignment(HAlign_Fill);
		LayerSlot->SetVerticalAlignment(VAlign_Fill);
	}

	for (const auto& Extension : Registry->Extensions)
	{
		auto* Panel = Overlays.FindChecked(Extension.Layer).Get();
		auto* PaddingBorder = WidgetTree->ConstructWidget<UBorder>();
		PaddingBorder->SetBrushColor(FLinearColor::Transparent);
		PaddingBorder->SetPadding(Extension.Padding);
		PaddingBorder->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		auto* Content = WidgetTree->ConstructWidget<UOverlay>();
		Content->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		PaddingBorder->SetContent(Content);
		auto* ExtensionSlot = Panel->AddChildToCanvas(PaddingBorder);
		ExtensionSlot->SetAnchors(FAnchors(Extension.Alignment.X, Extension.Alignment.Y));
		ExtensionSlot->SetAlignment(Extension.Alignment);
		ExtensionSlot->SetAutoSize(true);
		ExtensionSlot->SetPosition(FVector2D::ZeroVector);
		ExtensionHosts.Add(Extension.Tag, Content);
	}

	return true;
}

UKSIScreen* UKSIUIRoot::AddScreen(const FKSIUIViewDefinition& Definition, TFunctionRef<void(UKSIScreen&)> InitializeScreen)
{
	if (!Layout || !Definition.WidgetClass.Get())
	{
		return nullptr;
	}

	if (auto* Container = Containers.Find(Definition.Layer))
	{
		return (*Container)->AddWidget<UKSIScreen>(Definition.WidgetClass.Get(), InitializeScreen);
	}

	auto* Layer = Overlays.Find(Definition.Layer);
	if (!Layer || (Definition.Extension.IsValid() && !ExtensionHosts.Contains(Definition.Extension)))
	{
		return nullptr;
	}

	auto* Screen = OverlayPool.GetOrCreateInstance<UKSIScreen>(Definition.WidgetClass.Get());
	if (!Screen)
	{
		return nullptr;
	}

	InitializeScreen(*Screen);
	if (Definition.Extension.IsValid())
	{
		ExtensionHosts.FindChecked(Definition.Extension)->AddChildToOverlay(Screen);
	}
	else
	{
		auto* ScreenSlot = (*Layer)->AddChildToCanvas(Screen);
		ScreenSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		ScreenSlot->SetOffsets(FMargin(0.f));
	}

	Screen->ActivateWidget();
	return Screen;
}

void UKSIUIRoot::RemoveScreen(UKSIScreen* Screen)
{
	if (!Screen)
	{
		return;
	}

	Screen->ReleaseContext();
	for (const auto& Pair : Containers)
	{
		if (Pair.Value->GetWidgetList().Contains(Screen))
		{
			Pair.Value->RemoveWidget(*Screen);
			return;
		}
	}

	Screen->DeactivateWidget();
	Screen->RemoveFromParent();
	OverlayPool.Release(Screen, true);
}

void UKSIUIRoot::ClearScreens()
{
	for (const auto& Pair : Containers)
	{
		const TArray<UCommonActivatableWidget*> Active = Pair.Value->GetWidgetList();
		for (auto* Widget : Active)
		{
			if (auto* Screen = Cast<UKSIScreen>(Widget))
			{
				Screen->ReleaseContext();
			}
		}

		Pair.Value->ClearWidgets();
	}

	const TArray<UUserWidget*> Active = OverlayPool.GetActiveWidgets();
	for (auto* Widget : Active)
	{
		RemoveScreen(Cast<UKSIScreen>(Widget));
	}

	OverlayPool.ResetPool();
}

bool UKSIUIRoot::ContainsScreen(const UKSIScreen* Screen) const
{
	if (!Screen)
	{
		return false;
	}

	for (const auto& Pair : Containers)
	{
		if (Pair.Value->GetWidgetList().Contains(Screen))
		{
			return true;
		}
	}

	return OverlayPool.GetActiveWidgets().Contains(Screen);
}

const UCommonActivatableWidgetContainerBase* UKSIUIRoot::GetContainer(FGameplayTag Layer) const
{
	const auto* Container = Containers.Find(Layer);
	return Container ? Container->Get() : nullptr;
}
