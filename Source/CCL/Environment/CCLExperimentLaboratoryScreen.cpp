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
	const auto* C = ACCLWorldEnvironmentConfig::Find(GetWorld());
	const int32 Zone = C ? C->LaboratoryZone : -1;
	const int32 Subject = C ? C->LaboratorySubject : 0;
	auto Panel = SNew(SVerticalBox);
	auto Choices = SNew(SVerticalBox);
	auto Enabled = [this]() { return Director() && Director()->IsReady() && Director()->CanOperate(Controller()); };
	for (int32 Z = 0; Z < 12; ++Z)
	{
		for (int32 S = 0; S < (Z == 7 ? 3 : 1); ++S)
		{
			Choices->AddSlot().AutoHeight()[SNew(SButton)
				.Text(FText::FromString(ACCLWorldEnvironmentConfig::LaboratoryTitle(Z, S)))
				.IsEnabled_Lambda(Enabled)
				.OnClicked_Lambda([this, Z, S]() { if (auto* PC = Controller()) { PC->ConfigureLaboratory(Z, S, 0, 3); } return FReply::Handled(); })];
		}
	}
	Panel->AddSlot().AutoHeight()[SNew(SComboButton)
		.ButtonContent()[SNew(STextBlock).Text(FText::FromString(TEXT("전용 레벨 선택 · ") + ACCLWorldEnvironmentConfig::LaboratoryTitle(Zone, Subject)))]
		.MenuContent()[SNew(SBox).WidthOverride(390).MaxDesiredHeight(340)[SNew(SScrollBox) + SScrollBox::Slot()[Choices]]]];
	if (Zone < 0) { return Panel; }
	const auto Names = ACCLWorldEnvironmentConfig::LaboratoryVariants(Zone, Subject);
	auto Variant = MakeShared<int32>(FMath::Clamp(C->LaboratoryVariant, 0, Names.Num() - 1));
	auto Count = MakeShared<int32>(C->SampleCount());
	auto Selector = SNew(SComboButton).ButtonContent()[SNew(STextBlock).Text_Lambda([Variant, Names]() { return FText::FromString(TEXT("시험 종류: ") + Names[*Variant]); })];
	TWeakPtr<SComboButton> WeakSelector = Selector;
	auto Variants = SNew(SVerticalBox);
	for (int32 I = 0; I < Names.Num(); ++I)
	{
		Variants->AddSlot().AutoHeight()[SNew(SButton).Text(FText::FromString(Names[I]))
			.OnClicked_Lambda([Variant, I, WeakSelector]() { *Variant = I; if (auto Button = WeakSelector.Pin()) { Button->SetIsOpen(false); } return FReply::Handled(); })];
	}
	Selector->SetMenuContent(Variants);
	if (Zone == 6 || Zone == 7)
	{
		Panel->AddSlot().AutoHeight().Padding(0, 8)[Selector];
		Panel->AddSlot().AutoHeight()[SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(TEXT("표본 수 (1-16)  ")))]
			+ SHorizontalBox::Slot().FillWidth(1)[SNew(SSpinBox<int32>).MinValue(1).MaxValue(16).Value(*Count)
				.OnValueChanged_Lambda([Count](int32 Value) { *Count = Value; })]];
		Panel->AddSlot().AutoHeight().Padding(0, 8)[SNew(SButton).Text(FText::FromString(TEXT("선택한 시험만 새로 배치")))
			.IsEnabled_Lambda(Enabled)
			.OnClicked_Lambda([this, Zone, Subject, Variant, Count]() { if (auto* PC = Controller()) { PC->ConfigureLaboratory(Zone, Subject, *Variant, *Count); } return FReply::Handled(); })];
	}
	FString Steps = TEXT("1. 종류·수량을 고르고 새로 배치한다.\n2. 배율을 입력하고 적용한다. 0은 정지, 1은 실시간, 60은 빠른 관찰.\n3. F7로 화면을 닫아 표본을 본다. F7로 다시 열면 관측값을 볼 수 있다.\n현재 실험 초기화는 시간과 표본을 시작 상태로 되돌린다. 레벨 이동은 새 실험을 시작한다.\n");
	if (Zone == 6) { Steps += TEXT("먹이를 추가한 뒤 섭취량·에너지를 확인한다. Actor는 개별 몸체, Mass는 구 모양 표시로 위치를 확인한다. 수중 시험은 물 표면을 생략해 내부를 보여 준다. 숨김은 계산을 계속한다."); }
	if (Zone == 7 && Subject == 0) { Steps += TEXT("비·가뭄·겨울 버튼으로 입력을 바꾸고 성장·고사·휴면과 크기 변화를 본다."); }
	if (Zone == 7 && Subject == 1) { Steps += TEXT("첫 표본 수명을 10초로 줄이거나 보호·치명 피해를 적용한다. 나이·체력·사망 원인을 확인한다."); }
	if (Zone == 7 && Subject == 2) { Steps += TEXT("300초 부패 기준으로 신선도·곰팡이를 관찰한다. 수량은 표시된 보관함 안의 음식 개수다. 다른 보관 조건 비교는 종류를 바꿔 새로 배치한다."); }
	Panel->AddSlot().AutoHeight().Padding(0, 6)[SNew(SExpandableArea).InitiallyCollapsed(true)
		.HeaderContent()[SNew(STextBlock).Text(FText::FromString(TEXT("사용법")))]
		.BodyContent()[SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(Steps))]];
	return Panel;
}
