#include "CCLExperimentSmokeSubsystem.h"

#include "Environment/CCLExperimentDefinition.h"
#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLTerrainRegion.h"
#include "Environment/CCLExperimentPlayerController.h"
#include "Environment/CCLExperimentScreen.h"
#include "Environment/CCLExperimentStation.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Environment/CCLWorldEnvironmentConfig.h"
#include "Environment/CCLWorldEnvironmentState.h"
#include "Environment/CCLWorldEnvironmentPresentation.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "UI/CCLGameUI.h"
#include "UI/Core/CCLUISubsystem.h"
#include "Input/CommonUIActionRouterBase.h"
#include "Engine/World.h"
#include "Engine/DirectionalLight.h"
#include "Components/LightComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Camera/CameraActor.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SViewport.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#if WITH_EDITOR
#include "Editor.h"
#include "Containers/Ticker.h"
#endif

namespace
{
void FinishExperimentTest(UWorld* World, uint8 ExitCode)
{
#if WITH_EDITOR
	if (World && World->WorldType == EWorldType::PIE && GEditor)
	{
		GEditor->RequestEndPlayMap();
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([ExitCode](float)
		{
			if (GEditor && GEditor->PlayWorld)
			{
				return true;
			}

			FPlatformMisc::RequestExitWithStatus(false, ExitCode);
			return false;
		}), 1.f);
		return;
	}
#endif
	FPlatformMisc::RequestExitWithStatus(false, ExitCode);
}
}

bool UCCLExperimentSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLExperimentSmoke"));
#endif
}

