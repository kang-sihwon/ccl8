#include "CCLWorldAdvance.h"

#include "CCLWorldClock.h"
#include "Agents/CCLLifeSimulation.h"

bool CCLWorldAdvance::Advance(FCCLWorldClock& Clock, FCCLLifeSimulation& Life,
	double MaxGameSeconds, double MaxWorldSeconds, FString& Error, int32 MaxLifeSlices)
{
	Error.Reset();
	if (Clock.GetWorldSeconds() != Life.GetTime())
	{
		Error = TEXT("Clock and life must begin at the same committed time.");
		return false;
	}

	FCCLWorldClock CandidateClock = Clock;
	FCCLWorldStep Step;
	if (!CandidateClock.Prepare(MaxGameSeconds, MaxWorldSeconds, Step, Error) ||
		!CandidateClock.Commit(Step.Ticket, Error))
	{
		return false;
	}

	if (!Life.TryAdvanceTo(Step.WorldToSeconds, Error, MaxLifeSlices))
	{
		return false;
	}

	Clock = MoveTemp(CandidateClock);
	return true;
}
