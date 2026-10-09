#include "CCLWorldAdvance.h"

#include "CCLWorldClock.h"
#include "Agents/CCLLifeSimulation.h"
#include "Agents/CCLAgentTags.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWorldAdvanceRollbackTest, "CCL.Environment.Advance.RollbackAndRetry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLWorldAdvanceRollbackTest::RunTest(const FString& Parameters)
{
	FCCLLifeSimulation Life;
	FString Error;
	const auto Initial = FCCLLifeSimulation::MerchantScenario(42);
	if (!TestTrue(TEXT("initialize"), Life.Initialize(Initial, Error)))
	{
		return false;
	}

	TArray<uint8> Before;
	TestTrue(TEXT("capture before work"), Life.Save(Before));
	const auto Handle = Life.GetAgents().GetHandle(Initial.Agents[0].Id);
	TestFalse(TEXT("second hour exceeds budget after first hour changes candidate economy"),
		Life.TryAdvanceTo(7200, Error, 1));
	TestTrue(TEXT("failure is explained"), !Error.IsEmpty());
	TArray<uint8> After;
	TestTrue(TEXT("capture rejected work"), Life.Save(After));
	TestTrue(TEXT("every serialized field unchanged after partial candidate failure"), Before == After);
	TestEqual(TEXT("failed step retains time"), Life.GetTime(), 0.);
	TestNotNull(TEXT("stable handle retained"), Life.GetAgents().Find(Handle));

	TestTrue(TEXT("retry fits budget"), Life.TryAdvanceTo(7200, Error, 2));
	FCCLLifeSimulation Reference;
	TestTrue(TEXT("initialize reference"), Reference.Initialize(Initial, Error));
	TestTrue(TEXT("uninterrupted reference"), Reference.TryAdvanceTo(7200, Error));
	TestEqual(TEXT("retry neither loses nor duplicates economy and decisions"), Life.DailyReport(), Reference.DailyReport());
	TestNotNull(TEXT("successful publication preserves handles"), Life.GetAgents().Find(Handle));
	TestTrue(TEXT("capture committed work"), Life.Save(Before));
	TestTrue(TEXT("same target is idempotent"), Life.TryAdvanceTo(7200, Error));
	TestTrue(TEXT("capture repeated target"), Life.Save(After));
	TestTrue(TEXT("same target has no repeated events"), Before == After);
	TestFalse(TEXT("reverse time rejected"), Life.TryAdvanceTo(7199, Error));
	TestEqual(TEXT("reverse failure preserves completed time"), Life.GetTime(), 7200.);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWorldAdvanceClockTest, "CCL.Environment.Advance.ClockAndLifeCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLWorldAdvanceClockTest::RunTest(const FString& Parameters)
{
	FCCLLifeSimulation Life;
	FCCLWorldClock Clock;
	FString Error;
	TestTrue(TEXT("initialize life"), Life.Initialize(FCCLLifeSimulation::MerchantScenario(42), Error));
	TestTrue(TEXT("queue input at sixty times"), Clock.QueueGameTime(120, false, Error));
	TestFalse(TEXT("life failure rejects whole world step"), CCLWorldAdvance::Advance(Clock, Life, 120, 7200, Error, 1));
	TestEqual(TEXT("clock does not outrun life"), Clock.GetWorldSeconds(), Life.GetTime());
	TestEqual(TEXT("game input not falsely completed"), Clock.GetGameSeconds(), 0.);
	TestTrue(TEXT("unprocessed input retained"), Clock.HasPendingTime());
	TestFalse(TEXT("no stranded ticket"), Clock.HasPreparedStep());
	TestTrue(TEXT("change rate while failed input waits"), Clock.ChangeTimeScale(10, Error));
	TestTrue(TEXT("queue later input"), Clock.QueueGameTime(1, false, Error));
	TestTrue(TEXT("retry old segment"), CCLWorldAdvance::Advance(Clock, Life, 120, 7200, Error, 2));
	TestEqual(TEXT("old segment uses accepted rate"), Life.GetTime(), 7200.);
	TestTrue(TEXT("advance new segment"), CCLWorldAdvance::Advance(Clock, Life, 1, 60, Error));
	TestEqual(TEXT("new rate applied once"), Life.GetTime(), 7210.);
	TestEqual(TEXT("same committed time"), Clock.GetWorldSeconds(), Life.GetTime());
	TestEqual(TEXT("all accepted game input committed"), Clock.GetGameSeconds(), 121.);
	TestFalse(TEXT("nothing pending"), Clock.HasPendingTime());
	TestFalse(TEXT("empty advance rejected"), CCLWorldAdvance::Advance(Clock, Life, 1, 60, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWorldAdvanceWriterTest, "CCL.Environment.Advance.WriterOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLWorldAdvanceWriterTest::RunTest(const FString& Parameters)
{
	FCCLLifeSimulation Life;
	FCCLWorldClock Clock;
	FString Error;
	const auto Initial = FCCLLifeSimulation::MerchantScenario(42);
	TestTrue(TEXT("initialize"), Life.Initialize(Initial, Error));
	const FGuid Id = Initial.Agents[0].Id;
	const auto Handle = Life.GetAgents().GetHandle(Id);
	const auto Lease = Life.GetAgents().Acquire(Handle, FGuid::NewGuid());
	TestTrue(TEXT("hold writer"), Lease.Writer.IsValid());
	TestTrue(TEXT("queue while actor owns state"), Clock.QueueGameTime(60, false, Error));
	TestFalse(TEXT("writer blocks simulation publication"), CCLWorldAdvance::Advance(Clock, Life, 60, 3600, Error));
	TestEqual(TEXT("writer failure leaves both at zero"), Life.GetTime() + Clock.GetWorldSeconds(), 0.);
	TestTrue(TEXT("failed candidate does not invalidate live writer"), Life.GetAgents().Release(Lease));

	Life.SetActorActive(Id, true);
	const auto& Known = Initial.Agents[0].Features[CCLAgentTags::Feature_Experience].Data.Get<FCCLAgentExperience>().KnownOpportunities;
	const auto* Opportunity = Initial.Opportunities.FindByPredicate([&](const FCCLWorldOpportunity& Value)
	{
		return Known.Contains(Value.OpportunityId) && Value.bAvailable;
	});
	if (!TestNotNull(TEXT("known available opportunity exists"), Opportunity))
	{
		return false;
	}

	const auto Reservation = Life.ReserveOpportunity(Id, Opportunity->OpportunityId);
	TestTrue(TEXT("reservation acquired"), Reservation.IsValid());
	TestTrue(TEXT("commit small step after releasing writer"), CCLWorldAdvance::Advance(Clock, Life, 0.25, 15, Error));
	TestTrue(TEXT("candidate retains unexpired reservation"), Life.HasReservation(Id, Opportunity->OpportunityId, Reservation));
	TestTrue(TEXT("commit remaining hour"), CCLWorldAdvance::Advance(Clock, Life, 60, 3600, Error));
	const auto* ActorRecord = Life.Find(Id);
	if (TestNotNull(TEXT("actor record retained"), ActorRecord))
	{
		TestFalse(TEXT("actor-owned agent not also run by reduced executor"), ActorRecord->Intent.OpportunityId.IsValid());
		TestEqual(TEXT("actor needs still use world time"), ActorRecord->LastSimulatedTime, 3600.);
	}
	return true;
}

#endif