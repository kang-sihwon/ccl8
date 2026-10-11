#include "CCLEcologySimulation.h"
#include "Misc/AutomationTest.h"
#include "HAL/PlatformTime.h"
#include "Serialization/MemoryWriter.h"
#include "Misc/Crc.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	FGuid NId(uint32 Value)
	{
		return FGuid(941, 1, 0, Value);
	}

	TArray<uint8> SaveNutrition(const FCCLEcologySimulation& S)
	{
		TArray<uint8> Bytes;
		FString Error;
		S.Capture(Bytes, Error);
		return Bytes;
	}

	bool NutritionFixture(FCCLEcologySimulation& S, FString& Error, int32 Populations = 1, int64 Count = 10, bool bFood = false)
	{
		if (!S.Initialize(NId(1), 0., Error))
		{
			return false;
		}
		for (uint32 N = 0; N < uint32(Populations); ++N)
		{
			FCCLEcologySpecies Species;
			Species.Id = NId(10 + N);
			Species.Movement = ECCLEcologyMovement(N % 4);
			Species.LifespanSeconds = 1.e8;
			Species.AggregateDietMask = 1;
			Species.AggregateMetabolism = .25;
			Species.AggregateEnergyPerUnit = 1.;
			Species.AggregateStarvationSeconds = 2.;
			FCCLEcologyHabitat H;
			H.Id = NId(1000 + N);
			H.Capacity = Count;
			FCCLEcologyPopulation P;
			P.Id = NId(2000 + N);
			P.SpeciesId = Species.Id;
			P.HabitatId = H.Id;
			P.Cohorts.Add(NId(3000 + N), {NId(3000 + N), 0., Count});
			if (!S.AddSpecies(Species, Error) || !S.AddHabitat(H, Error) || !S.AddPopulation(P, Error))
			{
				return false;
			}
			if (bFood)
			{
				FCCLEcologyForage F;
				F.Id = NId(4000 + N);
				F.HabitatId = H.Id;
				F.InitialUnits = F.AvailableUnits = F.CapacityUnits = double(Count);
				F.RegrowthPerSecond = double(Count) * .25;
				if (!S.AddForage(F, Error))
				{
					return false;
				}
			}
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLEcologyNutritionTest, "CCL.Environment.Ecology.Nutrition.ReservationStarvationAndRestart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLEcologyNutritionTest::RunTest(const FString& Parameters)
{
	FCCLEcologySimulation Whole;
	FString Error;
	if (!TestTrue(TEXT("nutrition fixture"), NutritionFixture(Whole, Error)))
	{
		return false;
	}
	TestTrue(TEXT("consume initial reserve"), Whole.AdvanceTo(2., Error));
	TestEqual(TEXT("aggregate energy"), Whole.GetPopulations()[NId(2000)].Cohorts[NId(3000)].Nutrition.Energy, .5);
	TestTrue(TEXT("reserve from hungry aggregate"), Whole.Materialize(NId(5000), NId(2000), NId(3000), NId(5001), false, Error));
	TestEqual(TEXT("reservation inherits energy"), Whole.FindIndividual(NId(5001))->Nutrition.Energy, .5);
	FCCLEcologyLease Lease;
	TestTrue(TEXT("Mass execution"), Whole.Acquire(NId(5001), ECCLEcologyRepresentation::Mass, NId(9000), Lease, Error));
	TestTrue(TEXT("unmodeled reservation metabolizes"), Whole.AdvanceTo(3., Error));
	TestEqual(TEXT("reserved metabolism"), Whole.FindIndividual(NId(5001))->Nutrition.Energy, .25);
	TestTrue(TEXT("return preserves energy"), Whole.Release(Lease, Error));
	TestEqual(TEXT("returned energy"), Whole.GetPopulations()[NId(2000)].Cohorts[NId(3000)].Nutrition.Energy, .25);
	auto Before = SaveNutrition(Whole);
	TestFalse(TEXT("budget rejects whole candidate"), Whole.AdvanceTo(100., Error, 1));
	TestTrue(TEXT("budget failure preserves bytes"), Before == SaveNutrition(Whole));
	FCCLEcologySimulation Split;
	TestTrue(TEXT("restart"), Split.Restore(Before, Error));
	TestTrue(TEXT("whole starvation"), Whole.AdvanceTo(6., Error));
	for (double At : {3.25, 3.75, 4.5, 5.25, 6.})
	{
		TestTrue(TEXT("split starvation"), Split.AdvanceTo(At, Error));
	}
	TestTrue(TEXT("partition and restart identical"), SaveNutrition(Split) == SaveNutrition(Whole));
	TestEqual(TEXT("all hungry members died once"), Whole.GetPopulations()[NId(2000)].Died, int64(10));
	TestTrue(TEXT("empty population stays empty"), Whole.AdvanceTo(10., Error) && Whole.LivingCount(NId(2000)) == 0);

	FCCLEcologySimulation Reserved;
	TestTrue(TEXT("reserved death fixture"), NutritionFixture(Reserved, Error) &&
		Reserved.Materialize(NId(5010), NId(2000), NId(3000), NId(5011), false, Error) &&
		Reserved.Acquire(NId(5011), ECCLEcologyRepresentation::Actor, NId(9000), Lease, Error));
	TestTrue(TEXT("starvation revokes live representation"), Reserved.AdvanceTo(6., Error) && !Reserved.IsLeaseCurrent(Lease));
	TestEqual(TEXT("aggregate and reservation death counted once"), Reserved.GetPopulations()[NId(2000)].Died, int64(10));

	// A real v5 wire fixture has species policy fields absent, rather than a patched v6 header.
	TArray<uint8> Legacy;
	FMemoryWriter Writer(Legacy, true);
	uint32 Magic = 0x4F434345, Version = 5;
	FGuid World = NId(1), SpeciesId = NId(10);
	double Time = 40., Maturity = 10., Lifespan = 1000., Interval = 0.;
	int32 One = 1, Zero = 0;
	uint8 Movement = 0;
	Writer << Magic << Version << World << Time << One << SpeciesId << Movement << Maturity << Lifespan << Interval;
	for (int32 Array = 0; Array < 8; ++Array)
	{
		Writer << Zero;
	}
	uint32 Checksum = FCrc::MemCrc32(Legacy.GetData(), Legacy.Num());
	Writer << Checksum;
	FCCLEcologySimulation Migrated;
	TestTrue(TEXT("legacy species preserves disabled nutrition"), Migrated.Restore(Legacy, Error) &&
		Migrated.GetSpecies()[SpeciesId].AggregateDietMask == 0);

	FCCLEcologySimulation Fed;
	TestTrue(TEXT("four media food fixture"), NutritionFixture(Fed, Error, 4, 10, true));
	TestTrue(TEXT("food sustains aggregates"), Fed.AdvanceTo(30., Error));
	for (uint32 N = 0; N < 4; ++N)
	{
		TestEqual(TEXT("living count conserved"), Fed.LivingCount(NId(2000 + N)), int64(10));
		TestEqual(TEXT("food ledger equals aggregate intake"), Fed.GetForage()[NId(4000 + N)].ConsumedUnits, Fed.GetPopulations()[NId(2000 + N)].AggregateConsumedUnits);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLEcologyNutritionTransferTest, "CCL.Environment.Ecology.Nutrition.MigrationAndBehaviorTransfer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLEcologyNutritionTransferTest::RunTest(const FString& Parameters)
{
	FCCLEcologySimulation S;
	FString Error;
	if (!TestTrue(TEXT("hungry source"), NutritionFixture(S, Error) && S.AdvanceTo(2.5, Error)))
	{
		return false;
	}
	FCCLEcologyHabitat H;
	H.Id = NId(1010);
	FCCLEcologyPopulation P;
	P.Id = NId(2010);
	P.SpeciesId = NId(10);
	P.HabitatId = H.Id;
	if (!TestTrue(TEXT("empty receiving population"), S.AddHabitat(H, Error) && S.AddPopulation(P, Error)))
	{
		return false;
	}
	FCCLEcologyEvent Move;
	Move.Id = NId(6010);
	Move.Kind = ECCLEcologyEventKind::Migration;
	Move.Source = NId(2000);
	Move.Target = P.Id;
	Move.CohortId = NId(3000);
	Move.Count = 4;
	if (!TestTrue(TEXT("migrate at a fractional world time"), S.Apply(Move, Error)))
	{
		return false;
	}
	TestEqual(TEXT("migration charges to current time"), S.GetPopulations()[P.Id].Cohorts[Move.CohortId].Nutrition.Energy, .375);
	TestTrue(TEXT("move receipt remains idempotent"), S.Apply(Move, Error));
	TestEqual(TEXT("source count"), S.LivingCount(Move.Source), int64(6));
	TestEqual(TEXT("destination count"), S.LivingCount(P.Id), int64(4));
	if (!TestTrue(TEXT("persistent reservation"), S.Materialize(NId(6011), P.Id, Move.CohortId, NId(6012), true, Error)))
	{
		return false;
	}
	FCCLEcologyBehavior B;
	B.Id = NId(6012);
	B.Energy = 1.;
	B.MetabolismPerSecond = .25;
	B.StarvationLimitSeconds = 2.;
	if (!TestTrue(TEXT("behavior inherits nutrition instead of authored energy"), S.AddBehavior(B, Error)))
	{
		return false;
	}
	TestEqual(TEXT("behavior transferred energy"), S.GetBehaviors()[B.Id].Energy, .375);
	TestTrue(TEXT("one second empty energy is below death limit"), S.AdvanceTo(5., Error));
	TestEqual(TEXT("aggregate waits for actual starvation duration"), S.GetPopulations()[Move.Source].Died, int64(0));
	TestEqual(TEXT("behavior uses the same starvation duration"), S.GetPopulations()[P.Id].Died, int64(0));
	TestTrue(TEXT("all representations finish starvation"), S.AdvanceTo(6., Error));
	TestEqual(TEXT("source deaths"), S.GetPopulations()[Move.Source].Died, int64(6));
	TestEqual(TEXT("destination aggregate and behavioral deaths"), S.GetPopulations()[P.Id].Died, int64(4));
	TestTrue(TEXT("transfer ledger valid"), S.Validate(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLEcologyNutritionScaleTest, "CCL.Environment.Ecology.Nutrition.ScaleAndRepeatedRestore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLEcologyNutritionScaleTest::RunTest(const FString& Parameters)
{
	FCCLEcologySimulation S;
	FString Error;
	if (!TestTrue(TEXT("one million members in 100 aggregates"), NutritionFixture(S, Error, 100, 10000, true)))
	{
		return false;
	}
	const double Start = FPlatformTime::Seconds();
	int32 PeakBytes = 0;
	for (int32 Round = 1; Round <= 60; ++Round)
	{
		if (!TestTrue(TEXT("one simulated minute"), S.AdvanceTo(Round * 60., Error)))
		{
			return false;
		}
		const auto Bytes = SaveNutrition(S);
		PeakBytes = FMath::Max(PeakBytes, Bytes.Num());
		FCCLEcologySimulation Restored;
		if (!TestTrue(TEXT("repeated restore is exact"), Restored.Restore(Bytes, Error) && SaveNutrition(Restored) == Bytes))
		{
			return false;
		}
		S = MoveTemp(Restored);
	}
	int64 Living = 0;
	for (const auto& Pair : S.GetPopulations())
	{
		Living += S.LivingCount(Pair.Key);
	}
	TestEqual(TEXT("million members conserved after one world hour"), Living, int64(1000000));
	AddInfo(FString::Printf(TEXT("CCL_ECOLOGY_SCALE populations=100 members=1000000 world_seconds=3600 restores=60 wall_seconds=%.6f snapshot_bytes=%d receipts=%d"),
		FPlatformTime::Seconds() - Start, PeakBytes, S.ReceiptCount()));
	return true;
}
#endif
