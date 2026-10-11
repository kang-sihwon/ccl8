#include "CCLWorldEnvironmentConfig.h"

#include "Agents/CCLLifeSimulation.h"
#include "EngineUtils.h"

// 부모 인터페이스 함수

bool ACCLWorldEnvironmentConfig::BuildInitialPopulation(FCCLSimulationSnapshot& OutInitial, FString& Error) const
{
	OutInitial = FCCLLifeSimulation::MerchantScenario(PopulationSeed);
	return true;
}

// 내 클래스 함수

bool ACCLWorldEnvironmentConfig::BuildInputs(FKSIEnvironmentInputs& OutInputs, FString& Error) const
{
	if (!CelestialDefinition || Probes.Num() > 8 || ViewOpeningIds.Num() > 64 || ViewSurfaceIds.Num() > 32)
	{
		Error = TEXT("Map environment definition or diagnostic interest budget is invalid.");
		return false;
	}

	FKSIEnvironmentInputs Candidate;
	Candidate.Celestial = CelestialDefinition->Definition;
	Candidate.Observer = Observer;
	Candidate.Surfaces = Surfaces;
	Candidate.Openings = Openings;
	FKSICelestialSystem Celestial;
	FKSISurfaceScene Scene;
	FKSICelestialObservation Observation;
	if (!FKSIEnvironmentInputsCodec::Prepare(Candidate, 0., Celestial, Scene, Observation, Error))
	{
		return false;
	}

	TSet<FName> ProbeIds;
	for (const auto& Probe : Probes)
	{
		FKSIShelterQuery Query;
		Query.BodyId = Observer.BodyId;
		Query.PositionMeters = Probe.PositionMeters;
		Query.ToPrecipitationSource = Probe.ToPrecipitationSource;
		Query.ToWindSource = Probe.ToWindSource;
		FKSIShelterSample Sample;
		if (Probe.ProbeId.IsNone() || ProbeIds.Contains(Probe.ProbeId)
			|| !FKSIShelterEvaluator::Evaluate(Scene, Query, Sample, Error))
		{
			Error = TEXT("Invalid map diagnostic probe.");
			return false;
		}

		ProbeIds.Add(Probe.ProbeId);
	}

	if (!ValidateViewInputs(Candidate, Error))
	{
		return false;
	}

	OutInputs = MoveTemp(Candidate);
	return true;
}

bool ACCLWorldEnvironmentConfig::ValidateViewInputs(const FKSIEnvironmentInputs& Inputs, FString& Error) const
{
	TSet<FGuid> ViewIds;
	for (const FGuid& Id : ViewOpeningIds)
	{
		if (ViewIds.Contains(Id) || !Inputs.Openings.ContainsByPredicate([Id](const auto& Opening) { return Opening.OpeningId == Id; }))
		{
			Error = TEXT("Invalid or duplicate displayed opening.");
			return false;
		}

		ViewIds.Add(Id);
	}

	ViewIds.Reset();
	for (const FGuid& Id : ViewSurfaceIds)
	{
		if (ViewIds.Contains(Id) || !Inputs.Surfaces.ContainsByPredicate([Id](const auto& Surface) { return Surface.SurfaceId == Id; }))
		{
			Error = TEXT("Invalid or duplicate displayed surface.");
			return false;
		}

		ViewIds.Add(Id);
	}

	for (const auto& Opening : Inputs.Openings)
	{
		if (ViewSurfaceIds.Contains(Opening.SurfaceId) && !ViewOpeningIds.Contains(Opening.OpeningId))
		{
			Error = TEXT("Every opening on a displayed surface must be included in its view.");
			return false;
		}
	}

	return true;
}

const ACCLWorldEnvironmentConfig* ACCLWorldEnvironmentConfig::Find(const UWorld* World)
{
	for (TActorIterator<ACCLWorldEnvironmentConfig> It(World); It; ++It)
	{
		return *It;
	}

	return nullptr;
}
