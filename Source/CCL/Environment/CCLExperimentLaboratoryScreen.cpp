#include "CCLExperimentScreen.h"

#include "CCLWorldEnvironmentConfig.h"
#include "CCLExperimentDirector.h"
#include "CCLExperimentPlayerController.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"

TSharedRef<SWidget> UCCLExperimentScreen::BuildLaboratoryControls()
{
	static const FTextBlockStyle ButtonText = FTextBlockStyle(FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText"))
		.SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 21));
	const auto* C = ACCLWorldEnvironmentConfig::Find(GetWorld());
	const int32 Zone = C ? C->LaboratoryZone : -1;
	const int32 Subject = C ? C->LaboratorySubject : 0;
	auto Panel = SNew(SVerticalBox);
	auto Choices = SNew(SVerticalBox);
	auto Enabled = [this]() { return Director() && Director()->IsReady() && Director()->CanOperate(Controller()); };
	for (int32 Z = 1; Z < 12; ++Z)
	{
		for (int32 S = 0; S < (Z == 7 ? 3 : 1); ++S)
		{
			Choices->AddSlot().AutoHeight()[SNew(SButton).ContentPadding(FMargin(14, 10)).TextStyle(&ButtonText)
				.Text(FText::FromString(ACCLWorldEnvironmentConfig::LaboratoryTitle(Z, S)))
				.IsEnabled_Lambda(Enabled)
				.OnClicked_Lambda([this, Z, S]() { if (auto* PC = Controller()) { PC->ConfigureLaboratory(Z, S, 0, 3); } return FReply::Handled(); })];
		}
	}
	Panel->AddSlot().AutoHeight()[SNew(SComboButton).ContentPadding(FMargin(12, 10))
		.ButtonContent()[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 20)).Text(FText::FromString(TEXT("전용 레벨 선택 · ") + ACCLWorldEnvironmentConfig::LaboratoryTitle(Zone, Subject)))]
		.MenuContent()[SNew(SBox).WidthOverride(520).MaxDesiredHeight(440)[SNew(SScrollBox) + SScrollBox::Slot()[Choices]]]];
	if (Zone < 0)
	{
		return Panel;
	}
	const auto Names = ACCLWorldEnvironmentConfig::LaboratoryVariants(Zone, Subject);
	auto Variant = MakeShared<int32>(FMath::Clamp(C->LaboratoryVariant, 0, Names.Num() - 1));
	auto Count = MakeShared<int32>(C->SampleCount());
	auto Selector = SNew(SComboButton).ContentPadding(FMargin(12, 10)).ButtonContent()[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 20)).Text_Lambda([Variant, Names]() { return FText::FromString(TEXT("시험 종류: ") + Names[*Variant]); })];
	TWeakPtr<SComboButton> WeakSelector = Selector;
	auto Variants = SNew(SVerticalBox);
	for (int32 I = 0; I < Names.Num(); ++I)
	{
		Variants->AddSlot().AutoHeight()[SNew(SButton).ContentPadding(FMargin(14, 10)).TextStyle(&ButtonText).Text(FText::FromString(Names[I]))
			.OnClicked_Lambda([Variant, I, WeakSelector]() { *Variant = I; if (auto Button = WeakSelector.Pin()) { Button->SetIsOpen(false); } return FReply::Handled(); })];
	}
	Selector->SetMenuContent(Variants);
	if (Zone == 6 || Zone == 7)
	{
		Panel->AddSlot().AutoHeight().Padding(0, 8)[Selector];
		Panel->AddSlot().AutoHeight()[SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 20)).Text(FText::FromString(TEXT("표본 수 (1-16)  ")))]
			+ SHorizontalBox::Slot().FillWidth(1)[SNew(SSpinBox<int32>).Font(FCoreStyle::GetDefaultFontStyle("Regular", 20)).MinValue(1).MaxValue(16).Value(*Count)
				.OnValueChanged_Lambda([Count](int32 Value) { *Count = Value; })]];
		Panel->AddSlot().AutoHeight().Padding(0, 8)[SNew(SButton).ContentPadding(FMargin(14, 10)).TextStyle(&ButtonText).Text(FText::FromString(TEXT("선택한 시험만 새로 배치")))
			.IsEnabled_Lambda(Enabled)
			.OnClicked_Lambda([this, Zone, Subject, Variant, Count]() { if (auto* PC = Controller()) { PC->ConfigureLaboratory(Zone, Subject, *Variant, *Count); } return FReply::Handled(); })];
	}
	FString Steps = TEXT("1. 종류·수량을 고르고 새로 배치한다.\n2. 배율을 입력하고 적용한다. 0은 정지, 1은 실시간, 60은 빠른 관찰.\n3. F7로 화면을 닫아 표본을 본다. F7로 다시 열면 관측값을 볼 수 있다.\n현재 실험 초기화는 시간과 표본을 시작 상태로 되돌린다. 레벨 이동은 새 실험을 시작한다.\n");
	if (Zone == 6)
	{
		Steps += TEXT("먹이를 추가한 뒤 섭취량·에너지를 확인한다. Actor는 개체마다 몸체가 있는 표시, Mass는 같은 개체를 가벼운 데이터와 구 표시로 처리하는 방식이다. 전환해도 개체·에너지·먹이 기록은 이어진다. 숨김은 화면만 감추고 계산한다. 이동을 보려면 1배를 사용한다. 높은 배율에서는 이동도 빠르게 진행된다.");
	}
	if (Zone == 7 && Subject == 0)
	{
		Steps += TEXT("비·가뭄·눈 오는 겨울은 지속할 환경 조건을 고른다. 60배로 진행해 토양 수분·생체량·성장·휴면 변화를 본다. 불붙이기는 첫 표본에서 시작한다. 자동 검사는 별도 계산으로 즉시 결과를 내며 장면을 진행시키지 않는다.");
	}
	if (Zone == 7 && Subject == 1)
	{
		Steps += TEXT("첫 표본 수명을 10초로 줄이거나 보호·치명 피해를 적용한다. 나이·체력·사망 원인을 확인한다.");
	}
	if (Zone == 7 && Subject == 2)
	{
		Steps += TEXT("건조·고온다습·냉각은 선택한 보관함의 앞으로의 조건을 바꾼다. 1배로 천천히, 60배로 빠르게 진행해 색과 초록 곰팡이 군락이 점차 변하는지 본다. 0배에서는 조건을 바꿔도 품질은 멈춘다. 냉각·건조는 이미 진행한 부패를 되돌리지 않는다. 밀폐는 부패가 느리고 곰팡이 노출은 0이다. 수량은 메시 하나가 나타내는 묶음의 개수다. 비교하려면 초기화 후 다른 조건을 선택한다.");
	}
	if (Zone == 1)
	{
		Steps += TEXT("세계 시간 조작과 천체 관측을 한 레벨에서 사용한다. 천체 관측창을 열면 배율은 관측용 시계에만 적용된다.");
	}
	if (Zone == 9)
	{
		Steps += TEXT("60배를 적용한 뒤 환경 순환 실행의 순환 시작을 누른다. 건조 → 적설 → 압축 → 융해·유출·진흙 → 결빙 → 건조를 두 번 진행한다. 아래 결과에서 현재 단계를 확인한다. F7로 닫고 경사 상단과 하단의 변화를 비교한다.");
	}
	if (Zone == 11)
	{
		Steps += TEXT("비·바람을 켜고 문을 닫힘·반 열림·열림으로 바꿔 본다. 실내 발화 후 연기 농도와 문 밖 배출을 관찰한다. 지붕 아래에는 비가 차폐된다. 불과 환기는 실제 게임 시간으로 진행된다.");
	}
	Panel->AddSlot().AutoHeight().Padding(0, 6)[SNew(SExpandableArea).HeaderPadding(FMargin(12, 10)).InitiallyCollapsed(true)
		.HeaderContent()[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 20)).Text(FText::FromString(TEXT("사용법")))]
		.BodyContent()[SNew(SBox).MaxDesiredHeight(220)[SNew(SScrollBox) + SScrollBox::Slot()[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 20)).AutoWrapText(true).Text(FText::FromString(Steps))]]]];
	return Panel;
}
