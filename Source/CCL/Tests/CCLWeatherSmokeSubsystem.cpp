#include "CCLWeatherSmokeSubsystem.h"

#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLExperimentPlayerController.h"
#include "Environment/CCLExperimentScreen.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Environment/CCLWorldEnvironmentState.h"
#include "EngineUtils.h"
#include "Environment/CCLSurfaceReplication.h"
#include "Environment/CCLWeatherPresentation.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "UI/CCLGameUI.h"
#include "UI/Core/CCLUISubsystem.h"
#include "Engine/World.h"
#include "UnrealClient.h"

bool UCCLWeatherSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLWeatherSmoke"));
#endif
}

void UCCLWeatherSmokeSubsystem::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	if (!Started)
	{
		Started = Now;
	}
	if (Now - Started > 180.)
	{
		Finish(false, FString::Printf(TEXT("timeout step=%d"), Step));
		return;
	}
	auto* World = GetWorld();
	auto* Director = World ? ACCLExperimentDirector::Find(World) : nullptr;
	auto* Runtime = World ? World->GetSubsystem<UCCLWorldSimulationSubsystem>() : nullptr;
	const auto* Surface = World ? UCCLSurfaceReplication::View(World) : nullptr;
	const auto* Grid = Surface ? Surface->FindRegion(ACCLExperimentDirector::WeatherRegionId()) : nullptr;
	if (!Director || !Director->IsReady() || !Grid || Now < Next)
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
		bool bFrozenSnapshot = false;
		for (TActorIterator<ACCLWorldEnvironmentState> It(World); It; ++It)
		{
			const auto& Time = It->GetTime();
			bFrozenSnapshot |= Time.TimeScale == 0. && Surface->GetWorldSeconds() == Time.WorldSeconds;
		}
		if (!bFrozenSnapshot)
		{
			return;
		}
		if (Grid->Ledger.RainCubicMeters + Grid->Ledger.SnowCubicMeters <= 0.0001)
		{
			return;
		}
		FCCLClimate Climate = Grid->Climate;
		if (!Check(Grid->Climate.bEnabled && Grid->bDynamicShelter && Grid->Cells[0].RainExposure == 0. && Grid->Cells.Last().RainExposure == 1. && Grid->Weather.VaporMeters > 0. && FCCLSurfaceSimulation::ValidateRegion(*Grid, Error)
			&& !Runtime->ChangeClimate(Grid->RegionId, Climate, Error)))
		{
			return;
		}
		Finish(true, FString::Printf(TEXT("client natural weather received T=%.6f vapor=%.9f cloud=%.9f"),
			Grid->Weather.TemperatureCelsius, Grid->Weather.VaporMeters, Grid->Weather.CloudMeters));
		return;
	}

	auto* PC = Cast<ACCLExperimentPlayerController>(World->GetFirstPlayerController());
	auto Execute = [&](ECCLExperimentAction Action)
	{
		return Director->Execute(PC, Action, TEXT("Zone_02"), Director->GetGeneration(),
			Director->FindResult(TEXT("Zone_02"))->RunId, Error);
	};
	const bool bRendered = FParse::Param(FCommandLine::Get(), TEXT("CCLWeatherCapture"));
	if (Step == 0)
	{
		if (!Check(Execute(ECCLExperimentAction::WeatherRain) && !Surface->FindRegion(Grid->RegionId)->Climate.bEnabled
			&& Execute(ECCLExperimentAction::WeatherNatural) && Surface->FindRegion(Grid->RegionId)->Climate.bEnabled
			&& Execute(ECCLExperimentAction::Start)))
		{
			return;
		}
		WorldStarted = Runtime->GetClock().GetWorldSeconds();
		UE_LOG(LogTemp, Display, TEXT("CCL_WEATHER_SMOKE BASE_READY"));
		Step = 1;
		return;
	}
	if (Step == 1)
	{
		const auto* Result = Director->FindResult(TEXT("Zone_02"));
		if (!Check(Result->Status != ECCLExperimentStatus::Failed))
		{
			return;
		}
		if (Runtime->GetClock().GetWorldSeconds() - WorldStarted < 900. || Result->Status != ECCLExperimentStatus::Passed)
		{
			return;
		}
		if (!Check(Grid->Ledger.RainCubicMeters + Grid->Ledger.SnowCubicMeters > 0.0001
			&& Grid->bDynamicShelter && Grid->Cells[0].RainExposure == 0. && Grid->Cells.Last().RainExposure == 1.
			&& FCCLSurfaceSimulation::ValidateRegion(*Grid, Error)))
		{
			return;
		}
		auto* Agents = World->GetSubsystem<UCCLAgentWorldSubsystem>();
		TArray<uint8> Saved, Before, After;
		if (!Check(Runtime->ChangeTimeScale(0., Error) && Runtime->Save(Agents->GetSimulation(), Saved, Error)
			&& Surface->Capture(Before, Error) && Execute(ECCLExperimentAction::WeatherSnow)
			&& Runtime->Restore(Agents->GetSimulation(), Saved, Error) && Surface->Capture(After, Error) && Before == After))
		{
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("CCL_WEATHER_SMOKE RESTORE_READY"));
		Step = 2;
		Next = Now + 1.;
		return;
	}
	if (Step == 2 && bRendered)
	{
		auto* UI = CCLGameUI::Get(PC);
		auto* Screen = UI ? Cast<UCCLExperimentScreen>(UI->FindScreen(PC->GetExperimentView())) : nullptr;
		if (!Screen)
		{
			return;
		}
		if (!Check(Execute(ECCLExperimentAction::Teleport)))
		{
			return;
		}
		Screen->SelectCase(TEXT("Zone_02"));
		Step = 3;
		Next = Now + 1.;
		return;
	}
	if (Step == 3)
	{
		FString Directory;
		FParse::Value(FCommandLine::Get(), TEXT("CCLWeatherSmokeDir="), Directory);
		FScreenshotRequest::RequestScreenshot(Directory / TEXT("weather-controls.png"), true, false);
		Step = 4;
		Next = Now + 2.;
		return;
	}
	if (bRendered && Step >= 4 && Step <= 7)
	{
		FString Directory;
		FParse::Value(FCommandLine::Get(), TEXT("CCLWeatherSmokeDir="), Directory);
		if (Step == 4 || Step == 6)
		{
			if (!Check(Execute(Step == 4 ? ECCLExperimentAction::WeatherRain : ECCLExperimentAction::WeatherSnow))) { return; }
			Next = Now + 4.;
		}
		else
		{
			FScreenshotRequest::RequestScreenshot(Directory / (Step == 5 ? TEXT("weather-rain.png") : TEXT("weather-snow.png")), true, false);
			Next = Now + 2.;
		}
		++Step;
		return;
	}
	if (bRendered && Step == 8)
	{
		auto Climate = Grid->Climate;
		Climate.bEnabled = 1;
		Climate.MoistureSupplyMetersPerSecond = 0.00003;
		if (!Check(Runtime->ChangeClimate(Grid->RegionId, Climate, Error) && Runtime->ChangeTimeScale(60., Error))) { return; }
		WorldStarted = Runtime->GetClock().GetWorldSeconds();
		Step = 9;
		return;
	}
	if (bRendered && Step == 9)
	{
		if (Runtime->GetClock().GetWorldSeconds() - WorldStarted < 900.) { return; }
		if (!Check(Runtime->ChangeTimeScale(0., Error) && ACCLWeatherPresentation::GetCloudFraction(World) > 0.2f)) { return; }
		PC->SetControlRotation(FRotator(35., PC->GetControlRotation().Yaw, 0.));
		Step = 10;
		Next = Now + 4.;
		return;
	}
	if (bRendered && Step == 10)
	{
		FString Directory;
		FParse::Value(FCommandLine::Get(), TEXT("CCLWeatherSmokeDir="), Directory);
		FScreenshotRequest::RequestScreenshot(Directory / TEXT("weather-clouds.png"), true, false);
		Step = 11;
		Next = Now + 2.;
		return;
	}
	Finish(true, FString::Printf(TEXT("server natural weather T=%.6f vapor=%.9f cloud=%.9f"),
		Grid->Weather.TemperatureCelsius, Grid->Weather.VaporMeters, Grid->Weather.CloudMeters));
}

void UCCLWeatherSmokeSubsystem::Finish(bool bSuccess, const FString& Message)
{
	bComplete = 1;
	UE_LOG(LogTemp, Display, TEXT("CCL_WEATHER_SMOKE %s %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Message);
	if (!bSuccess || (GetWorld() && GetWorld()->GetNetMode() == NM_Standalone))
	{
		FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1, TEXT("CCLWeatherSmoke"));
	}
}

TStatId UCCLWeatherSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLWeatherSmokeSubsystem, STATGROUP_Tickables);
}
