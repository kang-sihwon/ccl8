#include "CCLSessionMenuScreen.h"

#include "Session/CCLGameInstance.h"
#include "Blueprint/WidgetTree.h"
#include "Components/NativeWidgetHost.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"

void UCCLSessionMenuScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	Host = WidgetTree->ConstructWidget<UNativeWidgetHost>();
	WidgetTree->RootWidget = Host;
}

FReply UCCLSessionMenuScreen::NativeOnFocusReceived(const FGeometry& Geometry, const FFocusEvent& Event)
{
	return FirstButton ? FReply::Handled().SetUserFocus(FirstButton.ToSharedRef(), Event.GetCause()) :
		Super::NativeOnFocusReceived(Geometry, Event);
}

bool UCCLSessionMenuScreen::NativeOnHandleBackAction()
{
	if (const auto* Context = Cast<UCCLSessionMenuContext>(GetContext()); Context && Context->bInCampaign)
	{
		RequestClose();
	}

	return true;
}

void UCCLSessionMenuScreen::OnContextBound()
{
	const auto* Context = Cast<UCCLSessionMenuContext>(GetContext());
	if (!Context || !Context->Session.IsValid())
	{
		return;
	}

	const TWeakObjectPtr<UCCLGameInstance> Session = Context->Session;
	auto Choices = SNew(SVerticalBox);
	auto AddButton = [this, &Choices](const TCHAR* Label, TFunction<void()> Action)
	{
		TSharedPtr<SButton> Button;
		Choices->AddSlot().AutoHeight().Padding(0.f, 3.f)
		[SAssignNew(Button, SButton).ContentPadding(FMargin(12.f, 7.f))
			.OnClicked_Lambda([Action]() { Action(); return FReply::Handled(); })
			[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).Text(FText::FromString(Label))]];
		if (!FirstButton)
		{
			FirstButton = Button;
		}
	};
	auto Action = [Session](void (UCCLGameInstance::*Method)())
	{
		return [Session, Method]() { if (Session.IsValid()) { (Session.Get()->*Method)(); } };
	};
	Choices->AddSlot().AutoHeight().Padding(0.f, 4.f)
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 32)).Text(FText::FromString(TEXT("CCL | EAST GATE EXPEDITION")))];
	Choices->AddSlot().AutoHeight().Padding(0.f, 6.f)
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).Text(FText::FromString(TEXT("Prototype campaign - solo or direct multiplayer")))];
	if (Context->bInCampaign)
	{
		AddButton(TEXT("Resume"), [Session, Controller = Context->Controller]()
		{
			if (Session.IsValid() && Controller.IsValid())
			{
				Session->HideMenuForPlayer(Controller.Get());
			}
		});
		AddButton(TEXT("Save host checkpoint"), [Session]() { if (Session.IsValid()) { Session->SaveSession(); } });
		AddButton(TEXT("Return to start screen (unsaved changes are lost)"), Action(&UCCLGameInstance::ReturnToMenu));
	}
	else
	{
		AddButton(TEXT("New solo expedition"), [Session]() { if (Session.IsValid()) { Session->StartNew(false); } });
		AddButton(TEXT("Host new expedition"), [Session]() { if (Session.IsValid()) { Session->StartNew(true); } });
		AddButton(TEXT("Load solo checkpoint"), [Session]() { if (Session.IsValid()) { Session->LoadSession(false); } });
		AddButton(TEXT("Host saved checkpoint"), [Session]() { if (Session.IsValid()) { Session->LoadSession(true); } });
		Choices->AddSlot().AutoHeight().Padding(0.f, 6.f)
			[SAssignNew(AddressBox, SEditableTextBox).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
			.Text(FText::FromString(TEXT("127.0.0.1:7777"))).HintText(FText::FromString(TEXT("Host name or IPv4:port")))];
		AddButton(TEXT("Join address"), [Session, Address = TWeakPtr<SEditableTextBox>(AddressBox)]()
		{
			if (const auto Box = Address.Pin(); Session.IsValid() && Box)
			{
				Session->Join(Box->GetText().ToString());
			}
		});
	}

	AddButton(TEXT("Cycle graphics quality (Low / Medium / High / Epic)"), Action(&UCCLGameInstance::CycleQuality));
	AddButton(TEXT("Toggle 1280x720 window / borderless desktop"), Action(&UCCLGameInstance::ToggleWindowMode));
	AddButton(TEXT("Quit game"), Action(&UCCLGameInstance::Quit));
	Choices->AddSlot().AutoHeight().Padding(0.f, 12.f)
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).WrapTextAt(780.f)
		.Text_Lambda([Session]() { return FText::FromString(Session.IsValid() ? Session->GetStatus() : FString()); })];
	Choices->AddSlot().AutoHeight()
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).WrapTextAt(780.f)
		.Text(FText::FromString(TEXT("Host checkpoint only; guests are session-only. Opening this menu does not pause the world.")))];
	Host->SetContent(SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.035f, 0.06f, 0.09f, 0.97f)).HAlign(HAlign_Center).VAlign(VAlign_Center)
		[SNew(SBox).WidthOverride(800.f).MaxDesiredHeight(940.f)[SNew(SScrollBox) + SScrollBox::Slot()[Choices]]]);
}

void UCCLSessionMenuScreen::OnContextReleased()
{
	if (Host)
	{
		Host->SetContent(SNullWidget::NullWidget);
	}

	AddressBox.Reset();
	FirstButton.Reset();
}