void UCCLExperimentSmokeSubsystem::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	if (Started == 0)
	{
		Started = Now;
	}

	if (Now - Started > 150)
	{
		Check(false, TEXT("watchdog"));
		return;
	}

	auto* World = GetWorld();
	if (!World || !World->HasBegunPlay() || Now < Next)
	{
		static bool bReportedWorld = false;
		if (World && !World->HasBegunPlay() && !bReportedWorld && Now - Started > 20.)
		{
			UE_LOG(LogTemp, Warning, TEXT("CCL_EXPERIMENT_WAIT World=%s has not begun play Mode=%d"), *World->GetName(), int32(World->GetNetMode()));
			bReportedWorld = true;
		}
		return;
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("CCLExperimentTravel")))
	{
		TickTravel(Now);
		return;
	}

	auto* Director = ACCLExperimentDirector::Find(World);
	auto* PC = Cast<ACCLExperimentPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
	if (!Director || !Director->IsReady() || (World->GetNetMode() != NM_DedicatedServer && (!PC || !PC->GetPawn())))
	{
		static bool bReportedReadiness = false;
		if (!bReportedReadiness && Now - Started > 20.)
		{
			UE_LOG(LogTemp, Warning, TEXT("CCL_EXPERIMENT_WAIT Director=%s Ready=%d Results=%d PC=%s Pawn=%s CanOperate=%d Mode=%d"),
				*GetNameSafe(Director), Director && Director->IsReady(), Director ? Director->GetResults().Num() : 0,
				*GetNameSafe(PC), *GetNameSafe(PC ? PC->GetPawn() : nullptr), Director && Director->CanOperate(PC), int32(World->GetNetMode()));
			bReportedReadiness = true;
		}
		return;
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("CCLExperimentMenuSmoke")))
	{
		TickMenuInput(Now);
		return;
	}

	FString Error;
	const FName Guide(TEXT("Zone_00"));
	const FName Clock(TEXT("Zone_01"));
	const FName Reserved(TEXT("Zone_02"));
	const FName Snapshot(TEXT("Zone_08"));
	auto Execute = [&](ECCLExperimentAction Action, FName Case = NAME_None)
	{
		const auto* Result = Director->FindResult(Case);
		return Director->Execute(PC, Action, Case, Director->GetGeneration(), Result ? Result->RunId : FGuid(), Error);
	};
	auto Passed = [&](FName Case)
	{
		const auto* Result = Director->FindResult(Case);
		if (Result && Result->Status == ECCLExperimentStatus::Failed)
		{
			Check(false, *Result->Detail);
		}

		return Result && Result->Status == ECCLExperimentStatus::Passed;
	};

	if (World->GetNetMode() == NM_Client)
	{
		FString Role;
		FParse::Value(FCommandLine::Get(), TEXT("CCLExperimentRole="), Role);
		if (Step == 0)
		{
			if (Role == TEXT("driver") && !Director->CanOperate(PC))
			{
				return;
			}

			if (!Check(Director->Definitions.Num() == 12 && Director->GetResults().Num() == 12,
				TEXT("all definitions and case states available on client")))
			{
				return;
			}

			if (!CheckReplicatedEnvironment())
			{
				return;
			}

			PreviousGeneration = Director->GetGeneration();
			PreviousRun = Director->FindResult(Guide)->RunId;
			PC->Submit(Role == TEXT("driver") ? ECCLExperimentAction::Start : ECCLExperimentAction::Reset, Guide);
			Step = 1;
			Next = Now + 1;
			return;
		}

		if (Role == TEXT("driver"))
		{
			if (Passed(Guide) && Director->FindResult(Guide)->RunId != PreviousRun)
			{
				Finish(TEXT("Driver"));
			}
		}
		else if (!PC->GetExperimentMessage().IsEmpty())
		{
			if (Check(!Director->CanOperate(PC) && Director->GetGeneration() == PreviousGeneration &&
				PC->GetExperimentMessage().Contains(TEXT("조작 담당자만")), TEXT("observer RPC cannot reset the experiment")))
			{
				Finish(TEXT("Observer"));
			}
		}

		return;
	}

	auto* Runtime = World->GetSubsystem<UCCLWorldSimulationSubsystem>();
	auto* Agents = World->GetSubsystem<UCCLAgentWorldSubsystem>();
	const bool bScenario = UCCLWorldSimulationSubsystem::DomainForWorld(World) == ECCLWorldDomain::Scenario;
	const bool bRendered = FParse::Param(FCommandLine::Get(), TEXT("CCLExperimentCapture"));
	FString CheckpointMode;
	if (FParse::Value(FCommandLine::Get(), TEXT("CCLExperimentCheckpoint="), CheckpointMode))
	{
		if (!Director->GetTerrainRegion() || !Director->GetTerrainRegion()->IsTerrainReady())
		{
			return;
		}

		FString ExpectedPath;
		if (!Check(World->GetNetMode() == NM_Standalone &&
			FParse::Value(FCommandLine::Get(), TEXT("CCLExperimentExpected="), ExpectedPath), TEXT("checkpoint test has an isolated expected record")))
		{
			return;
		}

		if (CheckpointMode == TEXT("write"))
		{
			FCCLWorldSnapshot Expected;
			TArray<uint8> ExpectedBytes;
			if (!Check(Execute(ECCLExperimentAction::CycleOpening) &&
				(!bScenario || Execute(ECCLExperimentAction::NextLatitude)), TEXT("persist changed observer and partial door")))
			{
				return;
			}

			if (!Check(Runtime->QueueGameTime(37, Error) && Runtime->AdvancePending(40, 2400, Error) &&
				Runtime->ChangeTimeScale(7, Error) && Runtime->QueueGameTime(1.25, Error) && Execute(ECCLExperimentAction::Save) &&
				FCCLWorldSnapshotCodec::Capture(Runtime->GetIdentity(), Runtime->GetClock(), Agents->GetSimulation(), Expected, Error, &Runtime->GetEnvironmentInputs(), &Runtime->GetSurfaceSimulation()) &&
				FCCLWorldSnapshotCodec::Encode(Expected, ExpectedBytes, Error) && FFileHelper::SaveArrayToFile(ExpectedBytes, *ExpectedPath),
				TEXT("write checkpoint with advanced life, scale history and pending time")))
			{
				return;
			}

			Finish(TEXT("CheckpointWrite"));
			return;
		}

		FCCLWorldSnapshot Expected;
		FCCLWorldSnapshot Actual;
		FCCLLifeSimulation ExpectedLife;
		TArray<uint8> ExpectedBytes;
		TArray<uint8> NormalizedExpected;
		TArray<uint8> ActualBytes;
		if (!Check(CheckpointMode == TEXT("read") && FFileHelper::LoadFileToArray(ExpectedBytes, *ExpectedPath) &&
			FCCLWorldSnapshotCodec::Decode(ExpectedBytes, Expected, Error) &&
			// SaveGame custom-version registrations can have a different order in a new process.
			ExpectedLife.Load(Expected.Life, Error) && ExpectedLife.Save(Expected.Life) &&
			FCCLWorldSnapshotCodec::Encode(Expected, NormalizedExpected, Error) &&
			Runtime->GetIdentity().WorldId != Expected.Identity.WorldId && Execute(ECCLExperimentAction::Load) &&
			FCCLWorldSnapshotCodec::Capture(Runtime->GetIdentity(), Runtime->GetClock(), Agents->GetSimulation(), Actual, Error, &Runtime->GetEnvironmentInputs(), &Runtime->GetSurfaceSimulation()) &&
			FCCLWorldSnapshotCodec::Encode(Actual, ActualBytes, Error) && ActualBytes == NormalizedExpected,
			TEXT("new process restores exact world identity, clock, pending input and life without offline aging")))
		{
			FFileHelper::SaveArrayToFile(ActualBytes, *(ExpectedPath + TEXT(".actual")));
			UE_LOG(LogTemp, Display, TEXT("CCL_EXPERIMENT_SMOKE Checkpoint error=%s ExpectedBytes=%d ActualBytes=%d ExpectedWorld=%.9f ActualWorld=%.9f"),
				*Error, ExpectedBytes.Num(), ActualBytes.Num(), Expected.Clock.WorldSeconds, Actual.Clock.WorldSeconds);
			return;
		}

		Finish(TEXT("CheckpointRead"));
		return;
	}

	if (Step == 0)
	{
		int32 Stations = 0;
		for (TActorIterator<ACCLExperimentStation> It(World); It; ++It)
		{
			++Stations;
		}

		if (!Check(Stations == 12 && Director->Definitions.Num() == 12 && Director->GetResults().Num() == 12,
			TEXT("twelve marked zones and shared case definitions")) ||
			!Check(!Execute(ECCLExperimentAction::Start, Reserved) &&
				Director->FindResult(Reserved)->Status == ECCLExperimentStatus::NotImplemented, TEXT("unimplemented case rejected honestly")))
		{
			return;
		}

		if (!bScenario && !Check(!Execute(ECCLExperimentAction::ScaleOne) && !Execute(ECCLExperimentAction::NextLatitude, Clock),
			TEXT("global time controls require isolated map")))
		{
			return;
		}

		FCCLSimulationSnapshot Before;
		Agents->GetSimulation().Capture(Before);
		const FGuid BeforeGeneration = Director->GetGeneration();
		const auto Lease = Agents->GetSimulation().GetAgents().Acquire(
			Agents->GetSimulation().GetAgents().GetHandle(Before.Agents[0].Id), FGuid::NewGuid());
		const bool bBusyResetRejected = !Execute(ECCLExperimentAction::Reset) && Director->GetGeneration() == BeforeGeneration;
		Agents->GetSimulation().GetAgents().Release(Lease);
		if (!Check(bBusyResetRejected, TEXT("busy writer rejects reset without changing generation")))
		{
			return;
		}

		Step = 1;
		Next = Now + 1;
		return;
	}

	if (Step == 1)
	{
		if (PC)
		{
			auto* UI = CCLGameUI::Get(PC);
			auto* Screen = UI ? Cast<UCCLExperimentScreen>(UI->FindScreen(PC->GetExperimentView())) : nullptr;
			if (!Screen)
			{
				return;
			}

			const auto* Router = UCommonUIActionRouterBase::Get(*Screen);
			if (!Check(Router && !Router->CanProcessNormalGameInput() && PC->IsMoveInputIgnored() &&
				PC->IsLookInputIgnored(), TEXT("CommonUI menu owns input")))
			{
				return;
			}

			Screen->SelectCase(Guide);
			Capture(TEXT("before-start"));
			if (bRendered)
			{
				const auto Button = Screen->GetStartButton();
				if (!Button || Button->GetCachedGeometry().GetAbsoluteSize().IsNearlyZero())
				{
					return;
				}

				const FVector2D Point = Button->GetCachedGeometry().GetAbsolutePosition() + Button->GetCachedGeometry().GetAbsoluteSize() * 0.5f;
				auto& App = FSlateApplication::Get();
				const auto Window = App.FindWidgetWindow(Button.ToSharedRef());
				UE_LOG(LogTemp, Display, TEXT("CCL_EXPERIMENT_SMOKE Pointer X=%.1f Y=%.1f Enabled=%d Window=%d"),
					Point.X, Point.Y, Button->IsEnabled(), Window.IsValid());
				App.ProcessMouseMoveEvent(FPointerEvent(0, Point, Point, TSet<FKey>(), EKeys::Invalid, 0, FModifierKeysState()));
				App.ProcessMouseButtonDownEvent(Window ? Window->GetNativeWindow() : nullptr,
					FPointerEvent(0, Point, Point, TSet<FKey>{EKeys::LeftMouseButton}, EKeys::LeftMouseButton, 0, FModifierKeysState()));
				App.ProcessMouseButtonUpEvent(FPointerEvent(0, Point, Point, TSet<FKey>(), EKeys::LeftMouseButton, 0, FModifierKeysState()));
			}
			else
			{
				PC->Submit(ECCLExperimentAction::Start, Guide);
			}
		}
		else if (!Check(Execute(ECCLExperimentAction::Start, Guide), TEXT("dedicated case start")))
		{
			return;
		}

		Step = 2;
		Next = Now + 1;
		return;
	}

	if (Step == 2 && Passed(Guide))
	{
		Capture(TEXT("controls"));
		if (!Check(Execute(ECCLExperimentAction::Start, Snapshot), TEXT("snapshot case starts")))
		{
			return;
		}

		Step = 3;
		return;
	}

	if (Step == 3 && Passed(Snapshot))
	{
		if (bScenario && !Check(Execute(ECCLExperimentAction::Start, Clock), TEXT("isolated clock case starts")))
		{
			return;
		}

		Step = 4;
		return;
	}

	if (Step == 4 && (!bScenario || Passed(Clock)))
	{
		if (!Check(Execute(ECCLExperimentAction::Start, Guide), TEXT("start a case before reset")))
		{
			return;
		}

		PreviousGeneration = Director->GetGeneration();
		PreviousRun = Director->FindResult(Guide)->RunId;
		if (!Check(Execute(ECCLExperimentAction::Reset), TEXT("safe reset cancels pending case")) ||
			!Check(Director->GetGeneration() != PreviousGeneration && Director->FindResult(Guide)->Status == ECCLExperimentStatus::Ready,
				TEXT("reset changes execution generation and case state")) ||
			!Check(!Director->Execute(PC, ECCLExperimentAction::Start, Guide, PreviousGeneration, PreviousRun, Error),
				TEXT("old generation rejected")) ||
			!Check(!Director->Execute(PC, ECCLExperimentAction::Stop, Guide, Director->GetGeneration(), PreviousRun, Error),
				TEXT("old run ID rejected")) ||
			!Check(Execute(ECCLExperimentAction::Start, Guide), TEXT("fresh case restarts after reset")))
		{
			return;
		}

		if (PC && !Check(FVector::Dist2D(PC->GetPawn()->GetActorLocation(), FVector(-550, -450, 0)) < 50,
			TEXT("player moved to safe reset location")))
		{
			return;
		}

		Step = 5;
		return;
	}

	if (Step == 5 && Passed(Guide))
	{
		const double SavedTime = Runtime->GetClock().GetWorldSeconds();
		const FGuid SavedWorld = Runtime->GetIdentity().WorldId;
		if (!Check(Execute(ECCLExperimentAction::Save), TEXT("dedicated experiment slot saved")) ||
			!Check(Runtime->QueueGameTime(1, Error) && Runtime->AdvancePending(2, 120, Error), TEXT("advance after checkpoint")) ||
			!Check(Execute(ECCLExperimentAction::Load), TEXT("experiment checkpoint restored")) ||
			!Check(Runtime->GetClock().GetWorldSeconds() == SavedTime && Agents->GetSimulation().GetTime() == SavedTime &&
				Runtime->GetIdentity().WorldId == SavedWorld, TEXT("clock life and world identity resume together")))
		{
			return;
		}

		Step = 6;
		Next = Now + 1;
		return;
	}

	if (Step == 6)
	{
		if (PC)
		{
			if (bRendered)
			{
				FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::F7, FModifierKeysState(), 0, false, 0, 0));
				FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(EKeys::F7, FModifierKeysState(), 0, false, 0, 0));
			}
			else
			{
				PC->CCLExperiment();
			}

			if (bRendered)
			{
				const FVector Location(-500, 0, 280);
				auto* Camera = World->SpawnActor<ACameraActor>(Location, UKismetMathLibrary::FindLookAtRotation(Location, FVector(680, 0, 170)));
				PC->SetViewTarget(Camera);
			}
		}

		Step = 7;
		Next = Now + 2;
		return;
	}

	if (Step == 7)
	{
		if (PC)
		{
			auto* UI = CCLGameUI::Get(PC);
			const auto* Router = PC->GetLocalPlayer()->GetSubsystem<UCommonUIActionRouterBase>();
			if (!Check(UI && !UI->IsViewOpen(PC->GetExperimentView()) && Router && Router->CanProcessNormalGameInput() &&
				!PC->IsMoveInputIgnored() && !PC->IsLookInputIgnored(), TEXT("closing CommonUI returns gameplay input")))
			{
				return;
			}
		}

		Capture(TEXT("station"));
		Step = 8;
		Next = Now + 2;
		return;
	}

	if (Step == 8)
	{
		if (PC && bRendered)
		{
			const FVector Location(3750, -5000, 9000);
			auto* Camera = World->SpawnActor<ACameraActor>(Location, UKismetMathLibrary::FindLookAtRotation(Location, FVector(3750, 2500, 0)));
			PC->SetViewTarget(Camera);
		}

		Step = 9;
		Next = Now + 2;
		return;
	}

	if (Step == 9)
	{
		Capture(TEXT("overview"));
		Step = 10;
		Next = Now + 2;
		return;
	}

	if (Step >= 10)
	{
		TickEnvironment(Now);
	}
}

