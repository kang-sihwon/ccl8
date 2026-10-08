#include "CCLHUDScreens.h"

#include "CCLCombatViewModel.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

namespace
{
UTextBlock* MakeText(UWidgetTree* Tree, int32 Size, FLinearColor Color, float WrapWidth)
{
	auto* Text = Tree->ConstructWidget<UTextBlock>();
	Text->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", Size));
	Text->SetColorAndOpacity(Color);
	Text->SetShadowColorAndOpacity(FLinearColor::Black);
	Text->SetShadowOffset(FVector2D(1.f));
	Text->SetWrapTextAt(WrapWidth);
	return Text;
}

void Position(UCanvasPanel* Canvas, UWidget* Widget, FVector2D Anchor, FVector2D Offset)
{
	// Canvas auto-size uses the explicit text wrapping / panel width below.
	auto* Slot = Canvas->AddChildToCanvas(Widget);
	Slot->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
	Slot->SetAlignment(Anchor);
	Slot->SetPosition(Offset);
	Slot->SetAutoSize(true);
}

UCanvasPanel* MakeCanvas(UCCLScreen* Screen, UWidgetTree* Tree)
{
	auto* Canvas = Tree->ConstructWidget<UCanvasPanel>();
	auto* DesignSize = Tree->ConstructWidget<USizeBox>();
	DesignSize->SetWidthOverride(1280.f);
	DesignSize->SetHeightOverride(720.f);
	DesignSize->SetContent(Canvas);
	auto* Scale = Tree->ConstructWidget<UScaleBox>();
	Scale->SetStretch(EStretch::ScaleToFit);
	Scale->SetContent(DesignSize);
	Tree->RootWidget = Scale;
	Screen->SetVisibility(ESlateVisibility::HitTestInvisible);
	return Canvas;
}
}

void UCCLHUDContext::Update(FString InOverview, FString InPrompts)
{
	if (Overview != InOverview || Prompts != InPrompts)
	{
		Overview = MoveTemp(InOverview);
		Prompts = MoveTemp(InPrompts);
		OnChanged.Broadcast();
	}
}

void UCCLDialogueContext::Update(const FString& InSpeaker, const FString& InBody)
{
	if (Speaker != InSpeaker || Body != InBody)
	{
		Speaker = InSpeaker;
		Body = InBody;
		OnChanged.Broadcast();
	}
}

void UCCLVitalsScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	auto* Canvas = MakeCanvas(this, WidgetTree);
	auto* Box = WidgetTree->ConstructWidget<UVerticalBox>();
	Values = MakeText(WidgetTree, 22, FLinearColor(0.7f, 1.f, 0.75f), 650.f);
	Box->AddChildToVerticalBox(Values);
	HealthBar = WidgetTree->ConstructWidget<UProgressBar>();
	StaminaBar = WidgetTree->ConstructWidget<UProgressBar>();
	HealthBar->SetFillColorAndOpacity(FLinearColor(0.75f, 0.12f, 0.12f));
	StaminaBar->SetFillColorAndOpacity(FLinearColor(0.16f, 0.7f, 0.25f));
	for (auto* Bar : {HealthBar.Get(), StaminaBar.Get()})
	{
		auto* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(420.f);
		Size->SetHeightOverride(8.f);
		Size->SetContent(Bar);
		Box->AddChildToVerticalBox(Size)->SetPadding(FMargin(0.f, 3.f));
	}

	Position(Canvas, Box, FVector2D::ZeroVector, FVector2D(24.f, 100.f));
}

void UCCLVitalsScreen::OnContextBound()
{
	if (auto* Context = Cast<UCCLHUDContext>(GetContext()); Context && Context->Vitals)
	{
		for (const auto& Field : {UCCLCombatViewModel::FFieldNotificationClassDescriptor::Health,
			UCCLCombatViewModel::FFieldNotificationClassDescriptor::MaxHealth,
			UCCLCombatViewModel::FFieldNotificationClassDescriptor::Stamina,
			UCCLCombatViewModel::FFieldNotificationClassDescriptor::MaxStamina})
		{
			Context->Vitals->AddFieldValueChangedDelegate(Field,
				INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateWeakLambda(this,
					[this](UObject*, UE::FieldNotification::FFieldId) { Refresh(); }));
		}
	}

	Refresh();
}

void UCCLVitalsScreen::OnContextReleased()
{
	if (auto* Context = Cast<UCCLHUDContext>(GetContext()); Context && Context->Vitals)
	{
		Context->Vitals->RemoveAllFieldValueChangedDelegates(this);
	}
}

