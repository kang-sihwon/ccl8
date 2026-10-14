#include "CCLExperimentDirector.h"

#include "CCLWorldSimulationSubsystem.h"

FKSISurfaceGrid ACCLExperimentDirector::FireGrid(bool bIndoor)
{
	FKSISurfaceGrid G;
	G.RegionId = FireRegionId(bIndoor);
	G.OriginMeters = UCCLExperimentDefinition::ZoneCenter(bIndoor ? 11 : 10) / 100. + FVector(-2., -2., 0.);
	G.Size = FIntPoint(4, 4);
	G.SpacingMeters = 1.;
	G.bDynamicShelter = 1;
	G.Cells.SetNum(16);
	G.Fire.Cells.SetNum(16);
	G.Forcing.TemperatureCelsius = 20.;
	G.Forcing.PhaseMetersPerDegreeWorldSecond = 0.;
	G.Forcing.InfiltrationMetersPerWorldSecond = 0.;
	G.Forcing.DrainageMetersPerWorldSecond = 0.;
	G.Forcing.FlowConductance = 0.02;
	FString Error;
	for (int32 I = 0; I < G.Cells.Num(); ++I)
	{
		auto& Cell = G.Cells[I];
		Cell.BedMeters = 0.1;
		Cell.CeilingMeters = bIndoor ? 4. : 10.;
		Cell.SnowCubicMeters = I % 4 == 0 ? 0.002 : 0.;
		Cell.SnowVolumeCubicMeters = Cell.SnowCubicMeters * 10.;
		Cell.SoilCubicMeters = I % 4 == 3 ? 0.002 : 0.;
		FKSICombustionModel::Initialize(G.Fire.Material, 0.5, 20., G.Fire.Cells[I], Error);
	}

	G.Ledger.InitialCubicMeters = G.TotalCubicMeters();
	return G;
}

FKSIAirSpace ACCLExperimentDirector::FireRoom()
{
	FKSIAirSpace Room;
	Room.SpaceId = FireSpaceId();
	Room.ClimateRegionId = FireRegionId(true);
	Room.MinimumMeters = FVector(71., 47., 0.);
	Room.MaximumMeters = FVector(79., 53., 4.);
	Room.VolumeCubicMeters = 192.;
	FString Error;
	Room.Initialize(20., 0.004, Error);
	return Room;
}

bool ACCLExperimentDirector::FireAction(ECCLExperimentAction Action, FName CaseId, FString& Error)
{
	const bool bIndoor = CaseId == TEXT("Zone_11");
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	const auto* G = Runtime ? Runtime->GetSurfaceSimulation().FindRegion(FireRegionId(bIndoor)) : nullptr;
	if ((!bIndoor && CaseId != TEXT("Zone_10")) || !G)
	{
		Error = TEXT("불 또는 실내 구역을 선택하고 준비를 기다려 줘.");
		return false;
	}

	if (Action == ECCLExperimentAction::FireIgnite)
	{
		return Runtime->IgniteSurface(G->RegionId, 5, 600000., Error);
	}

	if (Action == ECCLExperimentAction::ShelterWind && bIndoor)
	{
		return Runtime->ChangeSurfaceWind(G->RegionId, G->Weather.WindMetersPerSecond > 0. ? 0. : 8., Error);
	}

	if (Action != ECCLExperimentAction::FireRain)
	{
		Error = TEXT("실내는 지붕이 비를 막는다. 실외 불 구역에서 비를 조작해 줘.");
		return false;
	}

	auto Forcing = G->Forcing;
	Forcing.RainMetersPerWorldSecond = Forcing.RainMetersPerWorldSecond > 0. ? 0. : (bIndoor ? 0.00002 : 0.001);
	return Runtime->ChangeSurfaceForcing(G->RegionId, Forcing, Error);
}

bool ACCLExperimentDirector::RunFire(bool bIndoor, FString& Error)
{
	const auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	const auto& Inputs = Runtime->GetEnvironmentInputs();
	FKSISurfaceSimulation Candidate;
	FKSICelestialSystem Celestial;
	if (!Candidate.Initialize(FGuid::NewGuid(), 0., 0., 0, Error) || !Candidate.AddRegion(FireGrid(bIndoor), Error)
		|| (bIndoor && !Candidate.AddAirSpace(FireRoom(), Error)) || !Celestial.Initialize(Inputs.Celestial, Error)
		|| !Candidate.Ignite(FireRegionId(bIndoor), 5, 600000., Error))
	{
		return false;
	}

	FKSIWorldStep Step;
	Step.StepId = 1;
	Step.GameToSeconds = 10.;
	if (!Candidate.Advance(Step, Error, &Celestial, &Inputs.Observer, &Runtime->GetSurfaceProvider(), &Inputs.Openings)
		|| Candidate.FindRegion(FireRegionId(bIndoor))->Fire.Cells[5].BurnedKg <= 0.)
	{
		return false;
	}

	if (bIndoor)
	{
		const auto* Room = Candidate.GetAirSimulation().FindSpace(FireSpaceId());
		if (!Room || Room->Contents.SmokeKg <= 0. || Room->TemperatureCelsius() <= 20.)
		{
			Error = TEXT("실내 연소의 열·연기 전달을 확인하지 못했다.");
			return false;
		}
	}

	TArray<uint8> Before, After;
	FKSISurfaceSimulation Restored;
	return Candidate.Validate(Error) && Candidate.Capture(Before, Error) && Restored.Restore(Before, Error)
		&& Restored.Capture(After, Error) && Before == After;
}
