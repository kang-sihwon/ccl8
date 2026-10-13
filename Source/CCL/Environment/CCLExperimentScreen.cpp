#include "CCLExperimentScreen.h"
#include "CCLFoodExperiment.h"
#include "CCLBiologyExperiment.h"
#include "CCLEcologyExperiment.h"
#include "SCCLCelestialDebugView.h"
#include "Widgets/SOverlay.h"

#include "CCLExperimentDirector.h"
#include "CCLTerrainRegion.h"
#include "CCLSurfaceReplication.h"
#include "CCLExperimentPlayerController.h"
#include "CCLWorldEnvironmentState.h"
#include "CCLWorldSimulationSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/NativeWidgetHost.h"
#include "EngineUtils.h"
#include "Misc/EngineVersion.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SViewport.h"
#include "Framework/Application/SlateApplication.h"
#include <charconv>

namespace
{
	bool ParseTimeScale(const FText& Text, double& Scale)
	{
		const FString Value = Text.ToString().TrimStartAndEnd();
		const FTCHARToUTF8 Utf8(*Value);
		const char* Begin = Utf8.Get();
		const char* End = Begin + Utf8.Length();
		const auto Result = std::from_chars(Begin, End, Scale);
		return Result.ec == std::errc() && Result.ptr == End && FMath::IsFinite(Scale) && Scale >= 0.;
	}

	FString FormatElapsedTime(double Seconds)
	{
		if (!FMath::IsFinite(Seconds) || Seconds < 0.)
		{
			return TEXT("확인 불가");
		}

		const double WholeSeconds = FMath::FloorToDouble(Seconds);
		const double Hours = FMath::FloorToDouble(WholeSeconds / 3600.);
		const int32 Minutes = static_cast<int32>(FMath::Fmod(WholeSeconds, 3600.) / 60.);
		const int32 RemainingSeconds = static_cast<int32>(FMath::Fmod(WholeSeconds, 60.));
		return FString::Printf(TEXT("%02.0f시 %02d분 %02d초"), Hours, Minutes, RemainingSeconds);
	}
}

void UCCLExperimentScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	Host = WidgetTree->ConstructWidget<UNativeWidgetHost>();
	WidgetTree->RootWidget = Host;
}

FReply UCCLExperimentScreen::NativeOnFocusReceived(const FGeometry& Geometry, const FFocusEvent& Event)
{
	return CloseButton ? FReply::Handled().SetUserFocus(CloseButton.ToSharedRef(), Event.GetCause()) :
		Super::NativeOnFocusReceived(Geometry, Event);
}

FReply UCCLExperimentScreen::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (Event.GetKey() == EKeys::F7)
	{
		if (!Event.IsRepeat())
		{
			RequestClose();
		}

		return FReply::Handled();
	}

	Super::NativeOnKeyDown(Geometry, Event);
	return FReply::Handled();
}

FReply UCCLExperimentScreen::NativeOnKeyUp(const FGeometry& Geometry, const FKeyEvent& Event)
{
	Super::NativeOnKeyUp(Geometry, Event);
	return FReply::Handled();
}

void UCCLExperimentScreen::OnManagedTick(float DeltaTime)
{
	Super::OnManagedTick(DeltaTime);
	RefreshScaleInputs();
	const auto Viewport = FSlateApplication::Get().GetGameViewport();
	if (IsActivated() && GetIsEnabled() && Viewport && Viewport->HasKeyboardFocus())
	{
		RequestRefreshFocus();
	}
}

FReply UCCLExperimentScreen::NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (auto* PC = Controller())
	{
		FVector2D PixelPosition, ViewportPosition;
		USlateBlueprintLibrary::AbsoluteToViewport(this, Event.GetScreenSpacePosition(), PixelPosition, ViewportPosition);
		PC->SetTerrainCursor(PixelPosition);
	}
	return Super::NativeOnMouseMove(Geometry, Event);
}

FReply UCCLExperimentScreen::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	NativeOnMouseMove(Geometry, Event);
	if (auto* PC = Controller(); PC && PC->HasTerrainTool() && MenuPanel
		&& !IsObserving()
		&& !MenuPanel->GetCachedGeometry().IsUnderLocation(Event.GetScreenSpacePosition()))
	{
		if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			PC->ApplyTerrainTool();
			return FReply::Handled();
		}
		if (Event.GetEffectingButton() == EKeys::RightMouseButton)
		{
			PC->CancelTerrainTool();
			return FReply::Handled();
		}
	}

	return FReply::Handled().SetUserFocus(TakeWidget(), EFocusCause::Mouse);
}

bool UCCLExperimentScreen::IsObserving() const
{
	return bShowCelestialView && SelectedCase == TEXT("Zone_01") && CelestialView.IsValid();
}

double UCCLExperimentScreen::CurrentScale() const
{
	if (IsObserving())
	{
		const auto Preview = StaticCastSharedPtr<SCCLCelestialDebugView>(CelestialView);
		if (!Preview->IsFollowingWorldTime())
		{
			return Preview->GetObservationScale();
		}
	}
	for (TActorIterator<ACCLWorldEnvironmentState> It(GetWorld()); It; ++It)
	{
		return It->GetTime().TimeScale;
	}
	return 0.;
}

void UCCLExperimentScreen::RefreshScaleInputs(bool bForce)
{
	const double Scale = CurrentScale();
	const bool bObservation = IsObserving();
	if (!bForce && DisplayedScale == Scale && bDisplayedObservation == bObservation)
	{
		return;
	}
	DisplayedScale = Scale;
	bDisplayedObservation = bObservation;
	const FText Text = FText::FromString(FString::Printf(TEXT("%.17g"), Scale));
	for (const auto& Input : {ScaleInput, ObservationScaleInput})
	{
		if (Input)
		{
			Input->SetText(Text);
			Input->SetError(FText::GetEmpty());
		}
	}
}