TStatId UCCLExperimentSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLExperimentSmokeSubsystem, STATGROUP_Tickables);
}

void UCCLExperimentSmokeSubsystem::TickMenuInput(double Now)
{
	if (Step >= 20)
	{
		TickLab(Now);
		return;
	}
	auto* PC = Cast<ACCLExperimentPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
	auto* UI = CCLGameUI::Get(PC);
	const auto* Router = PC->GetLocalPlayer()->GetSubsystem<UCommonUIActionRouterBase>();
	if (!UI || !Router)
	{
		return;
	}

	const bool bOpen = UI->IsViewOpen(PC->GetExperimentView());
	const auto KeyEvent = FKeyEvent(EKeys::F7, FModifierKeysState(), 0, false, 0, 0);
	auto& App = FSlateApplication::Get();
	if (Step == 0)
	{
		if (!bOpen)
		{
			return;
		}

		Capture(TEXT("menu-initial"));
	}
	else if (Step == 1)
	{
		auto* Screen = Cast<UCCLExperimentScreen>(UI->FindScreen(PC->GetExperimentView()));
		if (!ClickControl(Screen ? Screen->GetCloseButton() : nullptr))
		{
			return;
		}
	}
	else if (Step == 2 || Step == 8)
	{
		if (!Check(!bOpen && Router->CanProcessNormalGameInput() && !PC->IsMoveInputIgnored() && !PC->IsLookInputIgnored(),
			TEXT("menu closed and gameplay input restored")))
		{
			return;
		}

		Capture(TEXT("menu-closed"));
	}
	else if (Step == 3 || Step == 6 || Step == 9)
	{
		// Reopening must pass through the focused viewport and Enhanced Input, not the menu command.
		App.ProcessKeyDownEvent(KeyEvent);
	}
	else if (Step == 4 || Step == 7 || Step == 10)
	{
		if (Step != 7)
		{
			App.ProcessKeyDownEvent(FKeyEvent(EKeys::F7, FModifierKeysState(), 0, true, 0, 0));
			if (!Check(UI->IsViewOpen(PC->GetExperimentView()), TEXT("held menu key does not close the reopened menu")))
			{
				return;
			}
		}

		App.ProcessKeyUpEvent(KeyEvent);
	}
	else if (Step == 5 || Step == 11)
	{
		if (!Check(bOpen && !Router->CanProcessNormalGameInput() && PC->IsMoveInputIgnored() && PC->IsLookInputIgnored(),
			TEXT("F7 reopens menu and applies menu input")) ||
			!Check(PC->GetPawn() && PC->GetPawn()->GetController() == PC, TEXT("menu shortcut preserves player possession")))
		{
			return;
		}

#if WITH_EDITOR
		if (GetWorld()->WorldType == EWorldType::PIE && !Check(GEditor && !GEditor->bIsSimulatingInEditor,
			TEXT("PIE remains in play mode after menu shortcut")))
		{
			return;
		}
#endif
		Capture(TEXT("menu-reopened"));
	}
	else if (Step == 12)
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("CCLExperimentLabSmoke")))
		{
			Step = 20;
			return;
		}
		Finish(TEXT("MenuInput"));
		return;
	}

	++Step;
	Next = Now + 0.75;
}

