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
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 32)).Text(FText::FromString(TEXT("CCL · 동쪽 성문 원정")))];
	Choices->AddSlot().AutoHeight().Padding(0.f, 6.f)
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).Text(FText::FromString(TEXT("캠페인 시제품 · 혼자 하기 / 직접 접속 멀티플레이")))];
	if (Context->bInCampaign)
	{
		AddButton(TEXT("계속하기"), [Session, Controller = Context->Controller]()
		{
			if (Session.IsValid() && Controller.IsValid())
			{
				Session->HideMenuForPlayer(Controller.Get());
			}
		});
		AddButton(TEXT("호스트 진행 상황 저장"), [Session]() { if (Session.IsValid()) { Session->SaveSession(); } });
		AddButton(TEXT("시작 화면으로 (저장하지 않은 진행은 사라짐)"), Action(&UCCLGameInstance::ReturnToMenu));
	}
	else
	{
		AddButton(TEXT("혼자 새 원정 시작"), [Session]() { if (Session.IsValid()) { Session->StartNew(false); } });
		AddButton(TEXT("새 원정 방 열기"), [Session]() { if (Session.IsValid()) { Session->StartNew(true); } });
		AddButton(TEXT("혼자 저장한 원정 계속"), [Session]() { if (Session.IsValid()) { Session->LoadSession(false); } });
		AddButton(TEXT("저장한 원정 방 열기"), [Session]() { if (Session.IsValid()) { Session->LoadSession(true); } });
		Choices->AddSlot().AutoHeight().Padding(0.f, 6.f)
			[SAssignNew(AddressBox, SEditableTextBox).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
			.Text(FText::FromString(TEXT("127.0.0.1:7777"))).HintText(FText::FromString(TEXT("호스트 이름 또는 IPv4 주소:포트")))];
		AddButton(TEXT("입력한 주소로 접속"), [Session, Address = TWeakPtr<SEditableTextBox>(AddressBox)]()
		{
			if (const auto Box = Address.Pin(); Session.IsValid() && Box)
			{
				Session->Join(Box->GetText().ToString());
			}
		});
	}

	AddButton(TEXT("그래픽 품질 변경 (낮음 / 보통 / 높음 / 최고)"), Action(&UCCLGameInstance::CycleQuality));
	AddButton(TEXT("화면 모드 전환 (1280×720 창 / 테두리 없는 창)"), Action(&UCCLGameInstance::ToggleWindowMode));
	AddButton(TEXT("게임 종료"), Action(&UCCLGameInstance::Quit));
	Choices->AddSlot().AutoHeight().Padding(0.f, 12.f)
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).WrapTextAt(780.f)
		.Text_Lambda([Session]() { return FText::FromString(Session.IsValid() ? Session->GetStatus() : FString()); })];
	Choices->AddSlot().AutoHeight()
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).WrapTextAt(780.f)
		.Text(FText::FromString(TEXT("호스트의 진행만 저장되며, 참가자의 진행은 이번 세션에서만 유지된다. 메뉴를 열어도 게임은 멈추지 않는다.")))];
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
