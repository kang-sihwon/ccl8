#include "CCLEcologySimulation.h"

#include "Misc/AutomationTest.h"
#include "Misc/Crc.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	FGuid EcologyId(uint32 N)
	{
		return FGuid(880, 881, 882, N);
	}

	bool SetupEcology(FCCLEcologySimulation& S, FString& Error, int64 Capacity = 100)
	{
		if (!S.Initialize(EcologyId(1), 100., Error))
		{
			return false;
		}

		for (uint32 I = 0; I < 4; ++I)
		{
			FCCLEcologySpecies Species;
			Species.Id = EcologyId(10 + I);
			Species.Movement = ECCLEcologyMovement(I);
			Species.MaturitySeconds = 10.;
			Species.LifespanSeconds = 1000.;
			FCCLEcologyHabitat Habitat;
			Habitat.Id = EcologyId(20 + I);
			Habitat.MovementMask = 1 << I;
			Habitat.Capacity = Capacity;
			FCCLEcologyPopulation Population;
			Population.Id = EcologyId(30 + I);
			Population.SpeciesId = Species.Id;
			Population.HabitatId = Habitat.Id;
			Population.Cohorts.Add(EcologyId(40 + I), {EcologyId(40 + I), 0., 10});
			if (!S.AddSpecies(Species, Error) || !S.AddHabitat(Habitat, Error) || !S.AddPopulation(Population, Error))
			{
				return false;
			}
		}

		FCCLEcologyPopulation Destination;
		Destination.Id = EcologyId(34);
		Destination.SpeciesId = EcologyId(10);
		Destination.HabitatId = EcologyId(20);
		return S.AddPopulation(Destination, Error);
	}

	TArray<uint8> EcologyBytes(const FCCLEcologySimulation& S)
	{
		TArray<uint8> Bytes;
		FString Error;
		S.Capture(Bytes, Error);
		return Bytes;
	}

	FCCLEcologyEvent Transfer(uint32 Id, ECCLEcologyEventKind Kind, int64 Count = 1)
	{
		FCCLEcologyEvent E;
		E.Id = EcologyId(Id);
		E.Kind = Kind;
		E.Source = EcologyId(30);
		E.CohortId = EcologyId(40);
		E.Count = Count;
		return E;
	}

	bool SetupLifeCycle(FCCLEcologySimulation& S, FString& Error, double Interval = 20.)
	{
		FCCLEcologySpecies Species;
		Species.Id = EcologyId(10);
		Species.MaturitySeconds = 10.;
		Species.LifespanSeconds = 60.;
		Species.BirthIntervalSeconds = Interval;
		FCCLEcologyHabitat Habitat;
		Habitat.Id = EcologyId(20);
		Habitat.Capacity = 100;
		FCCLEcologyPopulation Population;
		Population.Id = EcologyId(30);
		Population.SpeciesId = Species.Id;
		Population.HabitatId = Habitat.Id;
		Population.Cohorts.Add(EcologyId(40), {EcologyId(40), 0., 10});
		return S.Initialize(EcologyId(1), 0., Error) && S.AddSpecies(Species, Error)
			&& S.AddHabitat(Habitat, Error) && S.AddPopulation(Population, Error);
	}

	void RepairEcologyChecksum(TArray<uint8>& Bytes)
	{
		const uint32 Crc = FCrc::MemCrc32(Bytes.GetData(), Bytes.Num() - 4);
		FMemory::Memcpy(Bytes.GetData() + Bytes.Num() - 4, &Crc, 4);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLEcologyReservationTest, "CCL.Environment.Ecology.FourMediaReservationAndLease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLEcologyReservationTest::RunTest(const FString& Parameters)
{
	FCCLEcologySimulation S;
	FString Error;
	if (!TestTrue(TEXT("four species setup"), SetupEcology(S, Error)))
	{
		return false;
	}

	for (uint32 I = 0; I < 4; ++I)
	{
		const FGuid P = EcologyId(30 + I), C = EcologyId(40 + I), Id = EcologyId(50 + I), Request = EcologyId(60 + I);
		TestTrue(TEXT("reserve from aggregate"), S.Materialize(Request, P, C, Id, false, Error));
		TestEqual(TEXT("aggregate deducted"), S.AggregateCount(P), int64(9));
		TestEqual(TEXT("living conserved"), S.LivingCount(P), int64(10));
		FCCLEcologyLease Actor, Mass;
		TestTrue(TEXT("actor acquires"), S.Acquire(Id, ECCLEcologyRepresentation::Actor, EcologyId(90), Actor, Error));
		TestFalse(TEXT("Mass cannot also own it"), S.Acquire(Id, ECCLEcologyRepresentation::Mass, EcologyId(91), Mass, Error));
		TestEqual(TEXT("failed acquire leaves output untouched"), Mass.Sequence, uint64(0));
		TestTrue(TEXT("release to aggregate"), S.Release(Actor, Error));
		TestEqual(TEXT("aggregate restored"), S.AggregateCount(P), int64(10));
		TestNull(TEXT("ordinary identity record retired"), S.FindIndividual(Id));
		TestFalse(TEXT("stale release rejected"), S.Release(Actor, Error));
		TestFalse(TEXT("new request cannot reuse retired identity"), S.Materialize(EcologyId(70 + I), P, C, Id, false, Error));
		TestTrue(TEXT("original request replay accepted"), S.Materialize(Request, P, C, Id, false, Error));
		TestNull(TEXT("replay does not resurrect"), S.FindIndividual(Id));
		TestEqual(TEXT("replay conserves count"), S.LivingCount(P), int64(10));
	}

	TestTrue(TEXT("valid ledgers"), S.Validate(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLEcologyPersistentTest, "CCL.Environment.Ecology.PersistentIdentityAndRestoreEpoch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLEcologyPersistentTest::RunTest(const FString& Parameters)
{
	FCCLEcologySimulation S;
	FString Error;
	if (!TestTrue(TEXT("setup"), SetupEcology(S, Error)))
	{
		return false;
	}

	const FGuid Id = EcologyId(50);
	TestTrue(TEXT("persistent reservation"), S.Materialize(EcologyId(60), EcologyId(30), EcologyId(40), Id, true, Error));
	FCCLEcologyLease Actor, Mass, Restored;
	TestTrue(TEXT("actor lease"), S.Acquire(Id, ECCLEcologyRepresentation::Actor, EcologyId(90), Actor, Error));
	TestTrue(TEXT("actor return"), S.Release(Actor, Error));
	TestNotNull(TEXT("important identity retained"), S.FindIndividual(Id));
	TestEqual(TEXT("important individual remains reserved"), S.AggregateCount(EcologyId(30)), int64(9));
	TestTrue(TEXT("Mass lease"), S.Acquire(Id, ECCLEcologyRepresentation::Mass, EcologyId(91), Mass, Error));
	TestFalse(TEXT("old Actor cannot release Mass"), S.Release(Actor, Error));
	const auto Before = EcologyBytes(S);
	TestTrue(TEXT("restore active reservation"), S.Restore(Before, Error));
	TestTrue(TEXT("persistent bytes deterministic"), EcologyBytes(S) == Before);
	TestEqual(TEXT("reservation survives restore"), S.LivingCount(EcologyId(30)), int64(10));
	TestFalse(TEXT("pre-restore lease rejected"), S.Release(Mass, Error));
	TestTrue(TEXT("fresh lease available"), S.Acquire(Id, ECCLEcologyRepresentation::Actor, EcologyId(90), Restored, Error));
	TestTrue(TEXT("restore changes epoch"), Restored.Epoch != Mass.Epoch);
	TestTrue(TEXT("release fresh lease"), S.Release(Restored, Error));
	TestTrue(TEXT("valid"), S.Validate(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLEcologyEventsTest, "CCL.Environment.Ecology.ExactlyOnceBirthDeathCaptureMigration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLEcologyEventsTest::RunTest(const FString& Parameters)
{
	FCCLEcologySimulation S;
	FString Error;
	if (!TestTrue(TEXT("setup"), SetupEcology(S, Error)))
	{
		return false;
	}

	auto Birth = Transfer(100, ECCLEcologyEventKind::Birth, 2);
	Birth.Target = Birth.Source;
	Birth.CohortId = EcologyId(101);
	Birth.BirthWorldSeconds = 100.;
	TestTrue(TEXT("adult pair birth"), S.Apply(Birth, Error));
	TestEqual(TEXT("newborn population"), S.LivingCount(EcologyId(30)), int64(12));
	const auto Born = EcologyBytes(S);
	TestTrue(TEXT("duplicate birth"), S.Apply(Birth, Error));
	TestTrue(TEXT("duplicate has no effect"), Born == EcologyBytes(S));
	auto Collision = Birth;
	Collision.Count = 3;
	TestFalse(TEXT("same event ID different payload"), S.Apply(Collision, Error));
	TestTrue(TEXT("collision atomic"), Born == EcologyBytes(S));
	TestTrue(TEXT("death"), S.Apply(Transfer(102, ECCLEcologyEventKind::Death, 2), Error));
	TestTrue(TEXT("capture"), S.Apply(Transfer(103, ECCLEcologyEventKind::Capture, 1), Error));
	auto Move = Transfer(104, ECCLEcologyEventKind::Migration, 3);
	Move.Target = EcologyId(34);
	TestTrue(TEXT("migration"), S.Apply(Move, Error));
	TestEqual(TEXT("source after transfer"), S.LivingCount(EcologyId(30)), int64(6));
	TestEqual(TEXT("destination after transfer"), S.LivingCount(EcologyId(34)), int64(3));
	TestEqual(TEXT("departed ledger"), S.GetPopulations()[EcologyId(30)].Departed, int64(3));
	TestEqual(TEXT("arrived ledger"), S.GetPopulations()[EcologyId(34)].Arrived, int64(3));
	auto Arrival = Transfer(105, ECCLEcologyEventKind::Immigration, 4);
	Arrival.Source.Invalidate();
	Arrival.Target = EcologyId(30);
	TestTrue(TEXT("recorded outside inflow"), S.Apply(Arrival, Error));
	const auto Saved = EcologyBytes(S);
	TestTrue(TEXT("restore receipts"), S.Restore(Saved, Error));
	TestTrue(TEXT("migration replay after restore"), S.Apply(Move, Error));
	TestTrue(TEXT("no repeated inflow after restore"), S.Apply(Arrival, Error));
	TestTrue(TEXT("receipt replay leaves bytes intact"), Saved == EcologyBytes(S));
	TestTrue(TEXT("valid"), S.Validate(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLEcologyFailureTest, "CCL.Environment.Ecology.InvalidTransfersAreAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLEcologyFailureTest::RunTest(const FString& Parameters)
{
	FCCLEcologySimulation S;
	FString Error;
	if (!TestTrue(TEXT("setup at habitat capacity"), SetupEcology(S, Error, 10)))
	{
		return false;
	}

	const auto Before = EcologyBytes(S);
	auto Arrival = Transfer(100, ECCLEcologyEventKind::Immigration);
	Arrival.Source.Invalidate();
	Arrival.Target = EcologyId(30);
	TestFalse(TEXT("capacity cannot be exceeded"), S.Apply(Arrival, Error));
	TestFalse(TEXT("cannot remove excess population"), S.Apply(Transfer(101, ECCLEcologyEventKind::Death, 11), Error));
	auto Move = Transfer(102, ECCLEcologyEventKind::Migration);
	Move.Target = EcologyId(31);
	TestFalse(TEXT("species cannot change by migration"), S.Apply(Move, Error));
	FCCLEcologyPopulation WrongMedium;
	WrongMedium.Id = EcologyId(35);
	WrongMedium.SpeciesId = EcologyId(10);
	WrongMedium.HabitatId = EcologyId(21);
	TestFalse(TEXT("ground species cannot occupy water habitat"), S.AddPopulation(WrongMedium, Error));
	auto Birth = Transfer(103, ECCLEcologyEventKind::Birth, 6);
	Birth.Target = Birth.Source;
	Birth.CohortId = EcologyId(104);
	Birth.BirthWorldSeconds = 100.;
	TestFalse(TEXT("birth requires sufficient adult pairs"), S.Apply(Birth, Error));
	TestTrue(TEXT("all failed operations leave persistent state untouched"), Before == EcologyBytes(S));
	TestEqual(TEXT("rejected requests have no receipt"), S.ReceiptCount(), 0);
	TestTrue(TEXT("reserve while habitat full"), S.Materialize(EcologyId(105), EcologyId(30), EcologyId(40), EcologyId(50), true, Error));
	TestFalse(TEXT("reservation does not free habitat capacity"), S.Apply(Arrival, Error));
	TestTrue(TEXT("valid"), S.Validate(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLEcologyIndividualTransferTest, "CCL.Environment.Ecology.IdentifiedMigrationAndTerminalState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLEcologyIndividualTransferTest::RunTest(const FString& Parameters)
{
	FCCLEcologySimulation S;
	FString Error;
	if (!TestTrue(TEXT("setup"), SetupEcology(S, Error)))
	{
		return false;
	}

	const FGuid Id = EcologyId(50);
	TestTrue(TEXT("reserve identity"), S.Materialize(EcologyId(60), EcologyId(30), EcologyId(40), Id, true, Error));
	FCCLEcologyLease Lease;
	TestTrue(TEXT("execution lease"), S.Acquire(Id, ECCLEcologyRepresentation::Actor, EcologyId(90), Lease, Error));
	auto Move = Transfer(100, ECCLEcologyEventKind::Migration);
	Move.IndividualId = Id;
	Move.Target = EcologyId(34);
	const auto Before = EcologyBytes(S);
	TestFalse(TEXT("active writer must quiesce before transfer"), S.Apply(Move, Error));
	TestTrue(TEXT("failed transfer atomic"), Before == EcologyBytes(S));
	TestTrue(TEXT("quiesce"), S.Release(Lease, Error));
	TestTrue(TEXT("identified migration"), S.Apply(Move, Error));
	const auto* Individual = S.FindIndividual(Id);
	if (!TestNotNull(TEXT("identity retained"), Individual))
	{
		return false;
	}

	TestTrue(TEXT("destination retained in record"), Individual->PopulationId == EcologyId(34));
	TestEqual(TEXT("age retained"), Individual->BirthWorldSeconds, 0.);
	TestEqual(TEXT("source count"), S.LivingCount(EcologyId(30)), int64(9));
	TestEqual(TEXT("destination reserved count"), S.LivingCount(EcologyId(34)), int64(1));
	TestEqual(TEXT("destination has no duplicate aggregate"), S.AggregateCount(EcologyId(34)), int64(0));
	auto Capture = Transfer(101, ECCLEcologyEventKind::Capture);
	Capture.Source = EcologyId(34);
	Capture.IndividualId = Id;
	TestTrue(TEXT("capture identified individual"), S.Apply(Capture, Error));
	TestEqual(TEXT("capture removed from live count"), S.LivingCount(EcologyId(34)), int64(0));
	TestTrue(TEXT("captured identity retained"), S.FindIndividual(Id)->State == ECCLEcologyLifeState::Captured);
	TestFalse(TEXT("captured individual cannot execute"), S.Acquire(Id, ECCLEcologyRepresentation::Mass, EcologyId(90), Lease, Error));
	TestTrue(TEXT("capture duplicate"), S.Apply(Capture, Error));
	auto Death = Capture;
	Death.Id = EcologyId(102);
	Death.Kind = ECCLEcologyEventKind::Death;
	TestFalse(TEXT("cannot debit terminal individual again"), S.Apply(Death, Error));
	TestTrue(TEXT("valid"), S.Validate(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLEcologySnapshotTest, "CCL.Environment.Ecology.CorruptSnapshotAndAllocationBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLEcologySnapshotTest::RunTest(const FString& Parameters)
{
	FCCLEcologySimulation S;
	FString Error;
	if (!TestTrue(TEXT("setup"), SetupEcology(S, Error)))
	{
		return false;
	}

	const auto Before = EcologyBytes(S);
	TestTrue(TEXT("snapshot nonempty"), Before.Num() > 60);
	auto Bad = Before;
	Bad[40] ^= 1;
	TestFalse(TEXT("checksum corruption"), S.Restore(Bad, Error));
	Bad = Before;
	Bad.SetNum(Bad.Num() - 10);
	TestFalse(TEXT("truncation"), S.Restore(Bad, Error));
	Bad = Before;
	const uint32 Unsupported = 100;
	FMemory::Memcpy(Bad.GetData() + 4, &Unsupported, 4);
	RepairEcologyChecksum(Bad);
	TestFalse(TEXT("unsupported version despite valid checksum"), S.Restore(Bad, Error));
	Bad = Before;
	const int32 HugeCount = MAX_int32;
	FMemory::Memcpy(Bad.GetData() + 32, &HugeCount, 4);
	RepairEcologyChecksum(Bad);
	TestFalse(TEXT("unbounded allocation rejected"), S.Restore(Bad, Error));
	Bad = Before;
	Bad.Insert(uint8(0), Bad.Num() - 4);
	RepairEcologyChecksum(Bad);
	TestFalse(TEXT("trailing payload rejected"), S.Restore(Bad, Error));
	TestTrue(TEXT("all bad restores preserve original"), Before == EcologyBytes(S));
	FCCLEcologySimulation Restored;
	TestTrue(TEXT("valid snapshot restore"), Restored.Restore(Before, Error));
	TestTrue(TEXT("stable serialization order"), Before == EcologyBytes(Restored));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLEcologyAgeTest, "CCL.Environment.Ecology.ChronologicalBirthDeathAndPartition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLEcologyAgeTest::RunTest(const FString& Parameters)
{
	FCCLEcologySimulation Whole, Split, Saved;
	FString Error;
	if (!TestTrue(TEXT("lifecycle setup"), SetupLifeCycle(Whole, Error)))
	{
		return false;
	}

	Split = Whole;
	TestTrue(TEXT("first breeding window"), Split.AdvanceTo(20., Error));
	TestEqual(TEXT("five births from ten mature individuals"), Split.LivingCount(EcologyId(30)), int64(15));
	TestTrue(TEXT("second breeding window"), Split.AdvanceTo(40., Error));
	TestEqual(TEXT("seven further births"), Split.LivingCount(EcologyId(30)), int64(22));
	TestTrue(TEXT("death precedes coincident birth"), Split.AdvanceTo(60., Error));
	TestEqual(TEXT("ten die before six births"), Split.LivingCount(EcologyId(30)), int64(18));
	TestEqual(TEXT("natural mortality ledger"), Split.GetPopulations()[EcologyId(30)].Died, int64(10));
	TestTrue(TEXT("arbitrary split"), Split.AdvanceTo(73.25, Error));
	TestTrue(TEXT("restore between events"), Saved.Restore(EcologyBytes(Split), Error));
	for (double Time : {80., 101.5, 137., 200.})
	{
		TestTrue(TEXT("continue restored lifecycle"), Saved.AdvanceTo(Time, Error));
	}

	TestTrue(TEXT("single full interval"), Whole.AdvanceTo(200., Error));
	TestTrue(TEXT("same cohorts, event IDs and age for all partitions"), EcologyBytes(Whole) == EcologyBytes(Saved));
	const auto Before = EcologyBytes(Whole);
	TestTrue(TEXT("same-time advance idempotent"), Whole.AdvanceTo(200., Error));
	TestFalse(TEXT("rewind rejected"), Whole.AdvanceTo(199., Error));
	TestTrue(TEXT("same-time and rejected rewind preserve state"), Before == EcologyBytes(Whole));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLEcologyMortalityTest, "CCL.Environment.Ecology.MortalityRevokesExecutionAndNoRefill",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLEcologyMortalityTest::RunTest(const FString& Parameters)
{
	FCCLEcologySimulation S;
	FString Error;
	if (!TestTrue(TEXT("nonbreeding species setup"), SetupLifeCycle(S, Error, 0.)))
	{
		return false;
	}

	const FGuid Id = EcologyId(50);
	TestTrue(TEXT("reserve active individual"), S.Materialize(EcologyId(60), EcologyId(30), EcologyId(40), Id, true, Error));
	FCCLEcologyLease Lease;
	TestTrue(TEXT("active Actor writer"), S.Acquire(Id, ECCLEcologyRepresentation::Actor, EcologyId(90), Lease, Error));
	TestTrue(TEXT("advance past lifespan"), S.AdvanceTo(65., Error));
	TestEqual(TEXT("aggregate and identified mortality both counted"), S.GetPopulations()[EcologyId(30)].Died, int64(10));
	TestEqual(TEXT("extinct population"), S.LivingCount(EcologyId(30)), int64(0));
	TestTrue(TEXT("terminal identity retained"), S.FindIndividual(Id)->State == ECCLEcologyLifeState::Dead);
	TestFalse(TEXT("expired writer cannot return dead individual"), S.Release(Lease, Error));
	TestTrue(TEXT("restore mortality"), S.Restore(EcologyBytes(S), Error));
	TestTrue(TEXT("long absence"), S.AdvanceTo(10000., Error));
	TestEqual(TEXT("absence cannot refill population"), S.LivingCount(EcologyId(30)), int64(0));
	auto Arrival = Transfer(100, ECCLEcologyEventKind::Immigration, 2);
	Arrival.Source.Invalidate();
	Arrival.Target = EcologyId(30);
	TestFalse(TEXT("expired cohort cannot re-enter"), S.Apply(Arrival, Error));
	FCCLEcologyPopulation Expired;
	Expired.Id = EcologyId(35);
	Expired.SpeciesId = EcologyId(10);
	Expired.HabitatId = EcologyId(20);
	Expired.Cohorts.Add(EcologyId(40), {EcologyId(40), 0., 2});
	TestFalse(TEXT("initial allocation cannot reintroduce expired cohort"), S.AddPopulation(Expired, Error));
	Arrival.CohortId = EcologyId(101);
	Arrival.BirthWorldSeconds = 9990.;
	TestTrue(TEXT("recorded viable immigration restores population"), S.Apply(Arrival, Error));
	TestEqual(TEXT("only recorded inflow restored"), S.LivingCount(EcologyId(30)), int64(2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLEcologyAdvanceFailureTest, "CCL.Environment.Ecology.TimeBudgetFailurePreservesReservations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLEcologyAdvanceFailureTest::RunTest(const FString& Parameters)
{
	FCCLEcologySimulation S;
	FString Error;
	if (!TestTrue(TEXT("setup"), SetupLifeCycle(S, Error)))
	{
		return false;
	}

	const FGuid Id = EcologyId(50);
	TestTrue(TEXT("reserve"), S.Materialize(EcologyId(60), EcologyId(30), EcologyId(40), Id, true, Error));
	FCCLEcologyLease Lease;
	TestTrue(TEXT("lease"), S.Acquire(Id, ECCLEcologyRepresentation::Mass, EcologyId(90), Lease, Error));
	const auto Before = EcologyBytes(S);
	TestFalse(TEXT("bounded work rejects oversized interval"), S.AdvanceTo(200., Error, 1));
	TestTrue(TEXT("failure discards births, deaths, time and receipts"), Before == EcologyBytes(S));
	TestTrue(TEXT("original execution lease remains valid"), S.Release(Lease, Error));
	TestTrue(TEXT("smaller retry interval succeeds"), S.AdvanceTo(20., Error, 1));
	TestEqual(TEXT("retry executes birth only once"), S.LivingCount(EcologyId(30)), int64(15));
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLEcologyHandoffTest, "CCL.Environment.Ecology.LeaseHandoffPositionAndTerminalResult",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLEcologyHandoffTest::RunTest(const FString& Parameters)
{
	FCCLEcologySimulation S;
	FString Error;
	if (!TestTrue(TEXT("setup"), SetupEcology(S, Error)))
	{
		return false;
	}

	const FGuid Id = EcologyId(50);
	TestTrue(TEXT("reserve"), S.Materialize(EcologyId(60), EcologyId(30), EcologyId(40), Id, true, Error));
	FCCLEcologyLease Actor, Mass;
	TestTrue(TEXT("actor owns reservation"), S.Acquire(Id, ECCLEcologyRepresentation::Actor, EcologyId(90), Actor, Error));
	TestTrue(TEXT("actor moves within habitat"), S.UpdatePosition(Actor, FVector(20., 30., 40.), Error));
	const auto Before = EcologyBytes(S);
	TestFalse(TEXT("invalid destination rejected"), S.UpdatePosition(Actor, FVector(101., 30., 40.), Error));
	TestTrue(TEXT("failed position write unchanged"), Before == EcologyBytes(S));
	TestTrue(TEXT("handoff without returning population"), S.Transfer(Actor, ECCLEcologyRepresentation::Mass, EcologyId(91), Mass, Error));
	TestTrue(TEXT("handoff does not alter persistent state"), Before == EcologyBytes(S));
	TestFalse(TEXT("old Actor may not update"), S.UpdatePosition(Actor, FVector(10.), Error));
	TestFalse(TEXT("old Actor may not return population"), S.Release(Actor, Error));
	TestFalse(TEXT("cannot return reservation while Mass owns it"), S.ReturnToAggregate(Id, Error));
	TestTrue(TEXT("Mass is current"), S.IsLeaseCurrent(Mass));
	auto Death = Transfer(100, ECCLEcologyEventKind::Death);
	Death.IndividualId = Id;
	Death.Count = 2;
	TestFalse(TEXT("bad outcome preserves current writer"), S.Finish(Mass, Death, Error));
	TestTrue(TEXT("Mass writer retained"), S.IsLeaseCurrent(Mass));
	Death.Count = 1;
	TestTrue(TEXT("current writer records death"), S.Finish(Mass, Death, Error));
	TestEqual(TEXT("death counted once"), S.LivingCount(EcologyId(30)), int64(9));
	TestTrue(TEXT("terminal result replay succeeds"), S.Finish(Mass, Death, Error));
	TestEqual(TEXT("replay does not debit again"), S.LivingCount(EcologyId(30)), int64(9));
	TestFalse(TEXT("dead writer cannot return count"), S.Release(Mass, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLEcologySuspendTest, "CCL.Environment.Ecology.SuspendedReservationAndSavedPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLEcologySuspendTest::RunTest(const FString& Parameters)
{
	FCCLEcologySimulation S;
	FString Error;
	if (!TestTrue(TEXT("setup"), SetupEcology(S, Error)))
	{
		return false;
	}

	const FGuid Id = EcologyId(50);
	TestTrue(TEXT("ordinary reservation"), S.Materialize(EcologyId(60), EcologyId(30), EcologyId(40), Id, false, Error));
	FCCLEcologyLease Lease;
	TestTrue(TEXT("Mass lease"), S.Acquire(Id, ECCLEcologyRepresentation::Mass, EcologyId(90), Lease, Error));
	TestTrue(TEXT("position written"), S.UpdatePosition(Lease, FVector(25., 35., 45.), Error));
	TestTrue(TEXT("suspend without returning aggregate"), S.Suspend(Lease, Error));
	TestEqual(TEXT("reservation retained"), S.AggregateCount(EcologyId(30)), int64(9));
	const auto Bytes = EcologyBytes(S);
	TestTrue(TEXT("restore saved position"), S.Restore(Bytes, Error));
	TestTrue(TEXT("position preserved exactly"), S.FindIndividual(Id)->PositionMeters == FVector(25., 35., 45.));
	TestTrue(TEXT("unowned restored reservation may return"), S.ReturnToAggregate(Id, Error));
	TestEqual(TEXT("aggregate complete"), S.AggregateCount(EcologyId(30)), int64(10));
	TestFalse(TEXT("duplicate return rejected"), S.ReturnToAggregate(Id, Error));
	return true;
}
#endif