void UCCLExperimentSmokeSubsystem::TickTravel(double Now)
{
	static const TCHAR* Maps[] = {TEXT("EnvironmentPlayground"), TEXT("EnvironmentScenario"),
		TEXT("EnvironmentPlayground"), TEXT("CombatPlayground"), TEXT("EnvironmentPlayground"),
		TEXT("MultiplayerPlayground"), TEXT("EnvironmentPlayground")};
	auto* World = GetWorld();
	auto* PC = GetGameInstance()->GetFirstLocalPlayerController();
	if (!PC || !PC->GetPawn() || Step >= UE_ARRAY_COUNT(Maps) || UGameplayStatics::GetCurrentLevelName(World) != Maps[Step])
	{
		return;
	}

	auto* Director = ACCLExperimentDirector::Find(World);
	auto* Runtime = World->GetSubsystem<UCCLWorldSimulationSubsystem>();
	auto* ExperimentPC = Cast<ACCLExperimentPlayerController>(PC);
	if (Director && (!Director->IsReady() || !Director->CanOperate(PC)))
	{
		return;
	}

	FString Error;
	if (Step == 0)
	{
		HubWorldId = Runtime->GetIdentity().WorldId;
		if (!Check(Runtime->QueueGameTime(5, Error) && Runtime->AdvancePending(6, 360, Error), TEXT("advance hub before map travel")))
		{
			return;
		}

		HubGameSeconds = Runtime->GetClock().GetGameSeconds();
	}
	else if (Step == 1)
	{
		if (!Check(Runtime->GetIdentity().Domain == ECCLWorldDomain::Scenario && Runtime->GetIdentity().WorldId != HubWorldId,
			TEXT("isolated scenario has its own world identity")))
		{
			return;
		}
	}
	else if (Step == 2 || Step == 4 || Step == 6)
	{
		if (!Check(Runtime->GetIdentity().WorldId == HubWorldId && Runtime->GetClock().GetGameSeconds() >= HubGameSeconds,
			TEXT("return to hub resumes its original clock and world")))
		{
			return;
		}
	}
	else
	{
		auto* UI = CCLGameUI::Get(PC);
		const FGameplayTag ExperimentTag = FGameplayTag::RequestGameplayTag(TEXT("UI.View.EnvironmentExperiment"));
		if (!Check(!ExperimentPC && UI && !UI->FindRegistration(ExperimentTag).IsValid(), TEXT("existing playground releases experiment UI registration")))
		{
			return;
		}
	}

	if (Step == 6)
	{
		const auto* Store = GetGameInstance()->GetSubsystem<UCCLAgentSessionStore>();
		FCCLWorldSnapshot Initial;
		const auto* Baseline = Store->ExperimentInitialSnapshots.Find(ECCLWorldDomain::Playground);
		if (!Check(Baseline && FCCLWorldSnapshotCodec::Decode(*Baseline, Initial, Error) && Director->ResetExperiment(Error) &&
			Runtime->GetIdentity().WorldId == HubWorldId && Runtime->GetClock().GetWorldSeconds() == Initial.Clock.WorldSeconds,
			TEXT("reset after round trips retains the original baseline")))
		{
			return;
		}

		Finish(TEXT("Travel"));
		return;
	}

	if (ExperimentPC)
	{
		const ECCLExperimentAction Action = Step == 0 ? ECCLExperimentAction::TravelScenario :
			Step == 2 ? ECCLExperimentAction::TravelCombat : Step == 4 ? ECCLExperimentAction::TravelMultiplayer : ECCLExperimentAction::TravelHub;
		ExperimentPC->Submit(Action, NAME_None);
	}
	else
	{
		UGameplayStatics::OpenLevel(this, TEXT("/Game/Maps/EnvironmentPlayground"));
	}

	++Step;
	Next = Now + 1;
}

