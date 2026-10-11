#include "CCLEcologyZoneSmokeSubsystem.h"

#include "Environment/CCLEcologyExperiment.h"
#include "Environment/CCLEcologyRepresentationSubsystem.h"
#include "Environment/CCLCreatureActor.h"
#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLExperimentPlayerController.h"
#include "Environment/CCLExperimentScreen.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "UI/CCLGameUI.h"
#include "KSIUISubsystem.h"
#include "Camera/CameraActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UnrealClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

bool UCCLEcologyZoneSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLEcologyZoneSmoke"));
#endif
}

void UCCLEcologyZoneSmokeSubsystem::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	if (!Started)
	{
		Started = Now;
	}
	if (Now - Started > 240.)
	{
		Finish(false, FString::Printf(TEXT("timeout step=%d"), Step));
		return;
	}
	auto* W = GetWorld();
	auto* D = W ? ACCLExperimentDirector::Find(W) : nullptr;
	auto* R = W ? W->GetSubsystem<UCCLWorldSimulationSubsystem>() : nullptr;
	auto* Lab = W ? ACCLEcologyExperiment::Find(W) : nullptr;
	if (!D || !D->IsReady() || !R || !Lab || Lab->GetView().Rows.Num() != 4 || Now < Next)
	{
		return;
	}
	const auto& View = Lab->GetView();
	FString Directory, Error;
	FParse::Value(FCommandLine::Get(), TEXT("CCLEcologyZoneSmokeDir="), Directory);
	auto Check = [&](bool OK, const TCHAR* Message)
	{
		if (!OK)
		{
			Finish(false, FString::Printf(TEXT("step=%d %s %s"), Step, Message, *Error));
		}
		return OK;
	};
	if (!Check(!Directory.IsEmpty(), TEXT("isolated result directory")))
	{
		return;
	}
	auto* PC = Cast<ACCLExperimentPlayerController>(W->GetFirstPlayerController());
	auto Execute = [&](ECCLExperimentAction Action)
	{
		return D->Execute(PC, Action, TEXT("Zone_06"), D->GetGeneration(), D->FindResult(TEXT("Zone_06"))->RunId,
						  Error);
	};
	auto Counts = [&](int32 Actors, int32 Mass)
	{
		for (const auto& Row : View.Rows)
		{
			if (Row.Living != 20 || Row.Aggregate != 17 || Row.Reserved != 3 || Row.Actors != Actors ||
				Row.Mass != Mass)
			{
				return false;
			}
		}
		return true;
	};
	const bool bNetwork = W->GetNetMode() != NM_Standalone;
	const bool bRendered = FParse::Param(FCommandLine::Get(), TEXT("CCLEcologyZoneCapture"));
	auto Screen = [&]() -> UCCLExperimentScreen*
	{
		auto* UI = PC ? CCLGameUI::Get(PC) : nullptr;
		return UI ? Cast<UCCLExperimentScreen>(UI->FindScreen(PC->GetExperimentView())) : nullptr;
	};
	auto Capture = [&](const TCHAR* Name)
	{
		if (bRendered)
		{
			FScreenshotRequest::RequestScreenshot(Directory / Name, true, false);
		}
	};
	if (W->GetNetMode() == NM_Client)
	{
		if (!FPaths::FileExists(Directory / TEXT("final.txt")))
		{
			if (!Step && Counts(3, 0))
			{
				Step = 1;
				UE_LOG(LogTemp, Display, TEXT("CCL_ECOLOGY_ZONE BASE_OBSERVED"));
			}
			return;
		}
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *(Directory / TEXT("final.txt"))))
		{
			return;
		}
		TArray<FString> Lines;
		Text.ParseIntoArrayLines(Lines);
		if (!Check(Lines.Num() == 17, TEXT("server observation oracle")))
		{
			return;
		}
		if (!Counts(3, 0) || !FMath::IsNearlyEqual(View.WorldSeconds, FCString::Atod(*Lines[0]), 1.e-8))
		{
			return;
		}
		for (int32 N = 0; N < 4; ++N)
		{
			TArray<FString> Parts;
			Lines[1 + N].ParseIntoArray(Parts, TEXT(","));
			if (!Check(Parts.Num() == 5, TEXT("row oracle")))
			{
				return;
			}
			if (!FMath::IsNearlyEqual(View.Rows[N].Food, FCString::Atod(*Parts[0]), 1.e-8) ||
				!FMath::IsNearlyEqual(View.Rows[N].Energy, FCString::Atod(*Parts[1]), 1.e-8) ||
				!FMath::IsNearlyEqual(View.Rows[N].Consumed, FCString::Atod(*Parts[2]), 1.e-8) ||
				!FMath::IsNearlyEqual(View.Rows[N].AggregateEnergy, FCString::Atod(*Parts[3]), 1.e-8) ||
				!FMath::IsNearlyEqual(View.Rows[N].AggregateConsumed, FCString::Atod(*Parts[4]), 1.e-8))
			{
				return;
			}
		}
		int32 Seen = 0;
		for (TActorIterator<ACCLCreatureActor> It(W); It; ++It)
		{
			for (int32 N = 0; N < 12; ++N)
			{
				if (It->GetIndividualId() != ACCLEcologyExperiment::IndividualId(N / 3, N % 3))
				{
					continue;
				}
				TArray<FString> Parts;
				Lines[5 + N].ParseIntoArray(Parts, TEXT(","));
				if (!Check(Parts.Num() == 3, TEXT("actor oracle")))
				{
					return;
				}
				const FVector Expected(FCString::Atod(*Parts[0]), FCString::Atod(*Parts[1]), FCString::Atod(*Parts[2]));
				if (It->GetActorLocation().Equals(Expected * 100., 1.) &&
					It->GetMovementMedium() == ECCLEcologyMovement(N / 3))
				{
					++Seen;
				}
			}
		}
		if (Seen != 12)
		{
			return;
		}
		if (!Check(!Lab->Execute(ECCLExperimentAction::EcologyFeedGround, Error), TEXT("client mutation refused")))
		{
			return;
		}
		Finish(true, TEXT("four public rows and twelve restored actor poses replicated; client mutation refused"));
		return;
	}
	const auto& E = R->GetEcologySimulation();
	if (Step == 0)
	{
		if (!Check(R->ChangeTimeScale(0., Error), TEXT("pause")))
		{
			return;
		}
		R->SetTickableTickType(ETickableTickType::Never);
		UE_LOG(LogTemp, Display, TEXT("CCL_ECOLOGY_ZONE BASE_READY"));
		Step = 1;
		Next = Now + 2.;
		return;
	}
	if (!PC || !D->CanOperate(PC))
	{
		return;
	}
	if (Step == 1)
	{
		if (bNetwork && !FPaths::FileExists(Directory / TEXT("base-observed.txt")))
		{
			return;
		}
		if (!Check(Counts(3, 0) && Lab->CheckState(Error), TEXT("four authored populations and twelve actors")))
		{
			return;
		}
		TArray<uint8> Before, After;
		if (!Check(E.Capture(Before, Error) && !R->SeedEcology(E, Error) && E.Capture(After, Error) && Before == After,
				   TEXT("reseed refuses to overwrite existing ecology")))
		{
			return;
		}
		Error.Reset();
		if (bRendered)
		{
			auto* Camera = W->SpawnActor<ACameraActor>(
				UCCLExperimentDefinition::ZoneCenter(6) + FVector(0, -1500, 1200), FRotator(-38, 90, 0));
			PC->SetViewTarget(Camera);
			auto* UI = CCLGameUI::Get(PC);
			if (!UI->IsViewOpen(PC->GetExperimentView()))
			{
				PC->CCLExperiment();
			}
			if (auto* Current = Screen())
			{
				Current->SelectCase(TEXT("Zone_06"));
				Current->RevealControl(Current->GetEcologyFeedButton());
			}
		}
		FoodBefore = View.Rows[0].Food;
		Step = 2;
		Next = Now + 3.;
		return;
	}
	if (Step == 2)
	{
		if (bRendered)
		{
			auto* Current = Screen();
			auto Control = Current ? Current->GetEcologyFeedButton() : nullptr;
			if (!Check(Control && Control->IsEnabled() &&
						   !Control->GetCachedGeometry().GetAbsoluteSize().IsNearlyZero(),
					   TEXT("visible feed button")))
			{
				return;
			}
			const FVector2D Point = Control->GetCachedGeometry().GetAbsolutePosition() +
									Control->GetCachedGeometry().GetAbsoluteSize() * .5;
			auto& App = FSlateApplication::Get();
			const auto Window = App.FindWidgetWindow(Control.ToSharedRef());
			App.ProcessMouseMoveEvent(
				FPointerEvent(0, Point, Point, TSet<FKey>(), EKeys::Invalid, 0, FModifierKeysState()));
			App.ProcessMouseButtonDownEvent(Window ? Window->GetNativeWindow() : nullptr,
											FPointerEvent(0, Point, Point, TSet<FKey>{EKeys::LeftMouseButton},
														  EKeys::LeftMouseButton, 0, FModifierKeysState()));
			App.ProcessMouseButtonUpEvent(
				FPointerEvent(0, Point, Point, TSet<FKey>(), EKeys::LeftMouseButton, 0, FModifierKeysState()));
		}
		else if (!Check(Execute(ECCLExperimentAction::EcologyFeedGround), TEXT("feed ground")))
		{
			return;
		}
		Step = 3;
		Next = Now + 2.;
		return;
	}
	if (Step == 3)
	{
		if (!Check(FMath::IsNearlyEqual(View.Rows[0].Food, FoodBefore + 10., 1.e-8),
				   TEXT("feed button adds balanced resource")))
		{
			return;
		}
		Capture(TEXT("ecology-controls.png"));
		Step = 31;
		Next = Now + 1.;
		return;
	}
	if (Step == 31)
	{
		if (!Check(Execute(ECCLExperimentAction::EcologyMass) && Counts(0, 3), TEXT("mass preserves counts")))
		{
			return;
		}
		if (!Check(Execute(ECCLExperimentAction::EcologyHidden) && Counts(0, 0),
				   TEXT("hidden preserves reserved population")))
		{
			return;
		}
		for (int32 N = 0; N < 12; ++N)
		{
			PositionsBefore.Add(E.FindIndividual(ACCLEcologyExperiment::IndividualId(N / 3, N % 3))->PositionMeters);
		}
		if (!Check(R->ChangeTimeScale(4., Error), TEXT("advance hidden behavior")))
		{
			return;
		}
		TargetWorld = E.GetWorldSeconds() + 24.;
		Step = 4;
		return;
	}
	if (Step == 4)
	{
		if (E.GetWorldSeconds() < TargetWorld - 1.e-8)
		{
			if (!bQueued)
			{
				if (!Check(R->QueueGameTime(.25, Error), TEXT("queue once")))
				{
					return;
				}
				bQueued = 1;
			}
			if (!R->AdvancePending(.25, 1., Error))
			{
				if (!Check(Error.Contains(TEXT("awaits")), TEXT("spatial advance")))
				{
					return;
				}
				Next = Now + .25;
				return;
			}
			bQueued = 0;
			return;
		}
		if (!Check(R->ChangeTimeScale(0., Error) && Counts(0, 0) && E.Validate(Error),
				   TEXT("hidden model remains valid")))
		{
			return;
		}
		for (int32 N = 0; N < 12; ++N)
		{
			const auto Id = ACCLEcologyExperiment::IndividualId(N / 3, N % 3);
			const auto* B = E.GetBehaviors().Find(Id);
			if (!Check(B && B->ConsumedUnits > 0. &&
						   !E.FindIndividual(Id)->PositionMeters.Equals(PositionsBefore[N], .1),
					   TEXT("every hidden individual moved and fed")))
			{
				UE_LOG(LogTemp, Display, TEXT("CCL_ECOLOGY_ZONE DETAIL individual=%d activity=%d consumed=%.6f"), N,
					   B ? int32(B->Activity) : -1, B ? B->ConsumedUnits : -1.);
				return;
			}
		}
		if (!Check(Execute(ECCLExperimentAction::EcologyActors) && Counts(3, 0),
				   TEXT("actors restored at model positions")))
		{
			return;
		}
		if (bRendered && Screen())
		{
			PC->CCLExperiment();
		}
		Step = 5;
		Next = Now + 2.;
		return;
	}
	if (Step == 5)
	{
		Capture(TEXT("ecology-states.png"));
		Step = 51;
		Next = Now + 1.;
		return;
	}
	if (Step == 51)
	{
		if (!Check(E.Capture(SavedEcology, Error) && Execute(ECCLExperimentAction::Save) &&
					   Execute(ECCLExperimentAction::EcologyFeedWater) && Execute(ECCLExperimentAction::Load),
				   TEXT("save alter load")))
		{
			return;
		}
		Step = 6;
		Next = Now + 2.;
		return;
	}
	if (Step == 6)
	{
		if (D->IsRestoring())
		{
			return;
		}
		TArray<uint8> After;
		if (!Check(E.Capture(After, Error) && After == SavedEcology && Counts(3, 0),
				   TEXT("exact model restore and actor recreation")))
		{
			return;
		}
		if (!Check(Execute(ECCLExperimentAction::Reset), TEXT("reset")))
		{
			return;
		}
		Step = 7;
		Next = Now + 2.;
		return;
	}
	if (Step == 7)
	{
		if (D->IsRestoring())
		{
			return;
		}
		if (!Check(Counts(3, 0), TEXT("reset counts")))
		{
			return;
		}
		for (const auto& Row : View.Rows)
		{
			if (!Check(Row.Food == 100. && Row.Consumed == 0. && Row.AggregateConsumed == 0. &&
					   FMath::IsNearlyEqual(Row.AggregateEnergy, .35, 1.e-8) && FMath::IsNearlyEqual(Row.Energy, .35, 1.e-8),
					   TEXT("reset authored food and behavior")))
			{
				return;
			}
		}
		if (!Check(Execute(ECCLExperimentAction::Load), TEXT("reload final observation")))
		{
			return;
		}
		Step = 8;
		Next = Now + 2.;
		return;
	}
	if (D->IsRestoring())
	{
		return;
	}
	if (Step == 8)
	{
		if (!Check(Execute(ECCLExperimentAction::Start), TEXT("ecology automatic case starts")))
		{
			return;
		}
		Step = 9;
		Next = Now + 1.;
		return;
	}
	if (!Check(D->FindResult(TEXT("Zone_06"))->Status == ECCLExperimentStatus::Passed, TEXT("ecology automatic case passed")))
	{
		return;
	}
	TArray<uint8> After;
	if (!Check(E.Capture(After, Error) && After == SavedEcology && Counts(3, 0), TEXT("final restored model")))
	{
		return;
	}
	FString Oracle = FString::Printf(TEXT("%.12f\n"), View.WorldSeconds);
	for (const auto& Row : View.Rows)
	{
		Oracle += FString::Printf(TEXT("%.12f,%.12f,%.12f,%.12f,%.12f\n"), Row.Food, Row.Energy, Row.Consumed, Row.AggregateEnergy, Row.AggregateConsumed);
	}
	auto* Reps = W->GetSubsystem<UCCLEcologyRepresentationSubsystem>();
	for (int32 N = 0; N < 12; ++N)
	{
		const auto Id = ACCLEcologyExperiment::IndividualId(N / 3, N % 3);
		const auto& Position = E.FindIndividual(Id)->PositionMeters;
		const auto* Entry = Reps->Find(Id);
		if (!Check(Entry && Entry->Actor.IsValid() && Entry->Actor->GetActorLocation().Equals(Position * 100., 1.),
				   TEXT("actual actor matches model")))
		{
			return;
		}
		Oracle += FString::Printf(TEXT("%.12f,%.12f,%.12f\n"), Position.X, Position.Y, Position.Z);
	}
	if (!Check(FFileHelper::SaveStringToFile(Oracle, *(Directory / TEXT("final.txt"))), TEXT("publish final oracle")))
	{
		return;
	}
	UE_LOG(LogTemp, Display, TEXT("CCL_ECOLOGY_ZONE RESTORE_READY"));
	Finish(true, TEXT("zone06 feed actor mass hidden movement feeding save load reset verified"));
}

void UCCLEcologyZoneSmokeSubsystem::Finish(bool bSuccess, const FString& Message)
{
	bComplete = 1;
	UE_LOG(LogTemp, Display, TEXT("CCL_ECOLOGY_ZONE %s %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Message);
	if (!bSuccess || (GetWorld() && GetWorld()->GetNetMode() == NM_Standalone))
	{
		FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1, TEXT("CCLEcologyZoneSmoke"));
	}
}

TStatId UCCLEcologyZoneSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLEcologyZoneSmokeSubsystem, STATGROUP_Tickables);
}