void UCCLVitalsScreen::Refresh()
{
	const auto* Context = Cast<UCCLHUDContext>(GetContext());
	const auto* Model = Context ? Context->Vitals.Get() : nullptr;
	if (!Model || !Values)
	{
		return;
	}

	++RefreshCount;
	Values->SetText(FText::FromString(FString::Printf(TEXT("HP %.0f / %.0f   Stamina %.0f / %.0f"),
		Model->Health, Model->MaxHealth, Model->Stamina, Model->MaxStamina)));
	HealthBar->SetPercent(Model->MaxHealth > 0.f ? FMath::Clamp(Model->Health / Model->MaxHealth, 0.f, 1.f) : 0.f);
	StaminaBar->SetPercent(Model->MaxStamina > 0.f ? FMath::Clamp(Model->Stamina / Model->MaxStamina, 0.f, 1.f) : 0.f);
}

FString UCCLVitalsScreen::GetDisplayedText() const
{
	return Values ? Values->GetText().ToString() : FString();
}

void UCCLFieldHUDScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	auto* Canvas = MakeCanvas(this, WidgetTree);
	auto* Instructions = MakeText(WidgetTree, 18, FLinearColor::White, 650.f);
	Instructions->SetText(FText::FromString(TEXT("WASD Move | Mouse Look | Space Jump\nLMB Left hand | RMB Right hand | Q Parry | Shift Dodge")));
	Position(Canvas, Instructions, FVector2D::ZeroVector, FVector2D(24.f));
	Overview = MakeText(WidgetTree, 18, FLinearColor(1.f, 0.8f, 0.3f), 650.f);
	Position(Canvas, Overview, FVector2D::ZeroVector, FVector2D(24.f, 170.f));
	Prompts = MakeText(WidgetTree, 18, FLinearColor::White, 640.f);
	PromptPanel = WidgetTree->ConstructWidget<UBorder>();
	PromptPanel->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.03f, 0.85f));
	PromptPanel->SetPadding(FMargin(12.f));
	PromptPanel->SetContent(Prompts);
	Position(Canvas, PromptPanel, FVector2D(0.f, 1.f), FVector2D(16.f, -16.f));
}

void UCCLFieldHUDScreen::OnContextBound()
{
	if (auto* Context = Cast<UCCLHUDContext>(GetContext()))
	{
		Context->OnChanged.AddUObject(this, &ThisClass::Refresh);
	}

	Refresh();
}

void UCCLFieldHUDScreen::OnContextReleased()
{
	if (auto* Context = Cast<UCCLHUDContext>(GetContext()))
	{
		Context->OnChanged.RemoveAll(this);
	}
}

void UCCLFieldHUDScreen::Refresh()
{
	if (const auto* Context = Cast<UCCLHUDContext>(GetContext()); Context && Overview && Prompts)
	{
		Overview->SetText(FText::FromString(Context->Overview));
		Prompts->SetText(FText::FromString(Context->Prompts));
		PromptPanel->SetVisibility(Context->Prompts.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UCCLDialogueScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	auto* Canvas = MakeCanvas(this, WidgetTree);
	auto* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.03f, 0.94f));
	Panel->SetPadding(FMargin(20.f, 16.f));
	auto* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(880.f);
	Size->SetContent(Panel);
	auto* Lines = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Lines);
	Speaker = MakeText(WidgetTree, 22, FLinearColor(1.f, 0.82f, 0.4f), 840.f);
	Body = MakeText(WidgetTree, 20, FLinearColor::White, 840.f);
	auto* Hint = MakeText(WidgetTree, 17, FLinearColor(0.8f, 0.85f, 0.9f), 840.f);
	Hint->SetText(FText::FromString(TEXT("T Talk / Quest | B Buy potion | Esc Close")));
	Lines->AddChildToVerticalBox(Speaker);
	Lines->AddChildToVerticalBox(Body)->SetPadding(FMargin(0.f, 8.f, 0.f, 12.f));
	Lines->AddChildToVerticalBox(Hint);
	Position(Canvas, Size, FVector2D(0.5f, 1.f), FVector2D(0.f, -24.f));
}

void UCCLDialogueScreen::OnContextBound()
{
	if (auto* Context = Cast<UCCLDialogueContext>(GetContext()))
	{
		Context->OnChanged.AddUObject(this, &ThisClass::Refresh);
	}

	Refresh();
}

void UCCLDialogueScreen::OnContextReleased()
{
	if (auto* Context = Cast<UCCLDialogueContext>(GetContext()))
	{
		Context->OnChanged.RemoveAll(this);
	}
}

void UCCLDialogueScreen::Refresh()
{
	if (const auto* Context = Cast<UCCLDialogueContext>(GetContext()); Context && Speaker && Body)
	{
		Speaker->SetText(FText::FromString(Context->Speaker));
		Body->SetText(FText::FromString(Context->Body));
	}
}

FString UCCLDialogueScreen::GetDisplayedBody() const
{
	return Body ? Body->GetText().ToString() : FString();
}