bool UCCLExperimentSmokeSubsystem::Check(bool bCondition, const TCHAR* Message)
{
	UE_LOG(LogTemp, Display, TEXT("CCL_EXPERIMENT_SMOKE %s %s"), bCondition ? TEXT("CHECK") : TEXT("FAIL"), Message);
	if (!bCondition)
	{
		bComplete = 1;
		FinishExperimentTest(GetWorld(), 1);
	}

	return bCondition;
}

void UCCLExperimentSmokeSubsystem::Capture(const TCHAR* Name)
{
	if (FParse::Param(FCommandLine::Get(), TEXT("CCLExperimentCapture")))
	{
		FString Directory;
		if (!FParse::Value(FCommandLine::Get(), TEXT("CCLExperimentCaptureDir="), Directory))
		{
			Directory = FPaths::ProjectSavedDir() / TEXT("EnvironmentExperiments/Visual");
		}

		IFileManager::Get().MakeDirectory(*Directory, true);
		FScreenshotRequest::RequestScreenshot(Directory / (FString(Name) + TEXT(".png")), true, false);
	}
}

void UCCLExperimentSmokeSubsystem::Finish(const TCHAR* Role)
{
	UE_LOG(LogTemp, Display, TEXT("CCL_EXPERIMENT_SMOKE PASS Role=%s Map=%s"), Role, *GetWorld()->GetMapName());
	bComplete = 1;
	if (GetWorld()->GetNetMode() == NM_Standalone || FCString::Strcmp(Role, TEXT("Observer")) == 0)
	{
		FinishExperimentTest(GetWorld(), 0);
	}
}

bool UCCLExperimentSmokeSubsystem::ClickControl(TSharedPtr<SWidget> Control)
{
	if (!Control || !Control->IsEnabled() || Control->GetCachedGeometry().GetAbsoluteSize().IsNearlyZero())
	{
		return Check(false, *FString::Printf(TEXT("visible enabled environment control: exists=%d enabled=%d size=%s"),
			Control.IsValid(), Control && Control->IsEnabled(), Control ? *Control->GetCachedGeometry().GetAbsoluteSize().ToString() : TEXT("none")));
	}

	const FVector2D Point = Control->GetCachedGeometry().GetAbsolutePosition() + Control->GetCachedGeometry().GetAbsoluteSize() * 0.5;
	auto& App = FSlateApplication::Get();
	const auto Window = App.FindWidgetWindow(Control.ToSharedRef());
	App.ProcessMouseMoveEvent(FPointerEvent(0, Point, Point, TSet<FKey>(), EKeys::Invalid, 0, FModifierKeysState()));
	App.ProcessMouseButtonDownEvent(Window ? Window->GetNativeWindow() : nullptr,
		FPointerEvent(0, Point, Point, TSet<FKey>{EKeys::LeftMouseButton}, EKeys::LeftMouseButton, 0, FModifierKeysState()));
	App.ProcessMouseButtonUpEvent(FPointerEvent(0, Point, Point, TSet<FKey>(), EKeys::LeftMouseButton, 0, FModifierKeysState()));
	return true;
}

bool UCCLExperimentSmokeSubsystem::CheckReplicatedEnvironment()
{
	const auto* Config = ACCLWorldEnvironmentConfig::Find(GetWorld());
	if (!Config || !Config->CelestialDefinition)
	{
		return Check(false, TEXT("client map config exists"));
	}

	for (TActorIterator<ACCLWorldEnvironmentState> It(GetWorld()); It; ++It)
	{
		const auto Time = It->GetTime();
		const auto& View = Time.Environment;
		if (!View.bValid || View.Surfaces.Num() != 8 || View.Openings.Num() != 1 || View.Openings[0].OpenFraction != 1.)
		{
			return false;
		}

		FString Error;
		FCCLCelestialSystem System;
		FCCLCelestialObservation Observation;
		if (!Check(System.Initialize(Config->CelestialDefinition->Definition, Error)
			&& System.Observe(Time.WorldSeconds, View.Observer, Observation, Error)
			&& Observation.Stars.Num() == View.Stars.Num(), TEXT("client rebuilds exact server observation timestamp")))
		{
			return false;
		}

		for (int32 Index = 0; Index < View.Stars.Num(); ++Index)
		{
			if (!Check(Observation.Stars[Index].LocalDirection.Equals(View.Stars[Index].LocalDirection, 1.e-10)
				&& FMath::IsNearlyEqual(Observation.Stars[Index].SolarHours, View.Stars[Index].SolarHours, 1.e-9),
				TEXT("replicated stellar direction and local time match their committed world time")))
			{
				return false;
			}
		}

		const auto* Inside = View.Probes.FindByPredicate([](const auto& Probe) { return Probe.ProbeId == TEXT("Inside"); });
		return Check(View.DefinitionId == Config->CelestialDefinition->Definition.DefinitionId
			&& View.DefinitionVersion == Config->CelestialDefinition->Definition.Version
			&& View.Seed == Config->CelestialDefinition->Definition.Seed && View.SurfaceEpoch.IsValid()
			&& View.InputRevision > 1 && Inside && Inside->Transmission.Wind == 1. && Inside->Transmission.Precipitation == 0.,
			TEXT("late joining client receives opened door and independent roof shelter"));
	}

	return false;
}

