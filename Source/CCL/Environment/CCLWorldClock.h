#pragma once

#include "CoreMinimal.h"

struct FCCLWorldTimeRate
{
	double GameSeconds = 0;
	double WorldSeconds = 0;
	double Scale = 60;
};

struct FCCLPendingWorldTime
{
	double GameSeconds = 0;
	double GameToSeconds = 0;
	double WorldToSeconds = 0;
	double Scale = 60;
};

struct FCCLWorldClockSnapshot
{
	uint32 Version = 1;
	uint64 CompletedStepId = 0;
	double GameSeconds = 0;
	double WorldSeconds = 0;
	double AcceptedGameSeconds = 0;
	double RequestedWorldSeconds = 0;
	double TimeScale = 60;
	TArray<FCCLPendingWorldTime> Pending;
	TArray<FCCLWorldTimeRate> Rates;
};

struct FCCLWorldStep
{
	FGuid Ticket;
	uint64 StepId = 0;
	double GameFromSeconds = 0;
	double GameToSeconds = 0;
	double WorldFromSeconds = 0;
	double WorldToSeconds = 0;
};

// A single server owner queues game time and publishes a step only after its consumers commit.
// This value object neither ticks nor advances Agent state on its own.
class CCL_API FCCLWorldClock
{
public:
	FCCLWorldClock();

public:
	bool Reset(double InitialWorldSeconds, double InitialScale, FString& Error);
	bool QueueGameTime(double DeltaSeconds, bool bPaused, FString& Error);
	bool ChangeTimeScale(double NewScale, FString& Error);
	bool Prepare(double MaxGameSeconds, double MaxWorldSeconds, FCCLWorldStep& OutStep, FString& Error);
	bool Commit(const FGuid& Ticket, FString& Error);
	bool Abort(const FGuid& Ticket, FString& Error);
	bool Capture(FCCLWorldClockSnapshot& OutSnapshot, FString& Error) const;
	bool Restore(const FCCLWorldClockSnapshot& Snapshot, FString& Error);

	double GetGameSeconds() const { return State.GameSeconds; }
	double GetWorldSeconds() const { return State.WorldSeconds; }
	double GetTimeScale() const { return State.TimeScale; }
	double GetPendingGameSeconds() const { return State.AcceptedGameSeconds - State.GameSeconds; }
	uint64 GetCompletedStepId() const { return State.CompletedStepId; }
	bool HasPendingTime() const { return !State.Pending.IsEmpty(); }
	bool HasPreparedStep() const { return Prepared.Ticket.IsValid(); }

private:
	static bool Validate(const FCCLWorldClockSnapshot& Snapshot, FString& Error);

private:
	FCCLWorldClockSnapshot State;
	FCCLWorldStep Prepared;
};
