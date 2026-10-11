#include "CCLExperimentDirector.h"

#include "CCLWorldSimulationSubsystem.h"

FKSISurfaceGrid ACCLExperimentDirector::WeatherGrid()
{
	FKSISurfaceGrid G;
	G.RegionId = WeatherRegionId();
	G.OriginMeters = UCCLExperimentDefinition::ZoneCenter(2) / 100. + FVector(-4., -4., 0.);
	G.Size = FIntPoint(16, 16);
	G.SpacingMeters = 0.5;
	G.Climate.bEnabled = 1;
	G.bDynamicShelter = 1;
	G.Cells.SetNum(256);
	for (auto& Cell : G.Cells)
	{
		Cell.BedMeters = 0.2;
		Cell.CeilingMeters = 10.;
		Cell.bOpenBoundary = 1;
	}
	G.Forcing.BoundaryConductance = 0.1;
	G.Forcing.BoundaryLevelMeters = 0.2;
	return G;
}

bool ACCLExperimentDirector::WeatherAction(ECCLExperimentAction Action, FName CaseId, FString& Error)
{
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	const auto* Grid = Runtime ? Runtime->GetSurfaceSimulation().FindRegion(WeatherRegionId()) : nullptr;
	if (CaseId != TEXT("Zone_02") || !Grid)
	{
		Error = TEXT("날씨 구역을 선택하고 준비를 기다려 줘.");
		return false;
	}

	if (Action == ECCLExperimentAction::WeatherNatural)
	{
		auto Climate = Grid->Climate;
		Climate.bEnabled = 1;
		return Runtime->ChangeClimate(WeatherRegionId(), Climate, Error);
	}

	auto Forcing = Grid->Forcing;
	const bool bSnow = Action == ECCLExperimentAction::WeatherSnow;
	Forcing.RainMetersPerWorldSecond = bSnow ? 0. : 0.00002;
	Forcing.SnowMetersPerWorldSecond = bSnow ? 0.00002 : 0.;
	Forcing.TemperatureCelsius = bSnow ? -8. : 12.;
	Forcing.EvaporationMetersPerWorldSecond = 0.;
	return Runtime->ChangeSurfaceForcing(WeatherRegionId(), Forcing, Error);
}

bool ACCLExperimentDirector::RunWeather(FString& Error)
{
	const auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	FKSISurfaceSimulation Candidate = Runtime->GetSurfaceSimulation();
	const auto* Grid = Candidate.FindRegion(WeatherRegionId());
	if (!Grid)
	{
		Error = TEXT("날씨 표면을 아직 준비하지 못했다.");
		return false;
	}

	auto Climate = Grid->Climate;
	Climate.bEnabled = 1;
	FKSICelestialSystem Celestial;
	const auto& Inputs = Runtime->GetEnvironmentInputs();
	if (!Candidate.ChangeClimate(WeatherRegionId(), Climate, Error) || !Celestial.Initialize(Inputs.Celestial, Error))
	{
		return false;
	}

	for (int32 I = 0; I < 60; ++I)
	{
		FKSIWorldStep Step;
		Step.StepId = Candidate.GetStepId() + 1;
		Step.GameFromSeconds = Candidate.GetGameSeconds();
		Step.GameToSeconds = Step.GameFromSeconds + 0.25;
		Step.WorldFromSeconds = Candidate.GetWorldSeconds();
		Step.WorldToSeconds = Step.WorldFromSeconds + 15.;
		if (!Candidate.Advance(Step, Error, &Celestial, &Inputs.Observer, &Runtime->GetSurfaceProvider(), &Inputs.Openings))
		{
			return false;
		}
	}

	TArray<uint8> Bytes, Again;
	FKSISurfaceSimulation Restored;
	return Candidate.Validate(Error) && Candidate.Capture(Bytes, Error) && Restored.Restore(Bytes, Error)
		&& Restored.Capture(Again, Error) && Bytes == Again;
}