void UCCLExperimentSmokeSubsystem::TickEnvironment(double Now)
{
	auto* World = GetWorld();
	auto* Director = ACCLExperimentDirector::Find(World);
	auto* Runtime = World->GetSubsystem<UCCLWorldSimulationSubsystem>();
	auto* PC = Cast<ACCLExperimentPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
	auto* UI = PC ? CCLGameUI::Get(PC) : nullptr;
	auto* Screen = UI ? Cast<UCCLExperimentScreen>(UI->FindScreen(PC->GetExperimentView())) : nullptr;
	const auto* Config = ACCLWorldEnvironmentConfig::Find(World);
	const bool bRendered = FParse::Param(FCommandLine::Get(), TEXT("CCLExperimentCapture"));
	const bool bScenario = UCCLWorldSimulationSubsystem::DomainForWorld(World) == ECCLWorldDomain::Scenario;
	FString Error;
	auto Execute = [&](ECCLExperimentAction Action, FName Case = NAME_None)
	{
		return Director->Execute(PC, Action, Case, Director->GetGeneration(), FGuid(), Error);
	};
	auto Passed = [&](FName Case)
	{
		const auto* Result = Director->FindResult(Case);
		if (Result && Result->Status == ECCLExperimentStatus::Failed)
		{
			Check(false, *Result->Detail);
		}

		return Result && Result->Status == ECCLExperimentStatus::Passed;
	};
	auto DoorBlocked = [&]()
	{
		FHitResult Hit;
		return World->LineTraceSingleByChannel(Hit, FVector(7700., 4950., 150.), FVector(8000., 4950., 150.), ECC_Visibility);
	};
	if (Step == 10)
	{
		if (Check(Config && Execute(ECCLExperimentAction::Start, TEXT("Zone_01")), TEXT("map celestial reproduction case starts")))
		{
			Step = 11;
		}

		return;
	}

	if (Step == 11 && Passed(TEXT("Zone_01")))
	{
		if (Check(Execute(ECCLExperimentAction::Start, TEXT("Zone_11")), TEXT("map shelter case starts")))
		{
			Step = 12;
		}

		return;
	}

	if (Step == 12 && Passed(TEXT("Zone_11")))
	{
		auto IncompleteView = Runtime->GetEnvironmentInputs();
		const uint64 OriginalRevision = IncompleteView.Revision;
		const FGuid OriginalEpoch = Runtime->GetSurfaceProvider().GetEpoch();
		++IncompleteView.Revision;
		++IncompleteView.SurfaceRevision;
		IncompleteView.Surfaces.Pop();
		if (!Check(!Runtime->ReplaceEnvironmentInputs(IncompleteView, Error)
			&& Runtime->GetEnvironmentInputs().Revision == OriginalRevision
			&& Runtime->GetSurfaceProvider().GetEpoch() == OriginalEpoch,
			TEXT("missing displayed surface rejects replacement without partial state")))
		{
			return;
		}

		auto MissingOpening = Runtime->GetEnvironmentInputs();
		++MissingOpening.Revision;
		++MissingOpening.SurfaceRevision;
		auto ExtraOpening = MissingOpening.Openings[0];
		ExtraOpening.OpeningId = FGuid::NewGuid();
		ExtraOpening.CenterUV = FVector2D(2., 0.);
		ExtraOpening.HalfExtentsMeters = FVector2D(0.4, 0.4);
		MissingOpening.Openings.Add(ExtraOpening);
		if (!Check(!Runtime->ReplaceEnvironmentInputs(MissingOpening, Error)
			&& Runtime->GetEnvironmentInputs().Revision == OriginalRevision,
			TEXT("unlisted opening cannot silently disagree with rendered collision")))
		{
			return;
		}

		if (!Check(DoorBlocked(), TEXT("closed door has real collision on authority")))
		{
			return;
		}

		if (bRendered && PC)
		{
			PC->CCLExperiment();
			auto* NewScreen = Cast<UCCLExperimentScreen>(UI->FindScreen(PC->GetExperimentView()));
			if (!Check(NewScreen != nullptr, TEXT("celestial controls opened")))
			{
				return;
			}

			NewScreen->SelectCase(TEXT("Zone_01"));
			NewScreen->RevealControl(NewScreen->GetRotationButton());
		}

		Step = 13;
		Next = Now + 1;
		return;
	}

	if (Step == 13 && bRendered)
	{
		if (!Screen)
		{
			return;
		}
		Screen->RevealControl(Screen->GetRotationButton());
		Step = 131;
		Next = Now + 1.;
		return;
	}
	if (Step == 13 || Step == 131)
	{
		Capture(TEXT("celestial-controls"));
		if (bScenario)
		{
			const auto& Inputs = Runtime->GetEnvironmentInputs();
			PreviousSpinPhase = Inputs.Celestial.Bodies.FindByPredicate([&Inputs](const auto& Body) { return Body.BodyId == Inputs.Observer.BodyId; })->SpinPhaseDegrees;
			if (!Check(Execute(ECCLExperimentAction::NextLatitude) && Execute(ECCLExperimentAction::NextObliquity), TEXT("server changes latitude and obliquity"))
				|| !(bRendered ? ClickControl(Screen ? Screen->GetRotationButton() : nullptr) : Execute(ECCLExperimentAction::RotateQuarter)))
			{
				return;
			}
		}

		Step = 14;
		Next = Now + 1;
		return;
	}

	if (Step == 14)
	{
		if (World->GetNetMode() != NM_DedicatedServer)
		{
			for (TActorIterator<ACCLWorldEnvironmentState> It(World); It; ++It)
			{
				const auto& View = It->GetTime().Environment;
				const auto* Star = View.Stars.FindByPredicate([&View](const auto& Value) { return Value.BodyId == View.DominantStarId; });
				for (TActorIterator<ACCLWorldEnvironmentPresentation> Display(World); Display; ++Display)
				{
					const auto* Sun = Display->Sun.Get();
					if (!Check(Star && Sun && FVector::DotProduct(Sun->GetActorForwardVector(), -Star->LocalDirection) > 0.9999
						&& (Star->ElevationDegrees > 0. || Sun->GetLightComponent()->Intensity == 0.f),
						TEXT("directional light follows celestial direction and turns off below horizon")))
					{
						return;
					}
				}
			}
		}

		if (bScenario)
		{
			const auto& Inputs = Runtime->GetEnvironmentInputs();
			const auto* Body = Inputs.Celestial.Bodies.FindByPredicate([&Inputs](const auto& Value) { return Value.BodyId == Inputs.Observer.BodyId; });
			if (!Check(Inputs.Observer.LatitudeDegrees == 90. && Body->ObliquityDegrees == 45.
				&& Body->SpinPhaseDegrees == FMath::Fmod(PreviousSpinPhase + 90., 360.), TEXT("latitude tilt and actual rotation control applied"))
				|| !Check(Execute(ECCLExperimentAction::OrbitQuarter), TEXT("orbital phase control applied")))
			{
				return;
			}
		}

		if (Screen)
		{
			Screen->SelectCase(TEXT("Zone_11"));
			Screen->RevealControl(Screen->GetDoorButton());
		}

		Step = 15;
		Next = Now + 1;
		return;
	}

	if (Step == 15 && bRendered)
	{
		if (!Screen)
		{
			return;
		}
		Screen->RevealControl(Screen->GetDoorButton());
		Step = 151;
		Next = Now + 1.;
		return;
	}
	if (Step == 15 || Step == 151)
	{
		Capture(TEXT("shelter-controls"));
		if (!(bRendered ? ClickControl(Screen ? Screen->GetDoorButton() : nullptr) : Execute(ECCLExperimentAction::CycleOpening)))
		{
			return;
		}

		Step = 16;
		Next = Now + 1;
		return;
	}

	if (Step == 16)
	{
		if (!Check(Runtime->GetEnvironmentInputs().Openings[0].OpenFraction == 0.5 && !DoorBlocked(), TEXT("half-open aperture changes visible-mesh collision")))
		{
			return;
		}

		// Return celestial parameters to the map definition for the subsequent late-join comparison.
		auto Inputs = Runtime->GetEnvironmentInputs();
		Inputs.Celestial = Config->CelestialDefinition->Definition;
		Inputs.Observer = Config->Observer;
		++Inputs.Revision;
		++Inputs.SurfaceRevision;
		if (!Check(Runtime->ReplaceEnvironmentInputs(Inputs, Error), TEXT("restore map celestial comparison baseline")))
		{
			return;
		}

		if (bRendered && PC)
		{
			PC->CCLExperiment();
			const FVector Location(8150., 4250., 600.);
			auto* Camera = World->SpawnActor<ACameraActor>(Location, UKismetMathLibrary::FindLookAtRotation(Location, FVector(7350., 5000., 180.)));
			PC->SetViewTarget(Camera);
		}

		Step = 17;
		Next = Now + 2;
		return;
	}

	if (Step == 17)
	{
		Capture(TEXT("shelter-half"));
		if (!Check(Execute(ECCLExperimentAction::CycleOpening), TEXT("fully open door")))
		{
			return;
		}

		Step = 18;
		Next = Now + 1;
		return;
	}

	if (Step == 18)
	{
		const FGuid BeforeEpoch = Runtime->GetSurfaceProvider().GetEpoch();
		if (!Check(Runtime->GetEnvironmentInputs().Openings[0].OpenFraction == 1. && !DoorBlocked(), TEXT("fully open door has no residual collision"))
			|| !Check(Execute(ECCLExperimentAction::Save) && Execute(ECCLExperimentAction::CycleOpening)
				&& Runtime->GetEnvironmentInputs().Openings[0].OpenFraction == 0. && Execute(ECCLExperimentAction::Load)
				&& Runtime->GetEnvironmentInputs().Openings[0].OpenFraction == 1.
				&& Runtime->GetSurfaceProvider().GetEpoch() != BeforeEpoch, TEXT("opening restore rebuilds scene in a new execution epoch")))
		{
			return;
		}

		Step = 19;
		Next = Now + 2;
		return;
	}

	if (Step == 19)
	{
		Capture(TEXT("shelter-open"));
		if (!CheckReplicatedEnvironment())
		{
			return;
		}

		Step = 20;
		Next = Now + 1;
		return;
	}

	if (Step == 20)
	{
		Finish(TEXT("Authority"));
	}
}

