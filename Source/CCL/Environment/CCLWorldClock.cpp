#include "CCLWorldClock.h"

#include <limits>

namespace
{
	constexpr int32 MaxPendingSegments = 256;
	constexpr int32 MaxRateChanges = 4096;

	bool NonNegativeFinite(double Value)
	{
		return FMath::IsFinite(Value) && Value >= 0;
	}

	bool Matches(double A, double B)
	{
		return FMath::Abs(A - B) <= FMath::Max(1.e-8, FMath::Max(FMath::Abs(A), FMath::Abs(B)) * 1.e-12);
	}

	bool SameBoundary(double A, double B)
	{
		return FMath::Abs(A - B) <= 8 * std::numeric_limits<double>::epsilon() * FMath::Max(FMath::Abs(A), FMath::Abs(B));
	}

	bool Reject(FString& Error, const TCHAR* Reason)
	{
		Error = Reason;
		return false;
	}
}

FCCLWorldClock::FCCLWorldClock()
{
	State.Rates.Add(FCCLWorldTimeRate{});
}

bool FCCLWorldClock::Reset(double InitialWorldSeconds, double InitialScale, FString& Error)
{
	Error.Reset();
	if (!NonNegativeFinite(InitialWorldSeconds) || !NonNegativeFinite(InitialScale))
	{
		return Reject(Error, TEXT("Invalid initial time or scale."));
	}

	FCCLWorldClockSnapshot Candidate;
	Candidate.WorldSeconds = InitialWorldSeconds;
	Candidate.RequestedWorldSeconds = InitialWorldSeconds;
	Candidate.TimeScale = InitialScale;
	Candidate.Rates.Add({0, InitialWorldSeconds, InitialScale});
	State = MoveTemp(Candidate);
	Prepared = {};
	return true;
}

bool FCCLWorldClock::QueueGameTime(double DeltaSeconds, bool bPaused, FString& Error)
{
	Error.Reset();
	if (!NonNegativeFinite(DeltaSeconds))
	{
		return Reject(Error, TEXT("Game delta must be finite and nonnegative."));
	}

	if (bPaused || DeltaSeconds == 0)
	{
		return true;
	}

	const double GameTo = State.AcceptedGameSeconds + DeltaSeconds;
	const double WorldDelta = DeltaSeconds * State.TimeScale;
	const double WorldTo = State.RequestedWorldSeconds + WorldDelta;
	if (!NonNegativeFinite(GameTo) || !NonNegativeFinite(WorldTo) || GameTo <= State.AcceptedGameSeconds ||
		(State.TimeScale > 0 && (WorldDelta <= 0 || WorldTo <= State.RequestedWorldSeconds)))
	{
		return Reject(Error, TEXT("Time overflow or insufficient clock precision."));
	}

	const bool bMerge = !State.Pending.IsEmpty() && State.Pending.Last().Scale == State.TimeScale;
	if (!bMerge && State.Pending.Num() >= MaxPendingSegments)
	{
		return Reject(Error, TEXT("Pending time budget exhausted; input was not accepted."));
	}

	if (bMerge)
	{
		auto& Last = State.Pending.Last();
		const double Start = State.Pending.Num() == 1 ? State.GameSeconds : State.Pending[State.Pending.Num() - 2].GameToSeconds;
		Last.GameSeconds = GameTo - Start;
		Last.GameToSeconds = GameTo;
		Last.WorldToSeconds = WorldTo;
	}
	else
	{
		State.Pending.Add({GameTo - State.AcceptedGameSeconds, GameTo, WorldTo, State.TimeScale});
	}

	State.AcceptedGameSeconds = GameTo;
	State.RequestedWorldSeconds = WorldTo;
	return true;
}

bool FCCLWorldClock::ChangeTimeScale(double NewScale, FString& Error)
{
	Error.Reset();
	if (!NonNegativeFinite(NewScale))
	{
		return Reject(Error, TEXT("Time scale must be finite and nonnegative."));
	}

	if (NewScale == State.TimeScale)
	{
		return true;
	}

	// Coalesce changes at the same input boundary; no interval used the replaced rate.
	if (State.Rates.Last().GameSeconds == State.AcceptedGameSeconds)
	{
		if (State.Rates.Num() > 1 && State.Rates[State.Rates.Num() - 2].Scale == NewScale)
		{
			State.Rates.Pop();
		}
		else
		{
			State.Rates.Last().Scale = NewScale;
		}
	}
	else
	{
		if (State.Rates.Num() >= MaxRateChanges)
		{
			return Reject(Error, TEXT("Rate history budget exhausted; scale was not changed."));
		}

		State.Rates.Add({State.AcceptedGameSeconds, State.RequestedWorldSeconds, NewScale});
	}

	State.TimeScale = NewScale;
	return true;
}

