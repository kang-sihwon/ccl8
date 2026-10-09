#include "CCLExperimentScreen.h"

#include "CCLExperimentDirector.h"
#include "CCLTerrainRegion.h"
#include "CCLSurfaceReplication.h"
#include "CCLExperimentPlayerController.h"
#include "CCLWorldEnvironmentState.h"
#include "CCLWorldSimulationSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/NativeWidgetHost.h"
#include "EngineUtils.h"
#include "Misc/EngineVersion.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"

void UCCLExperimentScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	Host = WidgetTree->ConstructWidget<UNativeWidgetHost>();
	WidgetTree->RootWidget = Host;
}

FReply UCCLExperimentScreen::NativeOnFocusReceived(const FGeometry& Geometry, const FFocusEvent& Event)
{
	return FirstButton ? FReply::Handled().SetUserFocus(FirstButton.ToSharedRef(), Event.GetCause()) :
		Super::NativeOnFocusReceived(Geometry, Event);
}

FReply UCCLExperimentScreen::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (Event.GetKey() == EKeys::F8)
	{
		RequestClose();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(Geometry, Event);
}

void UCCLExperimentScreen::OnContextBound()
{
	auto Button = [](const TCHAR* Label, TFunction<void()> Action, TFunction<bool()> Enabled)
	{
		return SNew(SButton).ContentPadding(FMargin(10, 7))
			.IsEnabled_Lambda([Enabled]() { return Enabled(); })
			.OnClicked_Lambda([Action]() { Action(); return FReply::Handled(); })
			[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 17)).Text(FText::FromString(Label))];
	};
	auto Allowed = [this]() { return Director() && Director()->CanOperate(Controller()); };
	auto Action = [this](ECCLExperimentAction Command)
	{
		if (auto* PC = Controller())
		{
			PC->Submit(Command, SelectedCase);
		}
	};
	auto MapButtons = SNew(SHorizontalBox);
	const TPair<const TCHAR*, ECCLExperimentAction> Destinations[] = {
		{TEXT("종합 실험장"), ECCLExperimentAction::TravelHub}, {TEXT("격리 실험장"), ECCLExperimentAction::TravelScenario},
		{TEXT("전투 실험장"), ECCLExperimentAction::TravelCombat}, {TEXT("멀티플레이 실험장"), ECCLExperimentAction::TravelMultiplayer}};
	for (const auto& Destination : Destinations)
	{
		MapButtons->AddSlot().AutoWidth().Padding(0, 0, 8, 0)
			[Button(Destination.Key, [Action, Command = Destination.Value]() { Action(Command); }, Allowed)];
	}

	MapButtons->AddSlot().FillWidth(1);
	MapButtons->AddSlot().AutoWidth()[Button(TEXT("닫기 · F8"), [this]() { RequestClose(); }, []() { return true; })];

	auto ZoneList = SNew(SVerticalBox);
	if (const auto* Current = Director())
	{
		for (const UCCLExperimentDefinition* Definition : Current->Definitions)
		{
			if (!Definition)
			{
				continue;
			}

			const FName Id = Definition->CaseId;
			auto ZoneButton = SNew(SButton).ContentPadding(FMargin(10, 6))
				.ButtonColorAndOpacity_Lambda([this, Id]() { return SelectedCase == Id ? FLinearColor(0.12f, 0.45f, 0.65f) : FLinearColor(0.06f, 0.1f, 0.16f); })
				.OnClicked_Lambda([this, Id]() { SelectedCase = Id; return FReply::Handled(); })
				[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 17)).Text(Definition->Title)];
			ZoneList->AddSlot().AutoHeight().Padding(0, 0, 0, 3)[ZoneButton];
			if (!FirstButton)
			{
				FirstButton = ZoneButton;
			}
		}
	}

	auto Controls = SNew(SHorizontalBox);
	auto Start = Button(TEXT("시작 / 재실행"), [Action]() { Action(ECCLExperimentAction::Start); }, [this, Allowed]()
	{
		FString Reason;
		return Allowed() && Director()->CanStart(SelectedCase, Reason);
	});
	StartButton = Start;
	Controls->AddSlot().AutoWidth().Padding(0, 0, 6, 0)[Start];
	Controls->AddSlot().AutoWidth().Padding(0, 0, 6, 0)[Button(TEXT("중지"), [Action]() { Action(ECCLExperimentAction::Stop); }, [this, Allowed]()
	{
		const auto* Result = Director() ? Director()->FindResult(SelectedCase) : nullptr;
		return Allowed() && Result && Result->Status == ECCLExperimentStatus::Running;
	})];
	Controls->AddSlot().AutoWidth().Padding(0, 0, 6, 0)[Button(TEXT("전체 초기화"), [Action]() { Action(ECCLExperimentAction::Reset); }, Allowed)];
	Controls->AddSlot().AutoWidth()[Button(TEXT("선택 구역으로 이동"), [Action]() { Action(ECCLExperimentAction::Teleport); }, [this]() { return Controller() != nullptr; })];

	auto Storage = SNew(SHorizontalBox);
	Storage->AddSlot().AutoWidth().Padding(0, 0, 6, 0)[Button(TEXT("실험 저장"), [Action]() { Action(ECCLExperimentAction::Save); }, Allowed)];
	Storage->AddSlot().AutoWidth().Padding(0, 0, 16, 0)[Button(TEXT("저장 불러오기"), [Action]() { Action(ECCLExperimentAction::Load); }, Allowed)];
	auto ScaleAllowed = [this, Allowed]()
	{
		return Allowed() && UCCLWorldSimulationSubsystem::DomainForWorld(GetWorld()) == ECCLWorldDomain::Scenario;
	};
	const TPair<const TCHAR*, ECCLExperimentAction> Rates[] = {{TEXT("시간 정지"), ECCLExperimentAction::ScaleZero},
		{TEXT("1배"), ECCLExperimentAction::ScaleOne}, {TEXT("60배"), ECCLExperimentAction::ScaleSixty}};
	for (const auto& Rate : Rates)
	{
		Storage->AddSlot().AutoWidth().Padding(0, 0, 6, 0)[Button(Rate.Key, [Action, Command = Rate.Value]() { Action(Command); }, ScaleAllowed)];
	}

	auto IsEnvironmentCase = [this]() { return SelectedCase == TEXT("Zone_01") || SelectedCase == TEXT("Zone_11"); };
	auto EnvironmentControls = SNew(SVerticalBox).Visibility_Lambda([IsEnvironmentCase]()
	{
		return IsEnvironmentCase() ? EVisibility::Visible : EVisibility::Collapsed;
	});
	EnvironmentControls->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 16)).WrapTextAt(760)
		.Text_Lambda([this]()
		{
			for (TActorIterator<ACCLWorldEnvironmentState> It(GetWorld()); It; ++It)
			{
				const auto& View = It->GetTime().Environment;
				if (!View.bValid)
				{
					return FText::FromString(TEXT("서버 환경 관측 수신 대기"));
				}

				FString Text = FString::Printf(TEXT("위도 %.1f° | 기울기 %.2f° | 입력 %llu | 표면 %llu\n"),
					View.Observer.LatitudeDegrees, View.ObliquityDegrees, View.InputRevision, View.SurfaceRevision);
				for (const auto& Star : View.Stars)
				{
					if (Star.BodyId == View.DominantStarId)
					{
						Text += FString::Printf(TEXT("태양시 %.2f시 | 고도 %.2f° | 수평 일사 %.2f W/m²\n"),
							Star.SolarHours, Star.ElevationDegrees, Star.HorizontalIrradiance);
					}
				}

				if (SelectedCase == TEXT("Zone_11"))
				{
					for (const auto& Probe : View.Probes)
					{
						Text += FString::Printf(TEXT("%s  빛 %.0f%% · 비 %.0f%% · 바람 %.0f%%\n"), *Probe.ProbeId.ToString(),
							100. * Probe.Transmission.Sun, 100. * Probe.Transmission.Precipitation, 100. * Probe.Transmission.Wind);
					}

					if (!View.Openings.IsEmpty())
					{
						Text += FString::Printf(TEXT("문 열림 %.0f%% · 청록/노랑 구체가 진단 위치다."), View.Openings[0].OpenFraction * 100.);
					}
				}
				else
				{
					for (const auto& Body : View.SkyBodies)
					{
						Text += FString::Printf(TEXT("%s  밝은 면 %.1f%% · 각반경 %.3f°\n"), *Body.BodyId.ToString(),
							Body.IlluminatedFraction * 100., Body.AngularRadiusDegrees);
					}

					Text += TEXT("위상 버튼은 초기 천체 요소를 바꾸는 시험이며 NPC 시간을 건너뛰지 않는다.");
				}

				return FText::FromString(Text);
			}

			return FText::FromString(TEXT("서버 관측 수신 대기"));
		})];
	auto CelestialButtons = SNew(SHorizontalBox);
	const TPair<const TCHAR*, ECCLExperimentAction> CelestialActions[] = {
		{TEXT("위도 +45°"), ECCLExperimentAction::NextLatitude}, {TEXT("기울기 변경"), ECCLExperimentAction::NextObliquity},
		{TEXT("자전 위상 +90°"), ECCLExperimentAction::RotateQuarter}, {TEXT("공전 위상 +90°"), ECCLExperimentAction::OrbitQuarter}};
	for (const auto& Entry : CelestialActions)
	{
		auto Control = Button(Entry.Key, [Action, Command = Entry.Value]() { Action(Command); }, ScaleAllowed);
		CelestialButtons->AddSlot().AutoWidth().Padding(0, 0, 5, 0)[Control];
		if (Entry.Value == ECCLExperimentAction::RotateQuarter)
		{
			RotationButton = Control;
		}
	}

	EnvironmentControls->AddSlot().AutoHeight().Padding(0, 0, 0, 8)[CelestialButtons];
	auto Door = Button(TEXT("문: 닫힘 → 반 열림 → 열림"), [Action]() { Action(ECCLExperimentAction::CycleOpening); }, Allowed);
	DoorButton = Door;
	EnvironmentControls->AddSlot().AutoHeight()[Door];

	auto TerrainControls = SNew(SVerticalBox).Visibility_Lambda([this]()
	{
		return SelectedCase == TEXT("Zone_05") ? EVisibility::Visible : EVisibility::Collapsed;
	});
	TerrainControls->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 16)).WrapTextAt(760)
		.Text_Lambda([this]()
		{
			const auto* Region = Director() ? Director()->GetTerrainRegion() : nullptr;
			if (!Region || !Region->IsTerrainReady())
			{
				return FText::FromString(TEXT("지형 수신과 충돌 준비 대기"));
			}

			return FText::FromString(FString::Printf(TEXT("지형 %llu | 배포 %llu | 충돌 %s | AI 경로 %s\n%s"),
				Region->GetTerrainStore().GetRevision(), Region->GetPublicationSerial(),
				!Region->HasAuthority() && !Region->IsReplicaReady() ? TEXT("수신·충돌 대기") : (Region->IsPreparing() ? TEXT("후보 준비 중") : TEXT("준비")),
				Region->HasAuthority() ? (Region->IsNavigationReady() ? TEXT("준비") : TEXT("갱신 대기")) : TEXT("서버 판정"),
				*Region->GetLastError()));
		})];
	auto TerrainButtons = SNew(SHorizontalBox);
	const TPair<const TCHAR*, ECCLExperimentAction> TerrainActions[] = {
		{TEXT("구덩이 굴착"), ECCLExperimentAction::TerrainExcavate}, {TEXT("흙 쌓기"), ECCLExperimentAction::TerrainDeposit},
		{TEXT("수로 연결"), ECCLExperimentAction::TerrainChannel}, {TEXT("보호 구역 확인"), ECCLExperimentAction::TerrainProtection},
		{TEXT("지형 초기화"), ECCLExperimentAction::TerrainReset}};
	for (const auto& Entry : TerrainActions)
	{
		TerrainButtons->AddSlot().AutoWidth().Padding(0, 0, 4, 0)[Button(Entry.Key,
			[Action, Command = Entry.Value]() { Action(Command); }, [this, Allowed]()
			{
				const auto* Region = Director() ? Director()->GetTerrainRegion() : nullptr;
				return Allowed() && Region && Region->IsTerrainReady() && !Region->IsPreparing()
					&& (Region->HasAuthority() || Region->IsReplicaReady());
			})];
	}

	TerrainControls->AddSlot().AutoHeight()[TerrainButtons];

	auto WaterControls = SNew(SVerticalBox).Visibility_Lambda([this]()
	{
		return SelectedCase == TEXT("Zone_04") || SelectedCase == TEXT("Zone_05") ? EVisibility::Visible : EVisibility::Collapsed;
	});
	WaterControls->AddSlot().AutoHeight().Padding(0, 8)
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 16)).WrapTextAt(760).Text_Lambda([this]()
		{
			const auto* Model = UCCLSurfaceReplication::View(GetWorld());
			const auto* G = Model ? Model->FindRegion(SelectedCase == TEXT("Zone_05")
				? ACCLExperimentDirector::TerrainWaterRegionId() : ACCLExperimentDirector::WaterRegionId()) : nullptr;
			if (!G || !UCCLSurfaceReplication::IsGeometryReady(GetWorld(), *Model, *G))
			{
				return FText::FromString(TEXT("서버의 물과 지형을 준비하고 있다."));
			}
			double Liquid = 0., Ice = 0., Soil = 0.;
			for (const auto& C : G->Cells)
			{
				Liquid += C.WaterCubicMeters;
				Ice += C.IceCubicMeters;
				Soil += C.SoilCubicMeters;
			}
			return FText::FromString(FString::Printf(TEXT("물 %.3f · 얼음 %.3f · 토양 수분 %.3f m³\n기온 %.1f°C · 강수 %.1f mm/h · 수지 오차 %.8f m³\n청록: 수면 · 흰색: 얼음 · 짙은 갈색: 진흙"),
				Liquid, Ice, Soil, G->Forcing.TemperatureCelsius, G->Forcing.RainMetersPerWorldSecond * 3600000., G->BalanceErrorCubicMeters()));
		})];
	auto WaterAllowed = [this, Allowed]()
	{
		FString Reason;
		return Allowed() && Director()->CanStart(SelectedCase, Reason);
	};
	auto WaterButtons = SNew(SHorizontalBox);
	const TPair<const TCHAR*, ECCLExperimentAction> WaterActions[] = {
		{TEXT("비 켜기 / 끄기"), ECCLExperimentAction::WaterRain}, {TEXT("결빙"), ECCLExperimentAction::WaterFreeze},
		{TEXT("융해"), ECCLExperimentAction::WaterThaw}, {TEXT("건조"), ECCLExperimentAction::WaterDry}};
	for (const auto& Command : WaterActions)
	{
		WaterButtons->AddSlot().AutoWidth().Padding(0, 0, 6, 0)
			[Button(Command.Key, [Action, Value = Command.Value]() { Action(Value); }, WaterAllowed)];
	}
	WaterControls->AddSlot().AutoHeight()[WaterButtons];

	auto SnowControls = SNew(SHorizontalBox).Visibility_Lambda([this]()
	{
		return SelectedCase == TEXT("Zone_03") ? EVisibility::Visible : EVisibility::Collapsed;
	});
	for (const auto& Entry : TArray<TPair<FString, ECCLExperimentAction>>{
		{TEXT("새 적설 켜기 / 끄기"), ECCLExperimentAction::SnowFall}, {TEXT("융해"), ECCLExperimentAction::SnowMelt}})
	{
		SnowControls->AddSlot().AutoWidth().Padding(0, 0, 5, 0)[Button(*Entry.Key,
			[Action, Value = Entry.Value]() { Action(Value); }, [this, Allowed]()
			{
				FString Why;
				return Allowed() && Director() && Director()->CanStart(SelectedCase, Why);
			})];
	}

	auto Detail = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 26)).Text_Lambda([this]()
		{
			const auto* Definition = Director() ? Director()->FindDefinition(SelectedCase) : nullptr;
			return Definition ? Definition->Title : FText::FromString(TEXT("구역을 선택해 줘."));
		})]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 19)).ColorAndOpacity(FLinearColor(0.45f, 0.8f, 1))
		.Text_Lambda([this]()
		{
			const auto* Result = Director() ? Director()->FindResult(SelectedCase) : nullptr;
			return FText::FromString(TEXT("상태: ") + UCCLExperimentDefinition::StatusText(Result ? Result->Status : ECCLExperimentStatus::NotImplemented));
		})]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
		[SNew(STextBlock).Visibility_Lambda([IsEnvironmentCase]() { return IsEnvironmentCase() ? EVisibility::Collapsed : EVisibility::Visible; })
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).WrapTextAt(780).Text_Lambda([this]()
		{
			const auto* Definition = Director() ? Director()->FindDefinition(SelectedCase) : nullptr;
			return FText::FromString(Definition ? TEXT("조작: ") + Definition->Instructions.ToString() + TEXT("\n\n예상 결과: ") + Definition->Expected.ToString() : FString());
		})]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
		[SNew(STextBlock).Visibility_Lambda([IsEnvironmentCase]() { return IsEnvironmentCase() ? EVisibility::Collapsed : EVisibility::Visible; })
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 17)).WrapTextAt(780).Text_Lambda([this]()
		{
			const auto* Result = Director() ? Director()->FindResult(SelectedCase) : nullptr;
			if (!Result)
			{
				return FText::FromString(TEXT("서버 상태를 기다리고 있다."));
			}

			return FText::FromString(FString::Printf(TEXT("결과: %s\n완료 세계 시각: %.2f초\n실행 ID: %s"), *Result->Detail,
				Result->CompletedWorldSeconds, Result->RunId.IsValid() ? *Result->RunId.ToString().Left(8) : TEXT("미실행")));
		})]
		+ SVerticalBox::Slot().AutoHeight()[EnvironmentControls]
		+ SVerticalBox::Slot().AutoHeight()[TerrainControls]
		+ SVerticalBox::Slot().AutoHeight()[WaterControls]
		+ SVerticalBox::Slot().AutoHeight()[SnowControls]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 10)[Controls]
		+ SVerticalBox::Slot().AutoHeight()[Storage];

	Host->SetContent(SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.025f, 0.045f, 0.075f, 0.97f)).Padding(20).HAlign(HAlign_Center).VAlign(VAlign_Center)
		[SNew(SBox).WidthOverride(1160).HeightOverride(720)
		[SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
			[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 30)).Text(FText::FromString(TEXT("자연 환경 실험장"))) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
			[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 16)).Text_Lambda([this]()
			{
				FString TimeText = TEXT("공통 시계 수신 대기");
				for (TActorIterator<ACCLWorldEnvironmentState> It(GetWorld()); It; ++It)
				{
					const auto& Time = It->GetTime();
					TimeText = FString::Printf(TEXT("게임 %.2f초 | 세계 %.2f초 | %.0f배 | 대기 %.2f초"),
						Time.GameSeconds, Time.WorldSeconds, Time.TimeScale, Time.PendingGameSeconds);
					break;
				}

				const auto* Setup = Director() ? Director()->FindDefinition(TEXT("Zone_00")) : nullptr;
				TimeText = FString::Printf(TEXT("UE %d.%d.%d | Seed %d | "), FEngineVersion::Current().GetMajor(),
					FEngineVersion::Current().GetMinor(), FEngineVersion::Current().GetPatch(), Setup ? Setup->Seed : 0) + TimeText;
				return FText::FromString(TimeText + (Director() && Director()->CanOperate(Controller()) ? TEXT(" | 조작 담당자") : TEXT(" | 관찰 참가자")));
			})]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)[MapButtons]
			+ SVerticalBox::Slot().FillHeight(1)
			[SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 20, 0)[SNew(SBox).WidthOverride(290)[SNew(SScrollBox) + SScrollBox::Slot()[ZoneList]]]
				+ SHorizontalBox::Slot().FillWidth(1)[Detail]]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 16, 0, 6)
			[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 16)).Text_Lambda([this]()
			{
				return FText::FromString(Controller() ? Controller()->GetExperimentMessage() : FString());
			})]
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 15)).Text(FText::FromString(
				TEXT("F8: 실험 화면 열기/닫기 · WASD: 이동 · 미구현 구역은 실행 불가 · 캠페인 저장과 분리")))]]]);
}

void UCCLExperimentScreen::OnContextReleased()
{
	if (Host)
	{
		Host->SetContent(SNullWidget::NullWidget);
	}

	FirstButton.Reset();
	StartButton.Reset();
	RotationButton.Reset();
	DoorButton.Reset();
}

ACCLExperimentPlayerController* UCCLExperimentScreen::Controller() const
{
	const auto* Context = Cast<UCCLExperimentContext>(GetContext());
	return Context ? Context->Controller.Get() : nullptr;
}

ACCLExperimentDirector* UCCLExperimentScreen::Director() const
{
	return GetWorld() ? ACCLExperimentDirector::Find(GetWorld()) : nullptr;
}
