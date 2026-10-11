#include "CCLExperimentDirector.h"

#include "CCLWorldSimulationSubsystem.h"

bool ACCLExperimentDirector::VegetationGrid(double WorldSeconds, FCCLSurfaceGrid& Grid, FString& Error)
{
	FCCLSurfaceGrid G;
	G.RegionId = VegetationRegionId();
	G.OriginMeters = UCCLExperimentDefinition::ZoneCenter(7) / 100. + FVector(-6.75, -2.25, 0.);
	G.Size = FIntPoint(9, 3);
	G.SpacingMeters = 1.5;
	G.Cells.SetNum(27);
	G.Forcing.TemperatureCelsius = 20.;
	G.Forcing.FlowConductance = 0.;
	G.Forcing.DrainageMetersPerWorldSecond = 0.;
	G.Forcing.InfiltrationMetersPerWorldSecond = 0.00005;
	TArray<FCCLVegetationSpecies> Species;
	TArray<FCCLVegetationSeed> Seeds;
	for (int32 I = 0; I < 9; ++I)
	{
		FCCLVegetationSpecies S;
		S.Id = FGuid(7011, 2, 1, I + 1);
		S.Kind = ECCLVegetationKind(I);
		S.MaximumBiomassKg = 1.;
		S.GrowthKgPerWorldSecond = 0.0005;
		S.LifespanSeconds = 1800. + I * 90.;
		S.StressDeathSeconds = 600.;
		S.RegrowthDelaySeconds = 300.;
		S.DecayPerWorldSecond = 0.0001;
		S.MinimumDaylightHours = I == 8 ? 0. : 6.;
		Species.Add(S);
	}
	for (int32 I = 0; I < 27; ++I)
	{
		const int32 Kind = I % 9;
		const int32 Row = I / 9;
		FCCLVegetationSeed Seed;
		Seed.Id = FGuid(7011, 3, 1, I + 1);
		Seed.SpeciesIndex = Kind;
		Seed.BiomassKg = 0.15;
		Seed.SoilNutrientsKg = Row == 2 ? 0.001 : 0.2;
		Seed.SoilOrganicKg = Row == 2 ? 0. : 1.;
		Seeds.Add(Seed);
		auto& Cell = G.Cells[I];
		Cell.BedMeters = 0.2;
		Cell.SoilCubicMeters = 0.15;
		Cell.WaterCubicMeters = Kind == 6 || Kind == 7 ? 0.15 : 0.;
		Cell.SunExposure = Row == 1 ? 0.05 : 1.;
		Cell.CeilingMeters = 10.;
		Cell.bOpenBoundary = Kind == 6 || Kind == 7 ? 0 : 1;
	}
	G.Forcing.BoundaryConductance = 0.1;
	G.Forcing.BoundaryLevelMeters = 0.2;
	G.Ledger.InitialCubicMeters = G.TotalCubicMeters();
	if (!CCLSurfaceVegetation::Initialize(G, Species, Seeds, WorldSeconds, Error))
	{
		return false;
	}
	Grid = MoveTemp(G);
	return true;
}

bool ACCLExperimentDirector::VegetationAction(ECCLExperimentAction Action, FName CaseId, FString& Error)
{
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	const auto* G = Runtime ? Runtime->GetSurfaceSimulation().FindRegion(VegetationRegionId()) : nullptr;
	if (CaseId != TEXT("Zone_07") || !G)
	{
		Error = TEXT("식생 구역이 없는 이전 저장은 실험 초기화로 기본 배치를 만들 수 있다.");
		return false;
	}
	if (Action == ECCLExperimentAction::VegetationIgnite)
	{
		return Runtime->IgniteSurface(G->RegionId, 0, 1200000., Error);
	}
	auto F = G->Forcing;
	F.RainMetersPerWorldSecond = Action == ECCLExperimentAction::VegetationRain ? 0.00002 : 0.;
	F.EvaporationMetersPerWorldSecond = Action == ECCLExperimentAction::VegetationDry ? 0.0001 : 0.;
	F.TemperatureCelsius = Action == ECCLExperimentAction::VegetationWinter ? -5. : 20.;
	return Runtime->ChangeSurfaceForcing(G->RegionId, F, Error);
}

bool ACCLExperimentDirector::RunVegetation(FString& Error)
{
	const auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	const auto* G = Runtime->GetSurfaceSimulation().FindRegion(VegetationRegionId());
	if (!G)
	{
		Error = TEXT("식생 구역이 없다. 실험 초기화 후 다시 검사해 줘.");
		return false;
	}
	FCCLSurfaceSimulation Candidate;
	FCCLCelestialSystem Celestial;
	const auto& Inputs = Runtime->GetEnvironmentInputs();
	if (!Candidate.Initialize(Runtime->GetIdentity().WorldId, Runtime->GetClock().GetGameSeconds(),
		Runtime->GetClock().GetWorldSeconds(), Runtime->GetSurfaceSimulation().GetStepId(), Error)
		|| !Candidate.AddRegion(*G, Error) || !Celestial.Initialize(Inputs.Celestial, Error))
	{
		return false;
	}
	FKSIWorldStep Step;
	Step.StepId = Candidate.GetStepId() + 1;
	Step.GameFromSeconds = Candidate.GetGameSeconds();
	Step.GameToSeconds = Step.GameFromSeconds + 1.;
	Step.WorldFromSeconds = Candidate.GetWorldSeconds();
	Step.WorldToSeconds = Step.WorldFromSeconds + 3600.;
	TArray<uint8> Bytes, Again;
	FCCLSurfaceSimulation Restored;
	return Candidate.Advance(Step, Error, &Celestial, &Inputs.Observer)
		&& Candidate.Capture(Bytes, Error) && Restored.Restore(Bytes, Error)
		&& Restored.Capture(Again, Error) && Bytes == Again;
}