void UCCLExperimentSmokeSubsystem::TickLab(double Now)
{
	auto* World = GetWorld();
	auto* PC = Cast<ACCLExperimentPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
	auto* UI = CCLGameUI::Get(PC);
	auto* Screen = Cast<UCCLExperimentScreen>(UI->FindScreen(PC->GetExperimentView()));
	auto* Director = ACCLExperimentDirector::Find(World);
	auto* Region = Director->GetTerrainRegion();
	auto* Runtime = World->GetSubsystem<UCCLWorldSimulationSubsystem>();
	if (!Screen || !Region || !Region->IsTerrainReady() || Region->IsPreparing())
	{
		return;
	}
	auto& App = FSlateApplication::Get();
	FString Error;
	if (Step == 20)
	{
		const FGeometry Full = Screen->GetCachedGeometry();
		const FGeometry Panel = Screen->GetMenuPanel()->GetCachedGeometry();
		const FVector2D Local = Full.AbsoluteToLocal(Panel.GetAbsolutePosition());
		if (!Check(FMath::Abs(Local.X) < 2. && Panel.GetAbsoluteSize().X < Full.GetAbsoluteSize().X * 0.43
			&& FMath::Abs(Local.Y + Panel.GetLocalSize().Y * 0.5 - Full.GetLocalSize().Y * 0.5) < 2.,
			TEXT("menu flush left, vertically centered and narrower than half viewport")))
		{
			return;
		}
		Screen->SelectCase(TEXT("Zone_01"));
		PC->Submit(ECCLExperimentAction::ScaleZero, TEXT("Zone_01"));
	}
	else if (Step == 21)
	{
		Screen->RevealControl(Screen->GetCelestialViewButton());
	}
	else if (Step == 22)
	{
		if (!ClickControl(Screen->GetCelestialViewButton()))
		{
			return;
		}
	}
	else if (Step == 23)
	{
		if (!Check(Screen->GetCelestialView()->GetCachedGeometry().GetLocalSize().X > 300., TEXT("3D celestial inspection receives layout")))
		{
			return;
		}
		Capture(TEXT("celestial-debug"));
		PreviousTerrainRevision = Runtime->GetEnvironmentInputs().Revision;
		Screen->RevealControl(Screen->GetRotationButton());
	}
	else if (Step == 24)
	{
		if (!ClickControl(Screen->GetRotationButton()))
		{
			return;
		}
	}
	else if (Step == 25)
	{
		if (!Check(Runtime->GetEnvironmentInputs().Revision > PreviousTerrainRevision, TEXT("visible rotation button changes published input")))
		{
			return;
		}
		Capture(TEXT("celestial-rotated"));
		Screen->SelectCase(TEXT("Zone_05"));
		const FVector Focus = Region->GetActorLocation();
		const FVector Location = Focus + FVector(1600., -1600., 1500.);
		auto* Camera = World->SpawnActor<ACameraActor>(Location, UKismetMathLibrary::FindLookAtRotation(Location, Focus));
		PC->SetViewTarget(Camera);
		PC->SelectTerrainTool(ECCLExperimentAction::TerrainExcavate);
	}
	else if (Step == 26)
	{
		for (int32 I = 0; I < 3; ++I)
		{
			PC->Submit(ECCLExperimentAction::RotateQuarter, TEXT("Zone_01"));
		}

		FHitResult Hit;
		const FVector Target = Region->GetActorLocation() + FVector(200., 0., 0.);
		if (!Check(World->LineTraceSingleByChannel(Hit, Target + FVector(0, 0, 1000), Target - FVector(0, 0, 1000), ECC_Visibility)
			&& Hit.GetActor() == Region, TEXT("editable surface cursor target exists"))) { return; }
		FVector2D Pixel;
		if (!Check(PC->ProjectWorldLocationToScreen(Hit.ImpactPoint, Pixel), TEXT("terrain cursor projects to viewport")))
		{
			return;
		}
		USlateBlueprintLibrary::ScreenToWidgetAbsolute(World, Pixel, LabCursor, false);
		if (!Check(!Screen->GetMenuPanel()->GetCachedGeometry().IsUnderLocation(LabCursor), TEXT("terrain cursor outside menu")))
		{
			return;
		}
		PreviousTerrainRevision = Region->GetTerrainStore().GetRevision();
		LabTarget = Hit.ImpactPoint;
		FVector2D RoundTrip, WidgetPosition;
		USlateBlueprintLibrary::AbsoluteToViewport(World, LabCursor, RoundTrip, WidgetPosition);
		if (!Check(RoundTrip.Equals(Pixel, 0.1), TEXT("viewport pixel and Slate absolute coordinates round trip"))) { return; }
		FHitResult DirectHit;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(ExperimentLabPointer), true);
		Query.AddIgnoredActor(PC->GetPawn());
		const bool bDirectHit = PC->GetHitResultAtScreenPosition(Pixel, ECC_Visibility, Query, DirectHit);
		FVector2D HitPixel;
		// UE deprojection truncates subpixels. Bound error in pixels, not world centimeters.
		if (!Check(bDirectHit && DirectHit.GetActor() == Region
			&& PC->ProjectWorldLocationToScreen(DirectHit.ImpactPoint, HitPixel) && HitPixel.Equals(Pixel, 1.1),
			TEXT("projected target deprojects within one viewport pixel"))) { return; }
		LabTarget = DirectHit.ImpactPoint;
		App.SetCursorPos(LabCursor);
		App.ProcessMouseMoveEvent(FPointerEvent(0, LabCursor, LabCursor, TSet<FKey>(), EKeys::Invalid, 0, FModifierKeysState()));
	}
	else if (Step == 27)
	{
		Capture(TEXT("terrain-cursor"));
		const auto Window = App.FindWidgetWindow(Screen->TakeWidget());
		App.ProcessMouseButtonDownEvent(Window ? Window->GetNativeWindow() : nullptr,
			FPointerEvent(0, LabCursor, LabCursor, TSet<FKey>{EKeys::LeftMouseButton}, EKeys::LeftMouseButton, 0, FModifierKeysState()));
		App.ProcessMouseButtonUpEvent(FPointerEvent(0, LabCursor, LabCursor, TSet<FKey>(), EKeys::LeftMouseButton, 0, FModifierKeysState()));
		FHitResult CursorHit;
		const bool bHit = PC->TraceTerrainCursor(CursorHit);
		if (!Check(bHit && CursorHit.GetActor() == Region && CursorHit.ImpactPoint.Equals(LabTarget, 2.),
			*FString::Printf(TEXT("Slate click maps to world target: expected=%s actual=%s actor=%s"),
				*LabTarget.ToString(), *CursorHit.ImpactPoint.ToString(), *GetNameSafe(CursorHit.GetActor())))) { return; }
	}
	else if (Step == 28)
	{
		if (!Check(Region->GetTerrainStore().GetRevision() > PreviousTerrainRevision, *FString::Printf(TEXT("menu exterior click commits terrain at cursor: %s"), *PC->GetExperimentMessage())))
		{
			return;
		}
		Capture(TEXT("terrain-edited"));
		const uint64 Revision = Region->GetTerrainStore().GetRevision();
		const FVector Invalid(1.e9, 1.e9, 1.e9);
		if (!Check(!Director->Execute(PC, ECCLExperimentAction::TerrainDeposit, TEXT("Zone_05"), Director->GetGeneration(), FGuid(), Error,
			Region->GetPublicationSerial(), &Invalid) && Region->GetTerrainStore().GetRevision() == Revision, TEXT("out of bounds terrain request rejected atomically"))) { return; }
		PC->CancelTerrainTool();
		PC->Submit(ECCLExperimentAction::Save, TEXT("Zone_08"));
	}
	else if (Step == 29)
	{
		if (!Check(Director->GetSavedGameSeconds() >= 0. && Director->GetSavedTerrainRevision() == Region->GetTerrainStore().GetRevision(),
			TEXT("save feedback describes stored snapshot")))
		{
			return;
		}
		PC->Submit(ECCLExperimentAction::RotateQuarter, TEXT("Zone_01"));
		PC->Submit(ECCLExperimentAction::Load, TEXT("Zone_08"));
		Screen->SelectCase(TEXT("Zone_08"));
	}
	else if (Step == 30)
	{
		if (Director->IsRestoring())
		{
			return;
		}
		if (!Check(Director->GetStorageMessage().Contains(TEXT("복원 완료")), TEXT("restore feedback waits for terrain and world commit")))
		{
			return;
		}
		Capture(TEXT("snapshot-restored"));
	}
	else if (Step == 31)
	{
		Finish(TEXT("MenuInput"));
		return;
	}
	++Step;
	Next = Now + 1.;
}
