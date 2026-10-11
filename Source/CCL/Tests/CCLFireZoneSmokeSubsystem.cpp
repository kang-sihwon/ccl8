#include "CCLFireZoneSmokeSubsystem.h"

#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLExperimentPlayerController.h"
#include "Environment/CCLExperimentScreen.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Environment/CCLSurfaceReplication.h"
#include "Environment/CCLFirePresentation.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "UI/CCLGameUI.h"
#include "KSIUISubsystem.h"
#include "Camera/CameraActor.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "UnrealClient.h"

bool UCCLFireZoneSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLFireZoneSmoke"));
#endif
}

void UCCLFireZoneSmokeSubsystem::Tick(float DeltaTime)
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

	auto* World = GetWorld();
	auto* Director = World ? ACCLExperimentDirector::Find(World) : nullptr;
	auto* Runtime = World ? World->GetSubsystem<UCCLWorldSimulationSubsystem>() : nullptr;
	const auto* Surface = World ? UCCLSurfaceReplication::View(World) : nullptr;
	if (!Director || !Director->IsReady() || !Surface || !Runtime || Now < Next)
	{
		return;
	}

	const auto* Outdoor = Surface->FindRegion(ACCLExperimentDirector::FireRegionId(false));
	const auto* Indoor = Surface->FindRegion(ACCLExperimentDirector::FireRegionId(true));
	const auto* Room = Surface->GetAirSimulation().FindSpace(ACCLExperimentDirector::FireSpaceId());
	if (!Outdoor || !Indoor || !Room)
	{
		return;
	}

	FString Error;
	auto Check = [&](bool bOK)
	{
		if (!bOK)
		{
			Finish(false, FString::Printf(TEXT("step=%d %s"), Step, *Error));
		}
		return bOK;
	};
	if (World->GetNetMode() == NM_Client)
	{
		if (Outdoor->Forcing.RainMetersPerWorldSecond != 0.12345 || Room->Removed.SmokeKg <= 0.)
		{
			return;
		}

		if (Check(Surface->Validate(Error) && Outdoor->Fire.Cells[5].BurnedKg > 0. && Indoor->Fire.Cells[5].BurnedKg > 0.
			&& !Runtime->IgniteSurface(Outdoor->RegionId, 5, 1000., Error)))
		{
			Finish(true, TEXT("client persistent fire rooms received; authority enforced"));
		}
		return;
	}

	auto* PC = Cast<ACCLExperimentPlayerController>(World->GetFirstPlayerController());
	auto Execute = [&](ECCLExperimentAction Action, FName Case)
	{
		return Director->Execute(PC, Action, Case, Director->GetGeneration(), Director->FindResult(Case)->RunId, Error);
	};
	const bool bRendered = FParse::Param(FCommandLine::Get(), TEXT("CCLFireZoneCapture"));
	auto Select = [&](FName Case)
	{
		if (!bRendered || !PC)
		{
			return;
		}
		auto* UI = CCLGameUI::Get(PC);
		if (UI && !UI->IsViewOpen(PC->GetExperimentView()))
		{
			PC->CCLExperiment();
		}
		auto* Screen = UI ? Cast<UCCLExperimentScreen>(UI->FindScreen(PC->GetExperimentView())) : nullptr;
		if (Screen)
		{
			Screen->SelectCase(Case);
		}
	};
	auto Capture = [&](const TCHAR* Name)
	{
		if (bRendered)
		{
			FString Directory;
			FParse::Value(FCommandLine::Get(), TEXT("CCLFireZoneSmokeDir="), Directory);
			FScreenshotRequest::RequestScreenshot(Directory / Name, true, false);
		}
	};
	auto Camera = [&](const FVector& Position, const FVector& Target)
	{
		if (!bRendered || !PC)
		{
			return;
		}
		auto* View = World->SpawnActor<ACameraActor>(Position, (Target - Position).Rotation());
		PC->SetViewTarget(View);
		auto* UI = CCLGameUI::Get(PC);
		if (UI && UI->IsViewOpen(PC->GetExperimentView()))
		{
			PC->CCLExperiment();
		}
	};
	if (Step == 0)
	{
		if (!Check(Runtime->ChangeTimeScale(0., Error) && Execute(ECCLExperimentAction::Start, TEXT("Zone_10"))))
		{
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("CCL_FIRE_ZONE BASE_READY"));
		Step = 1;
		return;
	}
	if (Step == 1 || Step == 2)
	{
		const auto* Result = Director->FindResult(Step == 1 ? TEXT("Zone_10") : TEXT("Zone_11"));
		if (!Check(Result->Status != ECCLExperimentStatus::Failed) || Result->Status != ECCLExperimentStatus::Passed)
		{
			return;
		}
		if (Step == 1)
		{
			if (!Check(Execute(ECCLExperimentAction::Start, TEXT("Zone_11"))))
			{
				return;
			}
		}
		else
		{
			if (!Check(Execute(ECCLExperimentAction::FireIgnite, TEXT("Zone_10")) && Execute(ECCLExperimentAction::FireIgnite, TEXT("Zone_11"))))
			{
				return;
			}
			Select(TEXT("Zone_10"));
			Next = Now + 5.;
		}
		++Step;
		return;
	}
	if (Step == 3)
	{
		if (!Check(Outdoor->Fire.Cells[5].BurnedKg > 0. && Room->Contents.SmokeKg > 0. && Room->TemperatureCelsius() > 20.))
		{
			return;
		}
		Capture(TEXT("fire-controls.png"));
		Step = 4;
		Next = Now + 2.;
		return;
	}
	if (Step == 4)
	{
		Camera(FVector(5000., 4450., 380.), FVector(5000., 5000., 45.));
		Step = 5;
		Next = Now + 3.;
		return;
	}
	if (Step == 5)
	{
		Capture(TEXT("fire-outdoor.png"));
		if (!Check(Execute(ECCLExperimentAction::FireRain, TEXT("Zone_10")) && Runtime->ChangeTimeScale(1., Error)
			&& Execute(ECCLExperimentAction::CycleOpening, TEXT("Zone_11"))))
		{
			return;
		}
		Step = 6;
		Next = Now + 4.;
		return;
	}
	if (Step == 6)
	{
		if (!Check(!Outdoor->Fire.Cells[5].bBurning && Room->Removed.SmokeKg > 0. && Runtime->ChangeTimeScale(0., Error)))
		{
			return;
		}
		auto* Agents = World->GetSubsystem<UCCLAgentWorldSubsystem>();
		TArray<uint8> Saved, Before, After;
		if (!Check(Runtime->Save(Agents->GetSimulation(), Saved, Error) && Surface->Capture(Before, Error)
			&& Execute(ECCLExperimentAction::CycleOpening, TEXT("Zone_11"))
			&& Runtime->Restore(Agents->GetSimulation(), Saved, Error) && Surface->Capture(After, Error) && Before == After))
		{
			return;
		}
		Camera(FVector(7810., 5180., 220.), FVector(7440., 4970., 90.));
		Step = 7;
		Next = Now + 3.;
		return;
	}
	if (Step == 7)
	{
		Capture(TEXT("fire-indoor.png"));
		Step = 8;
		Next = Now + 2.;
		return;
	}
	if (Step == 8)
	{
		Select(TEXT("Zone_11"));
		Step = 9;
		Next = Now + 2.;
		return;
	}
	if (Step == 9)
	{
		Capture(TEXT("room-controls.png"));
		auto Marker = Outdoor->Forcing;
		Marker.RainMetersPerWorldSecond = 0.12345;
		if (!Check(Runtime->ChangeSurfaceForcing(Outdoor->RegionId, Marker, Error)))
		{
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("CCL_FIRE_ZONE RESTORE_READY"));
		Step = 10;
		Next = Now + 3.;
		return;
	}
	Finish(true, TEXT("zone10 zone11 controls burn rain ventilation and snapshot verified"));
}

void UCCLFireZoneSmokeSubsystem::Finish(bool bSuccess, const FString& Message)
{
	bComplete = 1;
	UE_LOG(LogTemp, Display, TEXT("CCL_FIRE_ZONE %s %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Message);
	if (!bSuccess || (GetWorld() && GetWorld()->GetNetMode() == NM_Standalone))
	{
		FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1, TEXT("CCLFireZoneSmoke"));
	}
}

TStatId UCCLFireZoneSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLFireZoneSmokeSubsystem, STATGROUP_Tickables);
}
