#include "CCLWorldAdvance.h"

#include "CCLWorldClock.h"
#include "CCLEcologySimulation.h"
#include "CCLSurfaceSimulation.h"
#include "Agents/CCLLifeSimulation.h"
#include "Agents/CCLAgentTags.h"
#include "ProfilingDebugging/CsvProfiler.h"

bool CCLWorldAdvance::Advance(FCCLWorldClock& Clock, FCCLLifeSimulation& Life,
	double MaxGameSeconds, double MaxWorldSeconds, FString& Error, int32 MaxLifeSlices, FCCLSurfaceSimulation* Surface,
	const FCCLCelestialSystem* Celestial, const FCCLCelestialObserver* Observer, const ICCLSurfaceProvider* Shelter,
	const TArray<FCCLSurfaceOpening>* Openings, FCCLEcologySimulation* Ecology, int32 MaxEcologyBoundaries, const ICCLEcologySpace* Space,
	const TFunction<bool(const FKSIWorldStep&, const FCCLSurfaceSimulation*, FString&)>& Prepare)
{
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(CCLWorldAdvance);
	Error.Reset();
	if (Clock.GetWorldSeconds() != Life.GetTime()
		|| (Ecology && (Ecology->GetWorldSeconds() != Clock.GetWorldSeconds()
			|| (Surface && Ecology->GetWorldId() != Surface->GetWorldId()))))
	{
		Error = TEXT("Clock, ecology and life must begin at the same committed time and world.");
		return false;
	}

	if (Ecology)
	{
		for (const auto& Pair : Ecology->GetIndividuals())
		{
			const auto* Agent = Life.Find(Pair.Key);
			if (Agent && Agent->Features.Contains(CCLAgentTags::Feature_Biology))
			{
				Error = TEXT("An individual cannot have both ecology and Agent biology ownership.");
				return false;
			}
		}
	}

	if (Ecology && Ecology->HasSpatialBehavior())
	{
		MaxWorldSeconds = FMath::Min(MaxWorldSeconds, FMath::FloorToDouble(Clock.GetWorldSeconds()) + 1. - Clock.GetWorldSeconds());
	}

	FCCLWorldClock CandidateClock = Clock;
	FKSIWorldStep Step;
	if (!CandidateClock.Prepare(MaxGameSeconds, MaxWorldSeconds, Step, Error) ||
		!CandidateClock.Commit(Step.Ticket, Error))
	{
		return false;
	}

	FCCLSurfaceSimulation CandidateSurface;
	if (Surface)
	{
		CSV_SCOPED_TIMING_STAT_EXCLUSIVE(CCLSurfaceAdvance);
		CandidateSurface = *Surface;
		if (!CandidateSurface.Advance(Step, Error, Celestial, Observer, Shelter, Openings))
		{
			return false;
		}
	}

	FCCLEcologySimulation CandidateEcology;
	if (Ecology)
	{
		CSV_SCOPED_TIMING_STAT_EXCLUSIVE(CCLEcologyAdvance);
		CandidateEcology = *Ecology;
		if (!CandidateEcology.AdvanceTo(Step.WorldToSeconds, Error, MaxEcologyBoundaries, Space, Surface ? &CandidateSurface : nullptr))
		{
			return false;
		}
	}

	if (Prepare && !Prepare(Step, Surface ? &CandidateSurface : nullptr, Error))
	{
		return false;
	}

	if (!Life.TryAdvanceTo(Step.WorldToSeconds, Error, MaxLifeSlices))
	{
		return false;
	}

	if (Surface)
	{
		*Surface = MoveTemp(CandidateSurface);
	}

	if (Ecology)
	{
		*Ecology = MoveTemp(CandidateEcology);
	}

	Clock = MoveTemp(CandidateClock);
	return true;
}