bool FCCLWorldClock::Prepare(double MaxGameSeconds, double MaxWorldSeconds, FCCLWorldStep& OutStep, FString& Error)
{
	Error.Reset();
	OutStep = {};
	if (HasPreparedStep() || State.Pending.IsEmpty())
	{
		return Reject(Error, TEXT("A step is already prepared or no time is pending."));
	}

	if (!NonNegativeFinite(MaxGameSeconds) || MaxGameSeconds == 0 ||
		!NonNegativeFinite(MaxWorldSeconds) || MaxWorldSeconds == 0 || State.CompletedStepId == MAX_uint64)
	{
		return Reject(Error, TEXT("Invalid step budget or exhausted step sequence."));
	}

	const auto& Pending = State.Pending[0];
	double Delta = FMath::Min(Pending.GameSeconds, MaxGameSeconds);
	if (Pending.Scale > 0)
	{
		Delta = FMath::Min(Delta, MaxWorldSeconds / Pending.Scale);
	}

	FCCLWorldStep Candidate;
	Candidate.StepId = State.CompletedStepId + 1;
	Candidate.GameFromSeconds = State.GameSeconds;
	Candidate.GameToSeconds = State.GameSeconds + Delta;
	Candidate.WorldFromSeconds = State.WorldSeconds;
	Candidate.WorldToSeconds = State.WorldSeconds + Delta * Pending.Scale;
	if (Delta == Pending.GameSeconds)
	{
		// Preserve the accepted boundary exactly when finishing an input segment.
		Candidate.GameToSeconds = Pending.GameToSeconds;
		Candidate.WorldToSeconds = Pending.WorldToSeconds;
	}

	if (Delta <= 0 || !NonNegativeFinite(Candidate.GameToSeconds) || !NonNegativeFinite(Candidate.WorldToSeconds) ||
		Candidate.GameToSeconds <= State.GameSeconds || (Pending.Scale > 0 && Candidate.WorldToSeconds <= State.WorldSeconds))
	{
		return Reject(Error, TEXT("Step cannot advance at this precision; time remains pending."));
	}

	Candidate.Ticket = FGuid::NewGuid();
	Prepared = Candidate;
	OutStep = Candidate;
	return true;
}

bool FCCLWorldClock::Commit(const FGuid& Ticket, FString& Error)
{
	Error.Reset();
	if (!Ticket.IsValid() || Ticket != Prepared.Ticket)
	{
		return Reject(Error, TEXT("Unknown, stale or already committed step ticket."));
	}

	auto& Pending = State.Pending[0];
	if (Prepared.GameToSeconds >= Pending.GameToSeconds)
	{
		State.Pending.RemoveAt(0);
	}
	else
	{
		Pending.GameSeconds = Pending.GameToSeconds - Prepared.GameToSeconds;
	}

	State.GameSeconds = Prepared.GameToSeconds;
	State.WorldSeconds = Prepared.WorldToSeconds;
	State.CompletedStepId = Prepared.StepId;
	Prepared = {};
	return true;
}

bool FCCLWorldClock::Abort(const FGuid& Ticket, FString& Error)
{
	Error.Reset();
	if (!Ticket.IsValid() || Ticket != Prepared.Ticket)
	{
		return Reject(Error, TEXT("Unknown or stale step ticket."));
	}

	Prepared = {};
	return true;
}

bool FCCLWorldClock::Capture(FCCLWorldClockSnapshot& OutSnapshot, FString& Error) const
{
	Error.Reset();
	if (HasPreparedStep())
	{
		return Reject(Error, TEXT("Finish or abort the prepared step before capturing state."));
	}

	if (!Validate(State, Error))
	{
		return false;
	}

	OutSnapshot = State;
	return true;
}

bool FCCLWorldClock::Restore(const FCCLWorldClockSnapshot& Snapshot, FString& Error)
{
	Error.Reset();
	if (!Validate(Snapshot, Error))
	{
		return false;
	}

	State = Snapshot;
	Prepared = {};
	return true;
}