void UCCLExperimentScreen::ApplyScale(double Scale)
{
	if (IsObserving())
	{
		if (!StaticCastSharedPtr<SCCLCelestialDebugView>(CelestialView)->SetObservationScale(Scale))
		{
			const FText Error = FText::FromString(TEXT("천체 정의 수신을 기다려 줘."));
			ScaleInput->SetError(Error);
			ObservationScaleInput->SetError(Error);
			return;
		}
	}
	else if (auto* PC = Controller())
	{
		PC->Submit(ECCLExperimentAction::ScaleCustom, SelectedCase, Scale);
	}
	RefreshScaleInputs(true);
}

void UCCLExperimentScreen::AdjustScale(double Delta, bool bStop)
{
	if (IsObserving())
	{
		ApplyScale(bStop ? 0. : FMath::Max(0., CurrentScale() + Delta));
	}
	else if (auto* PC = Controller())
	{
		PC->Submit(bStop ? ECCLExperimentAction::ScaleZero :
			(Delta > 0. ? ECCLExperimentAction::ScaleIncrease : ECCLExperimentAction::ScaleDecrease), SelectedCase);
		RefreshScaleInputs(true);
	}
}

void UCCLExperimentScreen::ApplyTimeScale()
{
	if (!ScaleInput || !ScaleInput->IsEnabled())
	{
		return;
	}
	double Scale = 0.;
	if (!ParseTimeScale(ScaleInput->GetText(), Scale))
	{
		ScaleInput->SetError(FText::FromString(TEXT("0 이상의 숫자를 입력해 줘. 예: 60, 6000, 600000")));
		return;
	}
	ScaleInput->SetError(FText::GetEmpty());
	ApplyScale(Scale);
}

void UCCLExperimentScreen::ApplyObservationScale()
{
	if (!ObservationScaleInput || !IsObserving())
	{
		return;
	}
	double Scale = 0.;
	if (!ParseTimeScale(ObservationScaleInput->GetText(), Scale))
	{
		ObservationScaleInput->SetError(FText::FromString(TEXT("0 이상의 숫자를 입력해 줘. 예: 600000, 1e10")));
		return;
	}
	ApplyScale(Scale);
}

