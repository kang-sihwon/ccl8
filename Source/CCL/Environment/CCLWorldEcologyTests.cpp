#include "CCLWorldAdvance.h"

#include "CCLEcologySimulation.h"
#include "CCLWorldSnapshot.h"
#include "CCLSurfaceSimulation.h"
#include "Agents/CCLLifeSimulation.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	FGuid WorldEcologyId(uint32 N)
	{
		return FGuid(890, 891, 892, N);
	}

	bool MakeEcology(FGuid WorldId, FCCLEcologySimulation& E, FString& Error)
	{
		FCCLEcologySpecies Species;
		Species.Id = WorldEcologyId(1);
		Species.MaturitySeconds = 100.;
		Species.LifespanSeconds = 10000.;
		Species.BirthIntervalSeconds = 3600.;
		FCCLEcologyHabitat Habitat;
		Habitat.Id = WorldEcologyId(2);
		FCCLEcologyPopulation Population;
		Population.Id = WorldEcologyId(3);
		Population.SpeciesId = Species.Id;
		Population.HabitatId = Habitat.Id;
		Population.Cohorts.Add(WorldEcologyId(4), {WorldEcologyId(4), 0., 10});
		return E.Initialize(WorldId, 0., Error) && E.AddSpecies(Species, Error)
			&& E.AddHabitat(Habitat, Error) && E.AddPopulation(Population, Error);
	}

	TArray<uint8> WorldEcologyBytes(const FCCLEcologySimulation& E)
	{
		FString Error;
		TArray<uint8> Bytes;
		E.Capture(Bytes, Error);
		return Bytes;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWorldEcologyAdvanceTest, "CCL.Environment.Ecology.WorldAdvanceAtomicity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLWorldEcologyAdvanceTest::RunTest(const FString& Parameters)
{
	FString Error;
	const FGuid WorldId = WorldEcologyId(9);
	FCCLEcologySimulation Ecology;
	FCCLWorldClock Clock;
	FCCLLifeSimulation Life;
	FCCLSurfaceSimulation Surface;
	if (!TestTrue(TEXT("initialize all owners"), MakeEcology(WorldId, Ecology, Error)
		&& Life.Initialize(FCCLLifeSimulation::MerchantScenario(42), Error)
		&& Surface.Initialize(WorldId, 0., 0., 0, Error)))
	{
		return false;
	}

	TestTrue(TEXT("queue world interval"), Clock.QueueGameTime(120., false, Error));
	const auto Before = WorldEcologyBytes(Ecology);
	TArray<uint8> BeforeLife, BeforeSurface;
	Life.Save(BeforeLife);
	Surface.Capture(BeforeSurface, Error);
	TestFalse(TEXT("ecology budget rejects whole interval"), CCLWorldAdvance::Advance(Clock, Life, 120., 7200., Error,
		256, &Surface, nullptr, nullptr, nullptr, nullptr, &Ecology, 1));
	TestTrue(TEXT("ecology unchanged"), Before == WorldEcologyBytes(Ecology));
	TestEqual(TEXT("clock input retained"), Clock.GetPendingGameSeconds(), 120.);
	TestEqual(TEXT("clock not published"), Clock.GetWorldSeconds(), 0.);
	TestFalse(TEXT("Agent failure rejects already prepared births"), CCLWorldAdvance::Advance(Clock, Life, 120., 7200., Error,
		1, &Surface, nullptr, nullptr, nullptr, nullptr, &Ecology));
	TestTrue(TEXT("prepared births discarded"), Before == WorldEcologyBytes(Ecology));
	TArray<uint8> AfterLife, AfterSurface;
	Life.Save(AfterLife);
	Surface.Capture(AfterSurface, Error);
	TestTrue(TEXT("Agent and surface unchanged after both failures"), BeforeLife == AfterLife && BeforeSurface == AfterSurface);
	TestTrue(TEXT("retry publishes all four owners"), CCLWorldAdvance::Advance(Clock, Life, 120., 7200., Error,
		2, &Surface, nullptr, nullptr, nullptr, nullptr, &Ecology));
	TestEqual(TEXT("births committed only once"), Ecology.LivingCount(WorldEcologyId(3)), int64(22));
	TestEqual(TEXT("ecology time matches clock"), Ecology.GetWorldSeconds(), Clock.GetWorldSeconds());
	TestEqual(TEXT("surface time matches clock"), Surface.GetWorldSeconds(), Clock.GetWorldSeconds());
	TestEqual(TEXT("Agent time matches clock"), Life.GetTime(), Clock.GetWorldSeconds());
	TestFalse(TEXT("no remaining input"), Clock.HasPendingTime());
	FCCLEcologySimulation WrongWorld;
	TestTrue(TEXT("wrong world fixture"), WrongWorld.Initialize(WorldEcologyId(99), Clock.GetWorldSeconds(), Error));
	TestTrue(TEXT("queue later interval"), Clock.QueueGameTime(1., false, Error));
	TestFalse(TEXT("foreign ecology rejected before publication"), CCLWorldAdvance::Advance(Clock, Life, 1., 60., Error,
		256, &Surface, nullptr, nullptr, nullptr, nullptr, &WrongWorld));
	TestEqual(TEXT("rejection keeps committed time"), Clock.GetWorldSeconds(), 7200.);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWorldEcologySaveTest, "CCL.Environment.Ecology.WorldSaveRestoreAndRejectedCandidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLWorldEcologySaveTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLWorldIdentity Identity;
	Identity.WorldId = WorldEcologyId(9);
	Identity.Generation = 77;
	FCCLEcologySimulation Ecology;
	FCCLWorldClock Clock;
	FCCLLifeSimulation Life;
	FCCLSurfaceSimulation Surface;
	if (!TestTrue(TEXT("fixture"), MakeEcology(Identity.WorldId, Ecology, Error)
		&& Life.Initialize(FCCLLifeSimulation::MerchantScenario(42), Error)
		&& Surface.Initialize(Identity.WorldId, 0., 0., 0, Error)))
	{
		return false;
	}

	TestTrue(TEXT("reserve identified member"), Ecology.Materialize(WorldEcologyId(20), WorldEcologyId(3), WorldEcologyId(4), WorldEcologyId(5), true, Error));
	FCCLEcologyLease Lease;
	TestTrue(TEXT("active execution lease"), Ecology.Acquire(WorldEcologyId(5), ECCLEcologyRepresentation::Actor, WorldEcologyId(6), Lease, Error));
	TestTrue(TEXT("queue historical rate"), Clock.QueueGameTime(120., false, Error));
	TestTrue(TEXT("advance first interval"), CCLWorldAdvance::Advance(Clock, Life, 60., 3600., Error,
		256, &Surface, nullptr, nullptr, nullptr, nullptr, &Ecology));
	FCCLWorldSnapshot Snapshot;
	TestTrue(TEXT("capture same generation"), FCCLWorldSnapshotCodec::Capture(Identity, Clock, Life, Snapshot, Error, nullptr, &Surface, &Ecology));
	TestEqual(TEXT("current world schema"), Snapshot.Schema, uint32(7));
	TArray<uint8> Saved;
	TestTrue(TEXT("encode all owners"), FCCLWorldSnapshotCodec::Encode(Snapshot, Saved, Error));
	FCCLWorldSnapshot Decoded;
	TestTrue(TEXT("decode"), FCCLWorldSnapshotCodec::Decode(Saved, Decoded, Error));
	TestTrue(TEXT("ecology bytes carried in same generation"), Decoded.Ecology == WorldEcologyBytes(Ecology) && Decoded.Identity.Generation == 77);
	TestTrue(TEXT("advance beyond save"), CCLWorldAdvance::Advance(Clock, Life, 60., 3600., Error,
		256, &Surface, nullptr, nullptr, nullptr, nullptr, &Ecology));
	const auto Before = WorldEcologyBytes(Ecology);
	auto Bad = Decoded;
	Bad.Ecology[40] ^= 1;
	TestFalse(TEXT("bad ecology rejects full restore"), FCCLWorldSnapshotCodec::Restore(Bad, Identity.Domain, Clock, Life, Error, &Surface, &Ecology));
	TestEqual(TEXT("rejected candidate keeps clock"), Clock.GetWorldSeconds(), 7200.);
	TestEqual(TEXT("rejected candidate keeps Agent time"), Life.GetTime(), 7200.);
	TestTrue(TEXT("rejected candidate keeps ecology"), Before == WorldEcologyBytes(Ecology));
	FCCLEcologySimulation WrongTime;
	TestTrue(TEXT("wrong-time ecology"), WrongTime.Initialize(Identity.WorldId, 1., Error) && WrongTime.Capture(Bad.Ecology, Error));
	TestFalse(TEXT("valid ecology at wrong time rejected"), FCCLWorldSnapshotCodec::Restore(Bad, Identity.Domain, Clock, Life, Error, &Surface, &Ecology));
	TestTrue(TEXT("wrong-world ecology"), WrongTime.Initialize(WorldEcologyId(99), 3600., Error) && WrongTime.Capture(Bad.Ecology, Error));
	TestFalse(TEXT("valid ecology from foreign world rejected"), FCCLWorldSnapshotCodec::Restore(Bad, Identity.Domain, Clock, Life, Error, &Surface, &Ecology));
	TestTrue(TEXT("restore full generation"), FCCLWorldSnapshotCodec::Restore(Decoded, Identity.Domain, Clock, Life, Error, &Surface, &Ecology));
	TestTrue(TEXT("restored reserved state equals saved state"), WorldEcologyBytes(Ecology) == Decoded.Ecology);
	TestFalse(TEXT("restore revokes pre-save execution writer"), Ecology.Release(Lease, Error));
	TestEqual(TEXT("restore retains pending accepted time"), Clock.GetPendingGameSeconds(), 60.);
	TestTrue(TEXT("resume pending input"), CCLWorldAdvance::Advance(Clock, Life, 60., 3600., Error,
		256, &Surface, nullptr, nullptr, nullptr, nullptr, &Ecology));
	TestTrue(TEXT("resume exactly matches uninterrupted ecology"), Before == WorldEcologyBytes(Ecology));
	return true;
}
#endif