bool FCCLWorldClock::Validate(const FCCLWorldClockSnapshot& Snapshot, FString& Error)
{
	if (Snapshot.Version != 1 || !NonNegativeFinite(Snapshot.GameSeconds) ||
		!NonNegativeFinite(Snapshot.WorldSeconds) || !NonNegativeFinite(Snapshot.AcceptedGameSeconds) ||
		!NonNegativeFinite(Snapshot.RequestedWorldSeconds) || !NonNegativeFinite(Snapshot.TimeScale) ||
		Snapshot.AcceptedGameSeconds < Snapshot.GameSeconds || Snapshot.RequestedWorldSeconds < Snapshot.WorldSeconds ||
		Snapshot.Pending.Num() > MaxPendingSegments || Snapshot.Rates.IsEmpty() || Snapshot.Rates.Num() > MaxRateChanges)
	{
		return Reject(Error, TEXT("Invalid clock snapshot header or storage budget."));
	}

	for (int32 Index = 0; Index < Snapshot.Rates.Num(); ++Index)
	{
		const auto& Rate = Snapshot.Rates[Index];
		if (!NonNegativeFinite(Rate.GameSeconds) || !NonNegativeFinite(Rate.WorldSeconds) ||
			!NonNegativeFinite(Rate.Scale) || Rate.GameSeconds > Snapshot.AcceptedGameSeconds ||
			(Index == 0 && Rate.GameSeconds != 0))
		{
			return Reject(Error, TEXT("Invalid rate history entry."));
		}

		if (Index > 0)
		{
			const auto& Previous = Snapshot.Rates[Index - 1];
			const double Expected = Previous.WorldSeconds + (Rate.GameSeconds - Previous.GameSeconds) * Previous.Scale;
			if (Rate.GameSeconds <= Previous.GameSeconds || !NonNegativeFinite(Expected) || !Matches(Rate.WorldSeconds, Expected))
			{
				return Reject(Error, TEXT("Discontinuous rate history."));
			}
		}
	}

	const auto& LastRate = Snapshot.Rates.Last();
	const double Requested = LastRate.WorldSeconds + (Snapshot.AcceptedGameSeconds - LastRate.GameSeconds) * LastRate.Scale;
	if (LastRate.Scale != Snapshot.TimeScale || !NonNegativeFinite(Requested) || !Matches(Requested, Snapshot.RequestedWorldSeconds))
	{
		return Reject(Error, TEXT("Requested time does not match rate history."));
	}

	int32 RateIndex = 0;
	while (RateIndex + 1 < Snapshot.Rates.Num() && Snapshot.Rates[RateIndex + 1].GameSeconds <= Snapshot.GameSeconds)
	{
		++RateIndex;
	}

	const auto& CompletedRate = Snapshot.Rates[RateIndex];
	const double CompletedWorld = CompletedRate.WorldSeconds + (Snapshot.GameSeconds - CompletedRate.GameSeconds) * CompletedRate.Scale;
	if (!NonNegativeFinite(CompletedWorld) || !Matches(CompletedWorld, Snapshot.WorldSeconds))
	{
		return Reject(Error, TEXT("Completed time does not match rate history."));
	}

	double Game = Snapshot.GameSeconds;
	double World = Snapshot.WorldSeconds;
	for (const auto& Pending : Snapshot.Pending)
	{
		if (!NonNegativeFinite(Pending.GameSeconds) || Pending.GameSeconds == 0 || !NonNegativeFinite(Pending.Scale) ||
			!NonNegativeFinite(Pending.GameToSeconds) || !NonNegativeFinite(Pending.WorldToSeconds))
		{
			return Reject(Error, TEXT("Invalid pending time interval."));
		}

		while (RateIndex + 1 < Snapshot.Rates.Num() &&
			(Snapshot.Rates[RateIndex + 1].GameSeconds <= Game ||
				(Pending.Scale == Snapshot.Rates[RateIndex + 1].Scale && SameBoundary(Snapshot.Rates[RateIndex + 1].GameSeconds, Game))))
		{
			++RateIndex;
		}

		const double End = Pending.GameToSeconds;
		if (End <= Game || !SameBoundary(Game + Pending.GameSeconds, End) ||
			!Matches(World + Pending.GameSeconds * Pending.Scale, Pending.WorldToSeconds))
		{
			return Reject(Error, TEXT("Pending duration does not match its accepted boundary."));
		}

		if (Pending.Scale != Snapshot.Rates[RateIndex].Scale ||
			(RateIndex + 1 < Snapshot.Rates.Num() && End > Snapshot.Rates[RateIndex + 1].GameSeconds &&
				!SameBoundary(End, Snapshot.Rates[RateIndex + 1].GameSeconds)))
		{
			return Reject(Error, TEXT("Pending time crosses or contradicts a rate change."));
		}

		Game = End;
		World = Pending.WorldToSeconds;
	}

	if (!NonNegativeFinite(Game) || !NonNegativeFinite(World) ||
		Game != Snapshot.AcceptedGameSeconds || World != Snapshot.RequestedWorldSeconds)
	{
		return Reject(Error, TEXT("Pending time does not reconcile with accepted input."));
	}

	return true;
}