void UCCLExperimentScreen::OnContextBound()
{
	SetVisibility(ESlateVisibility::Visible);
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
			if (Command >= ECCLExperimentAction::TerrainExcavate && Command <= ECCLExperimentAction::TerrainChannel)
			{
				PC->SelectTerrainTool(Command);
			}
			else
			{
				PC->Submit(Command, SelectedCase);
			}
		}
	};
	auto MapButtons = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(4, 4));
	const TPair<const TCHAR*, ECCLExperimentAction> Destinations[] = {
		{TEXT("종합 실험장"), ECCLExperimentAction::TravelHub}, {TEXT("격리 실험장"), ECCLExperimentAction::TravelScenario},
		{TEXT("전투 실험장"), ECCLExperimentAction::TravelCombat}, {TEXT("멀티플레이 실험장"), ECCLExperimentAction::TravelMultiplayer}};
	for (const auto& Destination : Destinations)
	{
		MapButtons->AddSlot().Padding(0, 0, 8, 0)
			[Button(Destination.Key, [Action, Command = Destination.Value]() { Action(Command); }, Allowed)];
	}

	CloseButton = Button(TEXT("닫기 · F7"), [this]() { RequestClose(); }, []() { return true; });


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
				.OnClicked_Lambda([this, Id]() { SelectCase(Id); ZoneSelector->SetIsOpen(false); return FReply::Handled(); })
				[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 17)).Text(Definition->Title)];
			ZoneList->AddSlot().AutoHeight().Padding(0, 0, 0, 3)[ZoneButton];
			if (!FirstButton)
			{
				FirstButton = ZoneButton;
			}
		}
	}

	auto Controls = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(4, 4));
	auto Start = Button(TEXT("자동 검사"), [Action]() { Action(ECCLExperimentAction::Start); }, [this, Allowed]()
	{
		FString Reason;
		return Allowed() && Director()->CanStart(SelectedCase, Reason);
	});
	StartButton = Start;
	Controls->AddSlot().Padding(0, 0, 6, 0)[Start];
	Controls->AddSlot().Padding(0, 0, 6, 0)[Button(TEXT("중지"), [Action]() { Action(ECCLExperimentAction::Stop); }, [this, Allowed]()
	{
		const auto* Result = Director() ? Director()->FindResult(SelectedCase) : nullptr;
		return Allowed() && Result && Result->Status == ECCLExperimentStatus::Running;
	})];
	Controls->AddSlot().Padding(0, 0, 6, 0)[Button(TEXT("전체 초기화"), [Action]() { Action(ECCLExperimentAction::Reset); }, Allowed)];
	Controls->AddSlot()[Button(TEXT("선택 구역으로 이동"), [Action]() { Action(ECCLExperimentAction::Teleport); }, [this]() { return Controller() != nullptr; })];

	auto Storage = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(4, 4));
	Storage->AddSlot().Padding(0, 0, 6, 0)[Button(TEXT("실험 저장"), [Action]() { Action(ECCLExperimentAction::Save); }, Allowed)];
	Storage->AddSlot().Padding(0, 0, 16, 0)[Button(TEXT("저장 불러오기"), [Action]() { Action(ECCLExperimentAction::Load); }, Allowed)];
	auto ScaleAllowed = [this, Allowed]()
	{
		return Allowed() && UCCLWorldSimulationSubsystem::DomainForWorld(GetWorld()) == ECCLWorldDomain::Scenario;
	};
	auto TimeControlAllowed = [this, ScaleAllowed]() { return IsObserving() || ScaleAllowed(); };
	ScaleStopButton = Button(TEXT("시간 정지"), [this]() { AdjustScale(0., true); }, TimeControlAllowed);
	ScaleDecreaseButton = Button(TEXT("60배 감소"), [this]() { AdjustScale(-60.); }, TimeControlAllowed);
	ScaleIncreaseButton = Button(TEXT("60배 증가"), [this]() { AdjustScale(60.); }, TimeControlAllowed);
	Storage->AddSlot().Padding(0, 0, 6, 0)[ScaleStopButton.ToSharedRef()];
	Storage->AddSlot().Padding(0, 0, 6, 0)[ScaleDecreaseButton.ToSharedRef()];
	Storage->AddSlot().Padding(0, 0, 6, 0)[ScaleIncreaseButton.ToSharedRef()];
	double InitialScale = 60.;
	for (TActorIterator<ACCLWorldEnvironmentState> It(GetWorld()); It; ++It)
	{
		InitialScale = It->GetTime().TimeScale;
		break;
	}
	ScaleInput = SNew(SEditableTextBox)
		.ClearKeyboardFocusOnCommit(false)
		.MinDesiredWidth(140.f)
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 17))
		.Text(FText::FromString(FString::SanitizeFloat(InitialScale)))
		.HintText(FText::FromString(TEXT("시간 배율")))
		.ToolTipText(FText::FromString(TEXT("천체창이 열려 있으면 관측 배율, 닫혀 있으면 세계 배율. 0은 정지.")))
		.IsEnabled_Lambda(TimeControlAllowed)
		.OnTextCommitted_Lambda([this](const FText&, ETextCommit::Type Commit)
		{
			if (Commit == ETextCommit::OnEnter)
			{
				ApplyTimeScale();
			}
		});
	ScaleApplyButton = Button(TEXT("배율 적용"), [this]() { ApplyTimeScale(); }, TimeControlAllowed);
	Storage->AddSlot().VAlign(VAlign_Center).Padding(0, 0, 6, 0)
		[SNew(STextBlock).Text_Lambda([this]() { return FText::FromString(IsObserving() ? TEXT("조작 대상: 천체 관측") : TEXT("조작 대상: 세계 시간")); })];
	Storage->AddSlot().Padding(0, 0, 6, 0)[ScaleInput.ToSharedRef()];
	Storage->AddSlot()[ScaleApplyButton.ToSharedRef()];

	auto IsEnvironmentCase = [this]() { return SelectedCase == TEXT("Zone_01") || SelectedCase == TEXT("Zone_11"); };
	auto EnvironmentControls = SNew(SVerticalBox).Visibility_Lambda([IsEnvironmentCase]()
	{
		return IsEnvironmentCase() ? EVisibility::Visible : EVisibility::Collapsed;
	});
	EnvironmentControls->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 16)).AutoWrapText(true)
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
						Text += FString::Printf(TEXT("문 열림 %.0f%%\n노랑: 빛 · 파랑: 비 · 청록: 바람\n방향별 차폐율과 아래의 실내 상태를 함께 비교한다."), View.Openings[0].OpenFraction * 100.);
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
	auto CelestialButtons = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(4, 4))
		.Visibility_Lambda([this]() { return SelectedCase == TEXT("Zone_01") ? EVisibility::Visible : EVisibility::Collapsed; });
	CelestialViewButton = Button(TEXT("천체 3D 관측 열기 / 닫기"), [this]()
	{
		bShowCelestialView = !bShowCelestialView;
		StaticCastSharedPtr<SCCLCelestialDebugView>(CelestialView)->FollowWorldTime();
		RefreshScaleInputs(true);
	}, []() { return true; });
	CelestialButtons->AddSlot()[CelestialViewButton.ToSharedRef()];
	const TPair<const TCHAR*, ECCLExperimentAction> CelestialActions[] = {
		{TEXT("위도 +45°"), ECCLExperimentAction::NextLatitude}, {TEXT("기울기 변경"), ECCLExperimentAction::NextObliquity},
		{TEXT("자전 위상 +90°"), ECCLExperimentAction::RotateQuarter}, {TEXT("공전 위상 +90°"), ECCLExperimentAction::OrbitQuarter}};
	for (const auto& Entry : CelestialActions)
	{
		auto Control = Button(Entry.Key, [Action, Command = Entry.Value]() { Action(Command); }, ScaleAllowed);
		CelestialButtons->AddSlot().Padding(0, 0, 5, 0)[Control];
		if (Entry.Value == ECCLExperimentAction::RotateQuarter)
		{
			RotationButton = Control;
		}
	}

	EnvironmentControls->AddSlot().AutoHeight().Padding(0, 0, 0, 8)[CelestialButtons];
	auto Door = Button(TEXT("문: 닫힘 → 반 열림 → 열림"), [Action]() { Action(ECCLExperimentAction::CycleOpening); }, Allowed);
	Door->SetVisibility(TAttribute<EVisibility>::CreateLambda([this]() { return SelectedCase == TEXT("Zone_11") ? EVisibility::Visible : EVisibility::Collapsed; }));
	DoorButton = Door;
	EnvironmentControls->AddSlot().AutoHeight()[Door];

	auto TerrainControls = SNew(SVerticalBox).Visibility_Lambda([this]()
	{
		return SelectedCase == TEXT("Zone_05") ? EVisibility::Visible : EVisibility::Collapsed;
	});
	TerrainControls->AddSlot().AutoHeight().Padding(0, 0, 0, 8)
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 16)).AutoWrapText(true)
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
	auto TerrainButtons = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(4, 4));
	const TPair<const TCHAR*, ECCLExperimentAction> TerrainActions[] = {
		{TEXT("구덩이 굴착"), ECCLExperimentAction::TerrainExcavate}, {TEXT("흙 쌓기"), ECCLExperimentAction::TerrainDeposit},
		{TEXT("수로 파기"), ECCLExperimentAction::TerrainChannel}, {TEXT("보호 구역 확인"), ECCLExperimentAction::TerrainProtection},
		{TEXT("지형 초기화"), ECCLExperimentAction::TerrainReset}};
	for (const auto& Entry : TerrainActions)
	{
		TerrainButtons->AddSlot().Padding(0, 0, 4, 0)[Button(Entry.Key,
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
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 16)).AutoWrapText(true).Text_Lambda([this]()
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
	auto WaterButtons = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(4, 4));
	const TPair<const TCHAR*, ECCLExperimentAction> WaterActions[] = {
		{TEXT("비 켜기 / 끄기"), ECCLExperimentAction::WaterRain}, {TEXT("결빙"), ECCLExperimentAction::WaterFreeze},
		{TEXT("융해"), ECCLExperimentAction::WaterThaw}, {TEXT("건조"), ECCLExperimentAction::WaterDry}};
	for (const auto& Command : WaterActions)
	{
		WaterButtons->AddSlot().Padding(0, 0, 6, 0)
			[Button(Command.Key, [Action, Value = Command.Value]() { Action(Value); }, WaterAllowed)];
	}
	WaterControls->AddSlot().AutoHeight()[WaterButtons];

	auto SnowControls = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(4, 4)).Visibility_Lambda([this]()
	{
		return SelectedCase == TEXT("Zone_03") ? EVisibility::Visible : EVisibility::Collapsed;
	});
	for (const auto& Entry : TArray<TPair<FString, ECCLExperimentAction>>{
		{TEXT("새 적설 켜기 / 끄기"), ECCLExperimentAction::SnowFall}, {TEXT("융해"), ECCLExperimentAction::SnowMelt}})
	{
		SnowControls->AddSlot().Padding(0, 0, 5, 0)[Button(*Entry.Key,
			[Action, Value = Entry.Value]() { Action(Value); }, [this, Allowed]()
			{
				FString Why;
				return Allowed() && Director() && Director()->CanStart(SelectedCase, Why);
			})];
	}

	auto WeatherControls = SNew(SVerticalBox).Visibility_Lambda([this]()
	{
		return SelectedCase == TEXT("Zone_02") ? EVisibility::Visible : EVisibility::Collapsed;
	});
	WeatherControls->AddSlot().AutoHeight().Padding(0, 8)
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 16)).AutoWrapText(true).Text_Lambda([this]()
		{
			const auto* Model = UCCLSurfaceReplication::View(GetWorld());
			const auto* G = Model ? Model->FindRegion(ACCLExperimentDirector::WeatherRegionId()) : nullptr;
			if (!G)
			{
				return FText::FromString(TEXT("날씨 상태 수신 대기"));
			}
			return FText::FromString(FString::Printf(TEXT("%s · 기온 %.1f°C · 바람 %.1f m/s\n수증기 %.2f · 구름 %.2f mm (물 환산)\n비 %.2f · 눈 %.2f mm/h · 일조 %.0f W/m²\n수지 오차 %.8f m³"),
				G->Climate.bEnabled ? TEXT("자연 진행") : TEXT("강제 입력 · 대기 상태 정지"), G->Forcing.TemperatureCelsius,
				G->Weather.WindMetersPerSecond, G->Weather.VaporMeters * 1000., G->Weather.CloudMeters * 1000.,
				G->Forcing.RainMetersPerWorldSecond * 3600000., G->Forcing.SnowMetersPerWorldSecond * 3600000.,
				G->Weather.IrradianceWattsPerM2, G->BalanceErrorCubicMeters()));
		})];
	auto WeatherButtons = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(4, 4));
	for (const auto& Entry : TArray<TPair<FString, ECCLExperimentAction>>{
		{TEXT("자연 진행"), ECCLExperimentAction::WeatherNatural}, {TEXT("강제 비"), ECCLExperimentAction::WeatherRain},
		{TEXT("강제 눈"), ECCLExperimentAction::WeatherSnow}})
	{
		WeatherButtons->AddSlot()[Button(*Entry.Key, [Action, Value = Entry.Value]() { Action(Value); }, WaterAllowed)];
	}
	WeatherControls->AddSlot().AutoHeight()[WeatherButtons];

	auto EcologyControls=SNew(SVerticalBox).Visibility_Lambda([this]() { return SelectedCase==TEXT("Zone_06")?EVisibility::Visible:EVisibility::Collapsed; });
	EcologyPanel=EcologyControls;
	EcologyControls->AddSlot().AutoHeight().Padding(0,8)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",15)).AutoWrapText(true).Text_Lambda([this]()
	{
		const auto* E=ACCLEcologyExperiment::Find(GetWorld());
		if (!E||E->GetView().Rows.Num()!=4)
		{
			return FText::FromString(TEXT("생태 관측 수신 대기"));
		}

		FString Text=TEXT("집계도 먹이를 소비한다. 지속 개체는 숨겨도 이동·섭취한다.\n");
		for (int32 N=0;N<4;++N)
		{
			const auto& R=E->GetView().Rows[N];
			Text+=FString::Printf(TEXT("%s · 전체 %lld / 집계 %lld / 예약 %d\nActor %d · Mass %d · 막힘 %d\n에너지 개체 %.0f%% · 집계 %.0f%%\n먹이 %.1f · 섭취 개체 %.1f / 집계 %.1f\n"),
				*ACCLEcologyExperiment::MediumName(N),R.Living,R.Aggregate,R.Reserved,R.Actors,R.Mass,R.Blocked,
				R.Energy*100.,R.AggregateEnergy*100.,R.Food,R.Consumed,R.AggregateConsumed);
		}
		return FText::FromString(Text);
	})];
	auto EcologyButtons=SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(4,4));
	for (const auto& Entry:TArray<TPair<FString,ECCLExperimentAction>>{
		{TEXT("지상 먹이 +10"),ECCLExperimentAction::EcologyFeedGround},{TEXT("물고기 먹이 +10"),ECCLExperimentAction::EcologyFeedWater},
		{TEXT("새 먹이 +10"),ECCLExperimentAction::EcologyFeedAir},{TEXT("곤충 먹이 +10"),ECCLExperimentAction::EcologyFeedInsect},
		{TEXT("Actor 표현"),ECCLExperimentAction::EcologyActors},{TEXT("Mass 표현"),ECCLExperimentAction::EcologyMass},{TEXT("표현 숨김"),ECCLExperimentAction::EcologyHidden}})
	{
		auto Control=Button(*Entry.Key,[Action,Value=Entry.Value]() { Action(Value); },WaterAllowed);
		EcologyButtons->AddSlot()[Control];
		if (Entry.Value==ECCLExperimentAction::EcologyFeedGround)
		{
			EcologyFeedButton=Control;
		}
	}
	EcologyControls->AddSlot().AutoHeight()[EcologyButtons];
	auto VegetationControls = SNew(SVerticalBox).Visibility_Lambda([this]()
	{
		return SelectedCase == TEXT("Zone_07") ? EVisibility::Visible : EVisibility::Collapsed;
	});
	VegetationControls->AddSlot().AutoHeight().Padding(0, 8)
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 15)).AutoWrapText(true).Text_Lambda([this]()
		{
			const auto* Model = UCCLSurfaceReplication::View(GetWorld());
			const auto* G = Model ? Model->FindRegion(ACCLExperimentDirector::VegetationRegionId()) : nullptr;
			if (!G)
			{
				return FText::FromString(TEXT("식생 수신 대기. 이전 저장에 배치가 없으면 실험 초기화를 사용해 줘."));
			}
			int32 Alive = 0, Dormant = 0, Dead = 0;
			uint64 Regrowth = 0;
			double Living = 0., Litter = 0., Burned = 0.;
			for (const auto& C : G->Vegetation.Cells)
			{
				Alive += C.State == EKSIVegetationState::Growing || C.State == EKSIVegetationState::Mature;
				Dormant += C.State == EKSIVegetationState::Dormant;
				Dead += C.State == EKSIVegetationState::Dead;
				Regrowth += C.RegrowthCount;
				Living += C.LivingBiomassKg;
				Litter += C.DeadBiomassKg;
				Burned += C.BurnedBiomassKg;
			}
			return FText::FromString(FString::Printf(TEXT("성장 %d · 휴면 %d · 고사 %d · 재생 %llu회\n생체 %.2f · 고사체 %.2f · 탄 양 %.2f kg\n기온 %.0f°C · 비 %.0f mm/h\n9종: 나무·풀·꽃·관목·이끼·양치·조류·수생·버섯\n세 줄: 채광 / 그늘 / 양분 부족\n자동 검사: 식생·저장. 음식·Agent 생애는 아래 버튼으로 수동 조작."),
				Alive, Dormant, Dead, Regrowth, Living, Litter, Burned, G->Forcing.TemperatureCelsius,
				G->Forcing.RainMetersPerWorldSecond * 3600000.));
		})];
	auto VegetationButtons = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(4, 4));
	for (const auto& Entry : TArray<TPair<FString, ECCLExperimentAction>>{
		{TEXT("식생에 비"), ECCLExperimentAction::VegetationRain}, {TEXT("가뭄"), ECCLExperimentAction::VegetationDry},
		{TEXT("겨울 휴면"), ECCLExperimentAction::VegetationWinter}, {TEXT("생장 기온"), ECCLExperimentAction::VegetationGrow},
		{TEXT("첫 나무 발화"), ECCLExperimentAction::VegetationIgnite}})
	{
		VegetationButtons->AddSlot()[Button(*Entry.Key, [Action, Value = Entry.Value]() { Action(Value); }, WaterAllowed)];
	}
	VegetationControls->AddSlot().AutoHeight()[VegetationButtons];
	auto BiologyControls = SNew(SVerticalBox).Visibility_Lambda([this]()
	{
		return SelectedCase == TEXT("Zone_07") ? EVisibility::Visible : EVisibility::Collapsed;
	});
	BiologyPanel = BiologyControls;
	auto BiologyButtons = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(4, 4));
	for (const auto& Entry : TArray<TPair<FString, ECCLExperimentAction>>{
		{TEXT("일반 주민: 남은 수명 10초"), ECCLExperimentAction::BiologyAge},
		{TEXT("보호 주민: 노화 보호 전환"), ECCLExperimentAction::BiologyProtect},
		{TEXT("보호 주민: 치명 피해"), ECCLExperimentAction::BiologyDamage}})
	{
		auto Control = Button(*Entry.Key, [Action, Value = Entry.Value]() { Action(Value); }, WaterAllowed);
		BiologyButtons->AddSlot()[Control];
		if (Entry.Value == ECCLExperimentAction::BiologyAge)
		{
			BiologyAgeButton = Control;
		}

		if (Entry.Value == ECCLExperimentAction::BiologyDamage)
		{
			BiologyDamageButton = Control;
		}
	}
	BiologyControls->AddSlot().AutoHeight()[BiologyButtons];
	BiologyControls->AddSlot().AutoHeight().Padding(0, 4)[SNew(STextBlock).AutoWrapText(true).Text_Lambda([this]()
	{
		const auto* Biology = ACCLBiologyExperiment::Find(GetWorld());
		if (!Biology || Biology->GetView().Rows.Num() != 2)
		{
			return FText::FromString(TEXT("생애 관측 수신 대기"));
		}

		FString Text = TEXT("Agent 생애 비교 · 시험 수명 300초\n");
		for (int32 Index = 0; Index < 2; ++Index)
		{
			const auto& Row = Biology->GetView().Rows[Index];
			if (!Row.bRecorded)
			{
				Text += TEXT("표본 없음 · 전체 초기화 필요\n");
				continue;
			}

			Text += FString::Printf(TEXT("%s · %s · 체력 %.0f\n나이 %.1f초 / 수명 %.1f초 · 노화 보호 %s\n"),
				Index ? TEXT("보호 주민") : TEXT("일반 주민"), Row.DeathCause == 1 ? TEXT("수명 종료") : Row.DeathCause == 2 ? TEXT("외부 사망") : TEXT("생존"),
				Row.Health, Row.Age, Row.Lifespan, Row.bProtected ? TEXT("켜짐") : TEXT("꺼짐"));
		}
		return FText::FromString(Text + TEXT("사망 뒤 나이는 멈춘다. 자연 노화 보호는 피해를 막지 않는다."));
	})];
	auto FoodControls = SNew(SVerticalBox).Visibility_Lambda([this]()
	{
		return SelectedCase == TEXT("Zone_07") ? EVisibility::Visible : EVisibility::Collapsed;
	});
	FoodPanel = FoodControls;
	FoodControls->AddSlot().AutoHeight().Padding(0, 4)
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 15)).AutoWrapText(true).Text_Lambda([this]()
		{
			const auto* Food = ACCLFoodExperiment::Find(GetWorld());
			if (!Food || Food->GetView().Rows.Num() != 3)
			{
				return FText::FromString(TEXT("음식 관측 수신 대기"));
			}
			FString Text = TEXT("음식 보관 비교 · 실험용 부패 기준 300초\n");
			for (int32 Index = 0; Index < 3; ++Index)
			{
				const auto& Row = Food->GetView().Rows[Index];
				Text += FString::Printf(TEXT("%s · %s %.1f°C / 습도 %.0f%%\n%d개/%d묶음 · 나이 %.0f초 · 신선 %.0f%%\n곰팡이 %.0f%% · 부패 %d개\n"),
					*ACCLFoodExperiment::StorageName(Index), Row.EnvironmentSource == 1 ? TEXT("외기") : Row.EnvironmentSource == 2 ? TEXT("실내") : TEXT("고정"), Row.Temperature, Row.Humidity * 100., Row.Quantity, Row.Lots,
					Row.AgeSeconds, Row.Freshness * 100., Row.Mold * 100., Row.Spoiled);
			}
			Text += TEXT("밀폐 부패 계수 0.2 / 곰팡이 노출 0. 외기는 구역 02 날씨, 실내는 구역 11 공간을 관측한다.");
			return FText::FromString(Text);
		})];
	auto FoodButtons = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(4, 4));
	for (const auto& Entry : TArray<TPair<FString, ECCLExperimentAction>>{
		{TEXT("새 음식 2개 추가"), ECCLExperimentAction::FoodAdd},
		{TEXT("노출 → 냉각 1개 이동"), ECCLExperimentAction::FoodMoveCold},
		{TEXT("냉각 음식 해동 / 재냉각"), ECCLExperimentAction::FoodThaw},
		{TEXT("노출: 외기 / 고정"), ECCLExperimentAction::FoodWeather},
		{TEXT("밀폐: 실내 / 고정"), ECCLExperimentAction::FoodIndoor},
		{TEXT("노출 → 바닥 1개 꺼내기"), ECCLExperimentAction::FoodDrop},
		{TEXT("바닥 → 노출 회수"), ECCLExperimentAction::FoodCollect}})
	{
		auto Control = Button(*Entry.Key, [Action, Value = Entry.Value]() { Action(Value); }, WaterAllowed);
		FoodButtons->AddSlot()[Control];
		if (Entry.Value == ECCLExperimentAction::FoodAdd)
		{
			FoodAddButton = Control;
		}

		if (Entry.Value == ECCLExperimentAction::FoodWeather)
		{
			FoodWeatherButton = Control;
		}

		if (Entry.Value == ECCLExperimentAction::FoodDrop)
		{
			FoodDropButton = Control;
		}

		if (Entry.Value == ECCLExperimentAction::FoodCollect)
		{
			FoodCollectButton = Control;
		}
	}
	FoodControls->InsertSlot(0).AutoHeight()[FoodButtons];
	FoodControls->InsertSlot(0).AutoHeight().Padding(0, 4)
		[SNew(STextBlock).AutoWrapText(true).Text_Lambda([this]()
		{
			const auto* Food = ACCLFoodExperiment::Find(GetWorld());
			return FText::FromString(FString::Printf(TEXT("바닥 음식 %d개 · 위치의 온습도 적용\n지역 밖: 20°C / 습도 50%%"), Food ? Food->GetView().GroundQuantity : 0));
		})];


	auto FireControls = SNew(SVerticalBox).Visibility_Lambda([this]()
	{
		return SelectedCase == TEXT("Zone_10") || SelectedCase == TEXT("Zone_11") ? EVisibility::Visible : EVisibility::Collapsed;
	});
	FireControls->AddSlot().AutoHeight().Padding(0, 8)
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 16)).AutoWrapText(true).Text_Lambda([this]()
		{
			const auto* Model = UCCLSurfaceReplication::View(GetWorld());
			const bool bIndoor = SelectedCase == TEXT("Zone_11");
			const auto* G = Model ? Model->FindRegion(ACCLExperimentDirector::FireRegionId(bIndoor)) : nullptr;
			if (!G)
			{
				return FText::FromString(TEXT("연료·실내 상태 수신 대기"));
			}

			double Fuel = 0., Burned = 0.;
			int32 Burning = 0;
			for (const auto& Cell : G->Fire.Cells)
			{
				Fuel += Cell.FuelKg;
				Burned += Cell.BurnedKg;
				Burning += Cell.bBurning ? 1 : 0;
			}

			FString Text = FString::Printf(TEXT("남은 연료 %.2f kg · 탄 양 %.2f kg · 불붙은 셀 %d\n발생 연기 %.2f g · 비 %.0f mm/h"),
				Fuel, Burned, Burning, G->Fire.EmittedSmokeKg * 1000., G->Forcing.RainMetersPerWorldSecond * 3600000.);
			if (bIndoor)
			{
				if (const auto* Room = Model->GetAirSimulation().FindSpace(ACCLExperimentDirector::FireSpaceId()))
				{
					Text += FString::Printf(TEXT("\n실내 %.1f°C · 수증기 %.2f g/m³ · 연기 %.3f g/m³"), Room->TemperatureCelsius(),
						Room->Contents.VaporKg * 1000. / Room->VolumeCubicMeters, Room->Contents.SmokeKg * 1000. / Room->VolumeCubicMeters);
				}
			}

			Text += TEXT("\n뜨거운 연료·고온 실내는 체력을 줄인다. 세계 0배에서도 열 피해는 진행된다.");
			return FText::FromString(Text);
		})];
	FireControls->AddSlot().AutoHeight()[Button(TEXT("마른 연료에 발화"), [Action]() { Action(ECCLExperimentAction::FireIgnite); }, WaterAllowed)];
	FireControls->AddSlot().AutoHeight()[SNew(SBox).Visibility_Lambda([this]() { return SelectedCase == TEXT("Zone_10") ? EVisibility::Visible : EVisibility::Collapsed; })
		[Button(TEXT("소화용 비 켜기 / 끄기"), [Action]() { Action(ECCLExperimentAction::FireRain); }, WaterAllowed)]];

	auto Detail = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 19)).ColorAndOpacity(FLinearColor(0.45f, 0.8f, 1))
		.Text_Lambda([this]()
		{
			const auto* Result = Director() ? Director()->FindResult(SelectedCase) : nullptr;
			return FText::FromString(TEXT("상태: ") + UCCLExperimentDefinition::StatusText(Result ? Result->Status : ECCLExperimentStatus::NotImplemented));
		})]
		+ SVerticalBox::Slot().AutoHeight()[EnvironmentControls]
		+ SVerticalBox::Slot().AutoHeight()[TerrainControls]
		+ SVerticalBox::Slot().AutoHeight()[WaterControls]
		+ SVerticalBox::Slot().AutoHeight()[SnowControls]
		+ SVerticalBox::Slot().AutoHeight()[WeatherControls]
		+ SVerticalBox::Slot().AutoHeight()[FireControls]
		+ SVerticalBox::Slot().AutoHeight()[EcologyControls]
		+ SVerticalBox::Slot().AutoHeight()[BiologyControls]
		+ SVerticalBox::Slot().AutoHeight()[FoodControls]
		+ SVerticalBox::Slot().AutoHeight()[VegetationControls]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
		[SNew(STextBlock)
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 16)).AutoWrapText(true).Text_Lambda([this]()
		{
			const auto* Definition = Director() ? Director()->FindDefinition(SelectedCase) : nullptr;
			return FText::FromString(Definition ? TEXT("조작: ") + Definition->Instructions.ToString() + TEXT("\n\n예상 결과: ") + Definition->Expected.ToString() : FString());
		})]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
		[SNew(STextBlock).Visibility_Lambda([this]()
		{
			const auto* Result = Director() ? Director()->FindResult(SelectedCase) : nullptr;
			return Result && (Result->RunId.IsValid() || Result->Status == ECCLExperimentStatus::Failed) ? EVisibility::Visible : EVisibility::Collapsed;
		})
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 17)).AutoWrapText(true).Text_Lambda([this]()
		{
			const auto* Result = Director() ? Director()->FindResult(SelectedCase) : nullptr;
			if (!Result)
			{
				return FText::FromString(TEXT("서버 상태를 기다리고 있다."));
			}

			return FText::FromString(FString::Printf(TEXT("결과: %s\n완료 세계 시각: %s\n실행 ID: %s"), *Result->Detail,
				*FormatElapsedTime(Result->CompletedWorldSeconds), Result->RunId.IsValid() ? *Result->RunId.ToString().Left(8) : TEXT("미실행")));
		})]
		+ SVerticalBox::Slot().AutoHeight()[Storage]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
		[SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(
			TEXT("천체창 열림: 모든 배율 조작은 관측에 적용. 천체창 닫힘: 세계 시간에 적용. Enter로 적용, 0은 정지.")))]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 15)).AutoWrapText(true).Text_Lambda([this]()
		{
			const auto* D = Director();
			if (!D)
			{
				return FText::GetEmpty();
			}
			FString Text = D->IsRestoring() ? TEXT("복원 중: 지형 충돌과 세계 상태 준비\n") : FString();
			Text += D->GetStorageMessage();
			if (D->GetSavedGameSeconds() >= 0.)
			{
				Text += FString::Printf(TEXT("\n저장 시점: 게임 %s\n세계 %s · 지형 %llu"),
					*FormatElapsedTime(D->GetSavedGameSeconds()), *FormatElapsedTime(D->GetSavedWorldSeconds()), D->GetSavedTerrainRevision());
			}
			return FText::FromString(Text);
		})];

	FirstButton = CloseButton;
	ZoneSelector = SNew(SComboButton)
		.ButtonContent()
		[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 18)).Text_Lambda([this]()
		{
			const auto* Definition = Director() ? Director()->FindDefinition(SelectedCase) : nullptr;
			return Definition ? Definition->Title : FText::FromString(TEXT("실험 선택"));
		})]
		.MenuContent()[SNew(SBox).WidthOverride(390).MaxDesiredHeight(340)
			[SNew(SScrollBox) + SScrollBox::Slot()[ZoneList]]];

	MenuPanel = SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.025f, 0.045f, 0.075f, 0.94f)).Padding(14)
		[SNew(SBox).WidthOverride_Lambda([this]()
		{
			return FOptionalSize(FMath::Clamp(float(GetCachedGeometry().GetLocalSize().X) * 0.36f - 28.f, 330.f, 492.f));
		})
		.HeightOverride_Lambda([this]()
		{
			return FOptionalSize(FMath::Clamp(float(GetCachedGeometry().GetLocalSize().Y) - 72.f, 360.f, 820.f));
		})
		[SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold", 24)).Text(FText::FromString(TEXT("환경 실험실")))]
				+ SHorizontalBox::Slot().AutoWidth()[CloseButton.ToSharedRef()]]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 15)).AutoWrapText(true).Text_Lambda([this]()
			{
				for (TActorIterator<ACCLWorldEnvironmentState> It(GetWorld()); It; ++It)
				{
					const auto& Time = It->GetTime();
					return FText::FromString(FString::Printf(TEXT("게임 %s\n세계 %s  ·  %g배  ·  대기 %.2f초"),
						*FormatElapsedTime(Time.GameSeconds), *FormatElapsedTime(Time.WorldSeconds), Time.TimeScale, Time.PendingGameSeconds));
				}
				return FText::FromString(TEXT("공통 시계 수신 대기"));
			})]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)[ZoneSelector.ToSharedRef()]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
			[SNew(SComboButton).ButtonContent()[SNew(STextBlock).Text(FText::FromString(TEXT("맵 이동")))]
				.MenuContent()[SNew(SBox).WidthOverride(460)[MapButtons]]]
			+ SVerticalBox::Slot().FillHeight(1)
			[SAssignNew(DetailScroll, SScrollBox) + SScrollBox::Slot()[Detail]]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)[Controls]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 6)
			[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 14)).AutoWrapText(true)
			.ColorAndOpacity(FLinearColor(0.65f, 0.9f, 1.f)).Text_Lambda([this]()
			{
				return FText::FromString(Controller() ? Controller()->GetExperimentMessage() : FString());
			})]
			+ SVerticalBox::Slot().AutoHeight()
			[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 13)).AutoWrapText(true)
				.Text(FText::FromString(TEXT("F7 열기/닫기 · 닫은 뒤 WASD 이동\n관찰 참가자는 조작 담당자의 변경을 함께 본다.")))]]];
	CelestialView = SNew(SCCLCelestialDebugView).World(GetWorld());
	ObservationScaleInput = SNew(SEditableTextBox).MinDesiredWidth(120.f)
		.ClearKeyboardFocusOnCommit(false)
		.Text(FText::FromString(TEXT("600000")))
		.ToolTipText(FText::FromString(TEXT("관측 시간만 진행한다. 0은 관측 정지. 지수 표기 예: 1e10")))
		.OnTextCommitted_Lambda([this](const FText&, ETextCommit::Type Commit)
		{
			if (Commit == ETextCommit::OnEnter)
			{
				ApplyObservationScale();
			}
		});
	ObservationApplyButton = Button(TEXT("관측 배율 적용"), [this]() { ApplyObservationScale(); }, []() { return true; });
	ObservationFollowButton = Button(TEXT("세계 시각으로 복귀"), [this]()
	{
		StaticCastSharedPtr<SCCLCelestialDebugView>(CelestialView)->FollowWorldTime();
		RefreshScaleInputs(true);
	}, []() { return true; });
	auto ObservationControls = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6, 4))
		+ SWrapBox::Slot().VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(TEXT("관측 배율")))]
		+ SWrapBox::Slot().VAlign(VAlign_Center)[ObservationScaleInput.ToSharedRef()]
		+ SWrapBox::Slot()[ObservationApplyButton.ToSharedRef()]
		+ SWrapBox::Slot()[ObservationFollowButton.ToSharedRef()];
	Host->SetContent(SNew(SOverlay)
		+ SOverlay::Slot()[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor::Transparent)]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center)[MenuPanel.ToSharedRef()]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(8)
		[SNew(SBox).Visibility_Lambda([this]() { return bShowCelestialView && SelectedCase == TEXT("Zone_01") ? EVisibility::Visible : EVisibility::Collapsed; })
			.WidthOverride_Lambda([this]() { return FOptionalSize(FMath::Clamp(float(GetCachedGeometry().GetLocalSize().X) * 0.6f - 16.f, 360.f, 900.f)); })
			.HeightOverride_Lambda([this]() { return FOptionalSize(FMath::Clamp(float(GetCachedGeometry().GetLocalSize().Y) - 72.f, 360.f, 760.f)); })
			[SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[SNew(SBorder).Padding(8)[ObservationControls]]
				+ SVerticalBox::Slot().FillHeight(1)[CelestialView.ToSharedRef()]]]);
	RefreshScaleInputs(true);
}

