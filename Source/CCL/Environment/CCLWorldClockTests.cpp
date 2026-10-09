#include "CCLWorldClock.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWorldClockCommitTest, "CCL.Environment.Clock.CommitAndRetry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLWorldClockCommitTest::RunTest(const FString& Parameters)
{
	FCCLWorldClock Clock;
	FString Error;
	FCCLWorldStep First;
	TestTrue(TEXT("accept a game second"), Clock.QueueGameTime(1, false, Error));
	TestTrue(TEXT("prepare bounded world interval"), Clock.Prepare(1, 15, First, Error));
	TestEqual(TEXT("world budget limits game delta"), First.GameToSeconds, 0.25);
	TestEqual(TEXT("preparation does not publish time"), Clock.GetWorldSeconds(), 0.);
	FCCLWorldClockSnapshot Snapshot;
	TestFalse(TEXT("cannot save an in-flight step"), Clock.Capture(Snapshot, Error));
	FCCLWorldStep Concurrent;
	TestFalse(TEXT("only one step can be in flight"), Clock.Prepare(1, 15, Concurrent, Error));
	TestFalse(TEXT("foreign completion is rejected"), Clock.Commit(FGuid::NewGuid(), Error));
	TestTrue(TEXT("failed consumer aborts its candidate"), Clock.Abort(First.Ticket, Error));
	TestEqual(TEXT("failure leaves completed time unchanged"), Clock.GetWorldSeconds(), 0.);
	FCCLWorldStep Retry;
	TestTrue(TEXT("same input can be retried"), Clock.Prepare(1, 15, Retry, Error));
	TestEqual(TEXT("retry preserves target"), Retry.WorldToSeconds, First.WorldToSeconds);
	TestTrue(TEXT("retry has a new ticket"), Retry.Ticket != First.Ticket);
	TestFalse(TEXT("late completion cannot commit retry"), Clock.Commit(First.Ticket, Error));
	TestTrue(TEXT("publish committed consumer state"), Clock.Commit(Retry.Ticket, Error));
	TestFalse(TEXT("duplicate completion rejected"), Clock.Commit(Retry.Ticket, Error));
	TestEqual(TEXT("only completed world interval is visible"), Clock.GetWorldSeconds(), 15.);
	TestTrue(TEXT("unprocessed input remains pending"), Clock.HasPendingTime());
	TestTrue(TEXT("quiescent snapshot includes pending interval"), Clock.Capture(Snapshot, Error));
	if (TestEqual(TEXT("one pending interval captured"), Snapshot.Pending.Num(), 1))
	{
		TestEqual(TEXT("pending duration retained"), Snapshot.Pending[0].GameSeconds, 0.75);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWorldClockRateTest, "CCL.Environment.Clock.RateHistoryAndPause",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLWorldClockRateTest::RunTest(const FString& Parameters)
{
	FCCLWorldClock Clock;
	FString Error;
	TestTrue(TEXT("old rate input"), Clock.QueueGameTime(1, false, Error));
	TestTrue(TEXT("new rate does not reinterpret pending input"), Clock.ChangeTimeScale(10, Error));
	TestTrue(TEXT("new rate input"), Clock.QueueGameTime(2, false, Error));
	TestTrue(TEXT("world pause ignores elapsed wall time"), Clock.QueueGameTime(100, true, Error));
	FCCLWorldStep Step;
	TestTrue(TEXT("old interval prepared"), Clock.Prepare(100, 1000, Step, Error));
	TestEqual(TEXT("old rate retained"), Step.WorldToSeconds, 60.);
	TestTrue(TEXT("old interval committed"), Clock.Commit(Step.Ticket, Error));
	TestTrue(TEXT("new interval prepared"), Clock.Prepare(100, 1000, Step, Error));
	TestEqual(TEXT("new rate applies only to its interval"), Step.WorldToSeconds, 80.);
	TestTrue(TEXT("new interval committed"), Clock.Commit(Step.Ticket, Error));
	TestEqual(TEXT("pause excluded from game time"), Clock.GetGameSeconds(), 3.);
	TestTrue(TEXT("world time can stop without stopping game time"), Clock.ChangeTimeScale(0, Error));
	TestTrue(TEXT("queue physics time"), Clock.QueueGameTime(1, false, Error));
	TestTrue(TEXT("zero scale can prepare"), Clock.Prepare(1, 1, Step, Error));
	TestTrue(TEXT("zero scale commits"), Clock.Commit(Step.Ticket, Error));
	TestEqual(TEXT("game time advances at zero scale"), Clock.GetGameSeconds(), 4.);
	TestEqual(TEXT("world time remains frozen"), Clock.GetWorldSeconds(), 80.);
	TestFalse(TEXT("nothing remains pending"), Clock.HasPendingTime());
	FCCLWorldClockSnapshot Snapshot;
	TestTrue(TEXT("rate history reconciles"), Clock.Capture(Snapshot, Error));
	if (TestTrue(TEXT("rate boundaries captured"), Snapshot.Rates.Num() >= 2))
	{
		TestEqual(TEXT("history records input change time"), Snapshot.Rates[1].GameSeconds, 1.);
		TestEqual(TEXT("history records corresponding world time"), Snapshot.Rates[1].WorldSeconds, 60.);
	}
	TestTrue(TEXT("temporary rate"), Clock.ChangeTimeScale(7, Error));
	TestTrue(TEXT("coalesce back to preceding rate"), Clock.ChangeTimeScale(0, Error));
	TestTrue(TEXT("more zero-scale input"), Clock.QueueGameTime(1, false, Error));
	TestTrue(TEXT("coalesced rate remains a valid snapshot"), Clock.Capture(Snapshot, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWorldClockRestoreTest, "CCL.Environment.Clock.RestoreAndCorruption",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLWorldClockRestoreTest::RunTest(const FString& Parameters)
{
	FCCLWorldClock Clock;
	FString Error;
	TestTrue(TEXT("legacy Agent time can seed world time"), Clock.Reset(86400, 60, Error));
	TestTrue(TEXT("accept old rate time"), Clock.QueueGameTime(1, false, Error));
	TestTrue(TEXT("change while behind"), Clock.ChangeTimeScale(5, Error));
	TestTrue(TEXT("accept new rate time"), Clock.QueueGameTime(2, false, Error));
	FCCLWorldStep Step;
	TestTrue(TEXT("partial step"), Clock.Prepare(0.5, 100, Step, Error));
	TestTrue(TEXT("commit partial step"), Clock.Commit(Step.Ticket, Error));
	FCCLWorldClockSnapshot Saved;
	if (!TestTrue(TEXT("capture completed and pending time"), Clock.Capture(Saved, Error)))
	{
		AddError(Error);
		return false;
	}

	FCCLWorldClock Restored;
	TestTrue(TEXT("restore committed snapshot"), Restored.Restore(Saved, Error));
	while (Clock.HasPendingTime())
	{
		FCCLWorldStep A;
		FCCLWorldStep B;
		if (!TestTrue(TEXT("original prepares"), Clock.Prepare(10, 100, A, Error)) ||
			!TestTrue(TEXT("restored prepares"), Restored.Prepare(10, 100, B, Error)))
		{
			return false;
		}

		TestEqual(TEXT("resume preserves step sequence"), A.StepId, B.StepId);
		TestEqual(TEXT("resume preserves target time"), A.WorldToSeconds, B.WorldToSeconds);
		if (!TestTrue(TEXT("original commits"), Clock.Commit(A.Ticket, Error)) ||
			!TestTrue(TEXT("restored commits"), Restored.Commit(B.Ticket, Error)))
		{
			return false;
		}
	}

	TestEqual(TEXT("pending time used original rates"), Restored.GetWorldSeconds(), 86470.);
	auto Corrupt = Saved;
	Corrupt.Pending[0].Scale = 61;
	TestFalse(TEXT("pending input cannot contradict rate history"), Restored.Restore(Corrupt, Error));
	TestEqual(TEXT("invalid restore retains live state"), Restored.GetWorldSeconds(), 86470.);
	Corrupt = Saved;
	Corrupt.Version = 2;
	TestFalse(TEXT("future schema rejected"), Restored.Restore(Corrupt, Error));
	Corrupt = Saved;
	Corrupt.Rates[1].WorldSeconds += 1;
	TestFalse(TEXT("discontinuous rate history rejected"), Restored.Restore(Corrupt, Error));
	TestTrue(TEXT("prepare pre-restore ticket"), Restored.QueueGameTime(1, false, Error) && Restored.Prepare(1, 100, Step, Error));
	TestTrue(TEXT("restore invalidates outstanding ticket"), Restored.Restore(Saved, Error));
	TestFalse(TEXT("pre-restore result rejected"), Restored.Commit(Step.Ticket, Error));
	TestTrue(TEXT("prepare pre-reset ticket"), Restored.Prepare(1, 100, Step, Error));
	TestTrue(TEXT("reset to independent experiment"), Restored.Reset(0, 60, Error));
	TestFalse(TEXT("pre-reset result rejected"), Restored.Commit(Step.Ticket, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWorldClockBoundsTest, "CCL.Environment.Clock.BoundsAndPrecision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLWorldClockBoundsTest::RunTest(const FString& Parameters)
{
	FCCLWorldClock Clock;
	FString Error;
	TestFalse(TEXT("negative input rejected"), Clock.QueueGameTime(-1, false, Error));
	TestFalse(TEXT("NaN input rejected"), Clock.QueueGameTime(std::numeric_limits<double>::quiet_NaN(), false, Error));
	TestFalse(TEXT("infinite scale rejected"), Clock.ChangeTimeScale(std::numeric_limits<double>::infinity(), Error));
	TestTrue(TEXT("rate for bounded backlog"), Clock.ChangeTimeScale(1, Error));
	for (int32 Index = 0; Index < 256; ++Index)
	{
		TestTrue(TEXT("bounded rate change"), Clock.ChangeTimeScale(1 + Index % 2, Error));
		TestTrue(TEXT("bounded time input"), Clock.QueueGameTime(1, false, Error));
	}

	TestTrue(TEXT("change after filling queue"), Clock.ChangeTimeScale(3, Error));
	TestFalse(TEXT("queue overflow rejects whole input"), Clock.QueueGameTime(1, false, Error));
	FCCLWorldClockSnapshot Snapshot;
	TestTrue(TEXT("queue rejection preserves valid snapshot"), Clock.Capture(Snapshot, Error));
	TestEqual(TEXT("rejected input not accepted"), Snapshot.AcceptedGameSeconds, 256.);
	FCCLWorldStep Step;
	TestFalse(TEXT("zero budget does not consume pending input"), Clock.Prepare(0, 1, Step, Error));
	TestTrue(TEXT("prepare at huge scale remains bounded"), Clock.Reset(0, 1.e6, Error) && Clock.QueueGameTime(1, false, Error) && Clock.Prepare(1, 1, Step, Error));
	TestEqual(TEXT("world interval respects budget"), Step.WorldToSeconds, 1.);
	TestTrue(TEXT("large scale commit"), Clock.Commit(Step.Ticket, Error));
	TestTrue(TEXT("unprocessed large-scale time retained"), Clock.HasPendingTime());
	TestTrue(TEXT("large-scale snapshot reconciles"), Clock.Capture(Snapshot, Error));
	TestTrue(TEXT("reset fractional-rate clock"), Clock.Reset(0, 60, Error));
	for (int32 Index = 0; Index < 6000; ++Index)
	{
		if (!Clock.QueueGameTime(1. / 60., false, Error) || !Clock.Prepare(1, 60, Step, Error) || !Clock.Commit(Step.Ticket, Error))
		{
			AddError(Error);
			return false;
		}
	}

	TestFalse(TEXT("fractional frames do not leave phantom backlog"), Clock.HasPendingTime());
	TestTrue(TEXT("fractional-rate snapshot reconciles"), Clock.Capture(Snapshot, Error));
	TestTrue(TEXT("reset sub-nanosecond clock"), Clock.Reset(0, 60, Error));
	TestTrue(TEXT("tiny old-rate input"), Clock.QueueGameTime(1.e-10, false, Error));
	TestTrue(TEXT("tiny rate boundary"), Clock.ChangeTimeScale(10, Error));
	TestTrue(TEXT("tiny new-rate input"), Clock.QueueGameTime(1.e-10, false, Error));
	TestTrue(TEXT("nearby boundaries are not collapsed by an absolute tolerance"), Clock.Capture(Snapshot, Error));
	TestTrue(TEXT("reset fractional backlog"), Clock.Reset(0, 60, Error));
	for (int32 Index = 0; Index < 3; ++Index)
	{
		TestTrue(TEXT("merge fractional input"), Clock.QueueGameTime(0.1, false, Error));
	}

	TestTrue(TEXT("prepare full fractional backlog"), Clock.Prepare(1, 100, Step, Error));
	TestTrue(TEXT("commit full fractional backlog"), Clock.Commit(Step.Ticket, Error));
	TestTrue(TEXT("input endpoint survives different accumulation order"), Clock.Capture(Snapshot, Error));
	return true;
}
#endif
