#include "CCLExperimentSmokeSubsystem.h"

#include "Environment/CCLExperimentDefinition.h"
#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLExperimentPlayerController.h"
#include "Environment/CCLExperimentScreen.h"
#include "Environment/CCLExperimentStation.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "UI/CCLGameUI.h"
#include "UI/Core/CCLUISubsystem.h"
#include "Input/CommonUIActionRouterBase.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Camera/CameraActor.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

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
		return;
	}

	FString Error;
	const FName Guide(TEXT("Zone_00"));
	const FName Clock(TEXT("Zone_01"));
	const FName Reserved(TEXT("Zone_03"));
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
			if (!Check(Runtime->QueueGameTime(37, Error) && Runtime->AdvancePending(40, 2400, Error) &&
				Runtime->ChangeTimeScale(7, Error) && Runtime->QueueGameTime(1.25, Error) && Execute(ECCLExperimentAction::Save) &&
				FCCLWorldSnapshotCodec::Capture(Runtime->GetIdentity(), Runtime->GetClock(), Agents->GetSimulation(), Expected, Error, &Runtime->GetEnvironmentInputs()) &&
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
			FCCLWorldSnapshotCodec::Capture(Runtime->GetIdentity(), Runtime->GetClock(), Agents->GetSimulation(), Actual, Error, &Runtime->GetEnvironmentInputs()) &&
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

		if (!bScenario && !Check(!Execute(ECCLExperimentAction::ScaleOne) && !Execute(ECCLExperimentAction::Start, Clock),
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
				FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::F8, FModifierKeysState(), 0, false, 0, 0));
				FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(EKeys::F8, FModifierKeysState(), 0, false, 0, 0));
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

	if (Step == 10)
	{
		Finish(TEXT("Authority"));
	}
}

TStatId UCCLExperimentSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLExperimentSmokeSubsystem, STATGROUP_Tickables);
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
		FPlatformMisc::RequestExitWithStatus(false, 1);
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
		FPlatformMisc::RequestExitWithStatus(false, 0);
	}
}