void UCCLExperimentScreen::OnContextReleased()
{
	if (Host)
	{
		Host->SetContent(SNullWidget::NullWidget);
	}

	if (auto* PC = Controller())
	{
		PC->CancelTerrainTool();
	}

	bShowCelestialView = false;
	DetailScroll.Reset();
	CelestialViewButton.Reset();
	CelestialView.Reset();
	ObservationScaleInput.Reset();
	ObservationApplyButton.Reset();
	ObservationFollowButton.Reset();
	ZoneSelector.Reset();
	MenuPanel.Reset();
	FirstButton.Reset();
	CloseButton.Reset();
	StartButton.Reset();
	ScaleStopButton.Reset();
	ScaleDecreaseButton.Reset();
	ScaleIncreaseButton.Reset();
	ScaleInput.Reset();
	ScaleApplyButton.Reset();
	RotationButton.Reset();
	DoorButton.Reset();
	EcologyFeedButton.Reset();
	EcologyPanel.Reset();
	FoodAddButton.Reset();
	FoodWeatherButton.Reset();
	BiologyPanel.Reset();
	BiologyAgeButton.Reset();
	BiologyDamageButton.Reset();
	FoodDropButton.Reset();
	FoodCollectButton.Reset();
	FoodPanel.Reset();
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

void UCCLExperimentScreen::SelectCase(FName CaseId)
{
	SelectedCase = CaseId;
	if (CelestialView)
	{
		StaticCastSharedPtr<SCCLCelestialDebugView>(CelestialView)->FollowWorldTime();
	}
	RefreshScaleInputs(true);
	if (DetailScroll)
	{
		DetailScroll->ScrollToStart();
	}
	if (auto* PC = Controller())
	{
		PC->CancelTerrainTool();
	}
}

void UCCLExperimentScreen::RevealControl(const TSharedPtr<SWidget>& Widget)
{
	if (DetailScroll && Widget)
	{
		DetailScroll->ScrollDescendantIntoView(Widget, false, EDescendantScrollDestination::Center);
	}
}

void UCCLExperimentScreen::RevealFoodControls()
{
	if (DetailScroll && FoodPanel)
	{
		DetailScroll->ScrollDescendantIntoView(FoodPanel, false, EDescendantScrollDestination::TopOrLeft);
	}
}

void UCCLExperimentScreen::RevealEcologyControls()
{
	if (DetailScroll&&EcologyPanel)
	{
		DetailScroll->ScrollDescendantIntoView(EcologyPanel,false,EDescendantScrollDestination::TopOrLeft);
	}
}


void UCCLExperimentScreen::RevealBiologyControls()
{
	if (DetailScroll && BiologyPanel)
	{
		DetailScroll->ScrollDescendantIntoView(BiologyPanel, false, EDescendantScrollDestination::TopOrLeft);
	}
}
