#include "CCLBiologyZoneSmokeSubsystem.h"

#include "Environment/CCLBiologyExperiment.h"
#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLExperimentPlayerController.h"
#include "Environment/CCLExperimentScreen.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Campaign/CCLLifeVillager.h"
#include "UI/CCLGameUI.h"
#include "UI/Core/CCLUISubsystem.h"
#include "Camera/CameraActor.h"
#include "Engine/World.h"
#include "UnrealClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Misc/FileHelper.h"

bool UCCLBiologyZoneSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLBiologyZoneSmoke"));
#endif
}

void UCCLBiologyZoneSmokeSubsystem::Tick(float DeltaTime)
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
	auto* Lab = W ? ACCLBiologyExperiment::Find(W) : nullptr;
	if (!D || !D->IsReady() || !R || !Lab || Lab->GetView().Rows.Num() != 2 || Now < Next)
	{
		return;
	}
	const auto& V = Lab->GetView();
	auto* PC = Cast<ACCLExperimentPlayerController>(W->GetFirstPlayerController());
	auto* Agents = W->GetSubsystem<UCCLAgentWorldSubsystem>();
	FString Error, Directory;
	FParse::Value(FCommandLine::Get(), TEXT("CCLBiologyZoneSmokeDir="), Directory);
	auto Check = [&](bool OK, const TCHAR* Message)
	{
		if (!OK)
		{
			Finish(false, FString::Printf(TEXT("step=%d %s %s"), Step, Message, *Error));
		}
		return OK;
	};
	auto ActorCount = [&]() { return int32(Lab->FindActor(0) != nullptr) + int32(Lab->FindActor(1) != nullptr); };
	auto Oracle = [&]()
	{
		FString Value = FString::Printf(TEXT("%s,%.17g"), *V.Epoch.ToString(), V.WorldSeconds);
		for (const auto& Row : V.Rows)
		{
			Value += FString::Printf(TEXT(",%d,%d,%d,%.17g,%.17g,%.9g"), Row.bRecorded, Row.bProtected, Row.DeathCause,
									 Row.Age, Row.Lifespan, Row.Health);
		}
		return Value;
	};
	if (W->GetNetMode() == NM_Client)
	{
		const bool bLate = FParse::Param(FCommandLine::Get(), TEXT("CCLBiologyZoneLate"));
		if (!bLate && Step == 0 && ActorCount() == 2 && V.Rows[0].bRecorded && V.Rows[1].bRecorded &&
			V.Rows[0].DeathCause == 0 && V.Rows[1].DeathCause == 0)
		{
			if (!Check(FFileHelper::SaveStringToFile(TEXT("ready"), *(Directory / TEXT("base-observed.txt"))),
					   TEXT("baseline acknowledgement")))
			{
				return;
			}
			Step = 1;
		}
		FString Expected;
		if ((!bLate && Step == 0) || !FFileHelper::LoadFileToString(Expected, *(Directory / TEXT("final-view.txt"))) ||
			Expected != Oracle() || ActorCount() != 0)
		{
			return;
		}
		if (!Check(!Lab->Execute(ECCLExperimentAction::BiologyDamage, Error), TEXT("client mutation denied")))
		{
			return;
		}
		Finish(true, TEXT("final epoch time biology quality and dead Actor absence match server"));
		return;
	}
	const bool Rendered = FParse::Param(FCommandLine::Get(), TEXT("CCLBiologyZoneCapture"));
	auto Execute = [&](ECCLExperimentAction Action)
	{
		return D->Execute(PC, Action, TEXT("Zone_07"), D->GetGeneration(), D->FindResult(TEXT("Zone_07"))->RunId,
						  Error);
	};
	auto Screen = [&]() -> UCCLExperimentScreen*
	{
		auto* UI = PC ? CCLGameUI::Get(PC) : nullptr;
		return UI ? Cast<UCCLExperimentScreen>(UI->FindScreen(PC->GetExperimentView())) : nullptr;
	};
	auto Click = [&](bool Age)
	{
		if (!Rendered)
		{
			return Execute(Age ? ECCLExperimentAction::BiologyAge : ECCLExperimentAction::BiologyDamage);
		}
		auto* S = Screen();
		auto Widget = S ? (Age ? S->GetBiologyAgeButton() : S->GetBiologyDamageButton()) : nullptr;
		if (!Widget || !Widget->IsEnabled() || Widget->GetCachedGeometry().GetAbsoluteSize().IsNearlyZero())
		{
			return false;
		}
		const auto Point =
			Widget->GetCachedGeometry().GetAbsolutePosition() + Widget->GetCachedGeometry().GetAbsoluteSize() * .5;
		auto& App = FSlateApplication::Get();
		const auto Window = App.FindWidgetWindow(Widget.ToSharedRef());
		App.ProcessMouseMoveEvent(
			FPointerEvent(0, Point, Point, TSet<FKey>(), EKeys::Invalid, 0, FModifierKeysState()));
		App.ProcessMouseButtonDownEvent(Window ? Window->GetNativeWindow() : nullptr,
										FPointerEvent(0, Point, Point, TSet<FKey>{EKeys::LeftMouseButton},
													  EKeys::LeftMouseButton, 0, FModifierKeysState()));
		App.ProcessMouseButtonUpEvent(
			FPointerEvent(0, Point, Point, TSet<FKey>(), EKeys::LeftMouseButton, 0, FModifierKeysState()));
		return true;
	};
	auto Capture = [&](const TCHAR* Name)
	{
		if (Rendered)
		{
			FScreenshotRequest::RequestScreenshot(Directory / Name, true, false);
		}
	};
	if (Step == 0)
	{
		R->SetTickableTickType(ETickableTickType::Never);
		if (!Check(R->ChangeTimeScale(0., Error), TEXT("pause")))
		{
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("CCL_BIOLOGY_ZONE BASE_READY"));
		Step = 1;
		Next = Now + 1.;
		return;
	}
	if (!PC || !D->CanOperate(PC))
	{
		return;
	}
	if (Step == 1)
	{
		if (W->GetNetMode() != NM_Standalone && !FPaths::FileExists(Directory / TEXT("base-observed.txt")))
		{
			return;
		}
		if (!Check(V.Rows[0].bRecorded && V.Rows[1].bRecorded && ActorCount() == 2 && V.Rows[0].Health == 100.f &&
					   V.Rows[1].bProtected && Execute(ECCLExperimentAction::BiologyProtect) &&
					   !CCLAgentBiology::Find(*Agents->GetSimulation().Find(Lab->AgentId(1)))->bProtectNaturalAging &&
					   Execute(ECCLExperimentAction::BiologyProtect),
				   TEXT("authored residents and protection toggle")))
		{
			return;
		}
		if (Rendered)
		{
			auto* Camera = W->SpawnActor<ACameraActor>(
				UCCLExperimentDefinition::ZoneCenter(7) + FVector(0., 1900., 500.), FRotator(-16., -90., 0.));
			PC->SetViewTarget(Camera);
			auto* UI = CCLGameUI::Get(PC);
			if (!UI->IsViewOpen(PC->GetExperimentView()))
			{
				PC->CCLExperiment();
			}
			if (Screen())
			{
				Screen()->SelectCase(TEXT("Zone_07"));
				Screen()->RevealBiologyControls();
			}
		}
		Step = 2;
		Next = Now + 2.;
		return;
	}
	if (Step == 2)
	{
		Capture(TEXT("biology-controls.png"));
		if (!Check(Click(true), TEXT("lifespan button")))
		{
			return;
		}
		Step = 3;
		Next = Now + 1.;
		return;
	}
	if (Step == 3)
	{
		const auto* B = CCLAgentBiology::Find(*Agents->GetSimulation().Find(Lab->AgentId(0)));
		if (!Check(B &&
					   FMath::IsNearlyEqual(B->BirthWorldSeconds + B->LifespanSeconds,
											R->GetClock().GetWorldSeconds() + 10., 1.e-8) &&
					   R->ChangeTimeScale(4., Error),
				   TEXT("lifespan shortened without changing birth")))
		{
			return;
		}
		Target = R->GetClock().GetWorldSeconds() + 12.;
		Step = 4;
		return;
	}
	if (Step == 4)
	{
		if (R->GetClock().GetWorldSeconds() < Target)
		{
			if (!R->GetClock().HasPendingTime() && !Check(R->QueueGameTime(.25, Error), TEXT("queue age")))
			{
				return;
			}
			if (!R->AdvancePending(.25, 1., Error))
			{
				Check(Error.Contains(TEXT("awaits")), TEXT("advance age"));
			}
			return;
		}
		if (V.Rows[0].DeathCause != 1 || ActorCount() != 1)
		{
			return;
		}
		if (!Check(V.Rows[1].DeathCause == 0 && V.Rows[1].bProtected && R->ChangeTimeScale(0., Error),
				   TEXT("natural death and protected survivor")))
		{
			return;
		}
		Capture(TEXT("biology-aged.png"));
		Step = 5;
		Next = Now + 1.;
		return;
	}
	if (Step == 5)
	{
		if (!Check(Click(false), TEXT("fatal damage button")))
		{
			return;
		}
		Step = 6;
		Next = Now + 1.;
		return;
	}
	if (Step == 6)
	{
		if (!Check(V.Rows[1].DeathCause == 2 && ActorCount() == 0 && V.Rows[1].Health == 0.f &&
					   Execute(ECCLExperimentAction::Save),
				   TEXT("fatal damage defeats aging protection and save")))
		{
			return;
		}
		Capture(TEXT("biology-dead.png"));
		Step = 7;
		Next = Now + 1.;
		return;
	}
	if (Step == 7)
	{
		if (!Check(Execute(ECCLExperimentAction::Reset), TEXT("reset biology")))
		{
			return;
		}
		Step = 8;
		Next = Now + 1.;
		return;
	}
	if (Step == 8)
	{
		if (D->IsRestoring() || ActorCount() != 2 || V.Rows[0].DeathCause != 0 || V.Rows[1].DeathCause != 0)
		{
			return;
		}
		if (!Check(V.Rows[0].Health == 100.f && V.Rows[1].Health == 100.f && Execute(ECCLExperimentAction::Load),
				   TEXT("reset regenerates living Actors then load terminal snapshot")))
		{
			return;
		}
		Step = 9;
		Next = Now + 1.;
		return;
	}
	if (D->IsRestoring() || ActorCount() != 0 || V.Rows[0].DeathCause != 1 || V.Rows[1].DeathCause != 2 ||
		V.Epoch != R->GetEpoch())
	{
		return;
	}
	if (!Check(FFileHelper::SaveStringToFile(Oracle(), *(Directory / TEXT("final-view.txt"))),
			   TEXT("public final oracle")))
	{
		return;
	}
	UE_LOG(LogTemp, Display, TEXT("CCL_BIOLOGY_ZONE RESTORE_READY"));
	Finish(true, TEXT("biology buttons natural death fatal GAS reset restore and Actor lifecycle verified"));
}

void UCCLBiologyZoneSmokeSubsystem::Finish(bool Success, const FString& Message)
{
	bComplete = 1;
	UE_LOG(LogTemp, Display, TEXT("CCL_BIOLOGY_ZONE %s %s"), Success ? TEXT("PASS") : TEXT("FAIL"), *Message);
	if (!Success || (GetWorld() && GetWorld()->GetNetMode() == NM_Standalone))
	{
		FPlatformMisc::RequestExitWithStatus(false, Success ? 0 : 1, TEXT("CCLBiologyZoneSmoke"));
	}
}

TStatId UCCLBiologyZoneSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLBiologyZoneSmokeSubsystem, STATGROUP_Tickables);
}
