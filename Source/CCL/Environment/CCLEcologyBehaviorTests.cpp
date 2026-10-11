#include "CCLEcologyLegacyTest.h"
#include "KSIEcologySimulation.h"
#include "CCLWorldAdvance.h"
#include "KSIWorldClock.h"
#include "Agents/CCLLifeSimulation.h"
#include "Misc/AutomationTest.h"
#include "Misc/Crc.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	FGuid BehaviorId(uint32 N)
	{
		return FGuid(920, 921, 922, N);
	}

	TArray<uint8> Bytes(const FKSIEcologySimulation& S)
	{
		TArray<uint8> Result;
		FString Error;
		S.Capture(Result, Error);
		return Result;
	}

	bool Setup(FKSIEcologySimulation& S, FString& Error, bool bBehavior = true)
	{
		if (!S.Initialize(BehaviorId(1), 0., Error))
		{
			return false;
		}

		for (uint32 Index = 0; Index < 4; ++Index)
		{
			FKSIEcologySpecies Species;
			Species.Id = BehaviorId(10 + Index);
			Species.Movement = EKSIEcologyMovement(Index);
			Species.MaturitySeconds = 10.;
			Species.LifespanSeconds = 10000.;
			FKSIEcologyHabitat H;
			H.Id = BehaviorId(20 + Index);
			H.MaximumMeters = FVector(10., 10., 4.);
			H.MovementMask = 1 << Index;
			FKSIEcologyPopulation P;
			P.Id = BehaviorId(30 + Index);
			P.SpeciesId = Species.Id;
			P.HabitatId = H.Id;
			P.Cohorts.Add(BehaviorId(40 + Index), {BehaviorId(40 + Index), 0., 10});
			if (!S.AddSpecies(Species, Error) || !S.AddHabitat(H, Error) || !S.AddPopulation(P, Error)
				|| !S.Materialize(BehaviorId(50 + Index), P.Id, BehaviorId(40 + Index), BehaviorId(60 + Index), true, Error))
			{
				return false;
			}

			if (!bBehavior)
			{
				continue;
			}

			FKSIEcologyGroup G;
			G.Id = BehaviorId(70 + Index);
			G.HabitatId = H.Id;
			G.AnchorMeters = FVector(2., 2., 2.);
			G.RadiusMeters = 1.;
			FKSIEcologyForage F;
			F.Id = BehaviorId(80 + Index);
			F.HabitatId = H.Id;
			F.PositionMeters = FVector(8., 5., (Index == 1 || Index == 2) ? 3. : 2.);
			F.FoodMask = 1 << Index;
			F.RegrowthPerSecond = .1;
			FKSIEcologyBehavior B;
			B.Id = BehaviorId(60 + Index);
			B.GroupId = G.Id;
			B.DietMask = F.FoodMask;
			B.Energy = .25;
			B.MetabolismPerSecond = .02;
			B.SpeedMetersPerSecond = 2.;
			if (!S.AddGroup(G, Error) || !S.AddForage(F, Error) || !S.AddBehavior(B, Error))
			{
				return false;
			}
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLBehaviorPartitionTest, "CCL.Environment.Ecology.Behavior.FourMediaPartitionAndRestart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLBehaviorPartitionTest::RunTest(const FString& Parameters)
{
	FKSIEcologySimulation Whole, Split, Restored;
	FString Error;
	if (!TestTrue(TEXT("four media behavior fixture"), Setup(Whole, Error)))
	{
		return false;
	}

	Split = Whole;
	for (double At : {.25, .75, 1.5, 7.25})
	{
		TestTrue(TEXT("fractional partitions"), Split.AdvanceTo(At, Error));
	}

	TestTrue(TEXT("restart partial behavior interval"), Restored.Restore(Bytes(Split), Error));
	for (double At : {8., 8.1, 13.7, 40.})
	{
		TestTrue(TEXT("continue restored behavior"), Restored.AdvanceTo(At, Error));
	}

	TestTrue(TEXT("whole interval"), Whole.AdvanceTo(40., Error));
	TestTrue(TEXT("movement energy feeding and resource histories identical"), Bytes(Whole) == Bytes(Restored));
	for (uint32 Index = 0; Index < 4; ++Index)
	{
		const auto& B = Whole.GetBehaviors()[BehaviorId(60 + Index)];
		const auto& I = *Whole.FindIndividual(B.Id);
		const auto& F = Whole.GetForage()[BehaviorId(80 + Index)];
		TestTrue(TEXT("each medium actually ate"), B.ConsumedUnits > 0. && F.ConsumedUnits > 0.);
		TestTrue(TEXT("food ledger balances"), FMath::IsNearlyEqual(F.InitialUnits + F.GrownUnits, F.AvailableUnits + F.ConsumedUnits, 1.e-8));
		TestTrue(TEXT("position changed"), !I.PositionMeters.Equals(FVector(5., 5., 2.)));
		if (Index == 0 || Index == 3)
		{
			TestEqual(TEXT("planar media preserve plane"), I.PositionMeters.Z, 2.);
		}

		TestEqual(TEXT("movement does not change population"), Whole.LivingCount(BehaviorId(30 + Index)), int64(10));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLBehaviorGroupTest, "CCL.Environment.Ecology.Behavior.SatiatedGroupAndFractionalRegistration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLBehaviorGroupTest::RunTest(const FString& Parameters)
{
	FKSIEcologySimulation S;
	FString Error;
	if (!TestTrue(TEXT("fixture"), Setup(S, Error, false) && S.AdvanceTo(.75, Error)))
	{
		return false;
	}

	FKSIEcologyGroup G;
	G.Id = BehaviorId(70);
	G.HabitatId = BehaviorId(20);
	G.AnchorMeters = FVector(2., 2., 2.);
	G.RadiusMeters = 1.;
	TestTrue(TEXT("group"), S.AddGroup(G, Error));
	FKSIEcologyForage F;
	F.Id = BehaviorId(80);
	F.HabitatId = G.HabitatId;
	F.PositionMeters = G.AnchorMeters;
	F.InitialUnits = F.AvailableUnits = 0.;
	F.RegrowthPerSecond = 1.;
	TestTrue(TEXT("source registered at fractional time"), S.AddForage(F, Error));
	for (uint32 N : {60u, 160u})
	{
		if (N != 60)
		{
			TestTrue(TEXT("second member"), S.Materialize(BehaviorId(150), BehaviorId(30), BehaviorId(40), BehaviorId(N), true, Error));
		}

		FKSIEcologyBehavior B;
		B.Id = BehaviorId(N);
		B.GroupId = G.Id;
		B.Energy = 1.;
		B.MetabolismPerSecond = 0.;
		B.SpeedMetersPerSecond = 2.;
		TestTrue(TEXT("satiated group member"), S.AddBehavior(B, Error));
	}
	TestTrue(TEXT("first partial behavior second"), S.AdvanceTo(1., Error));
	TestEqual(TEXT("no regrowth fabricated before source creation"), S.GetForage()[F.Id].GrownUnits, .25);
	TestTrue(TEXT("movement budget starts at registration"), FVector::Dist(S.FindIndividual(BehaviorId(60))->PositionMeters, FVector(5., 5., 2.)) <= .500001);
	TestTrue(TEXT("return to group formation"), S.AdvanceTo(15., Error));
	for (uint32 N : {60u, 160u})
	{
		TestTrue(TEXT("both members remain near common group anchor"), FVector::Dist(S.FindIndividual(BehaviorId(N))->PositionMeters, G.AnchorMeters) <= G.RadiusMeters);
		TestEqual(TEXT("satiated member does not consume"), S.GetBehaviors()[BehaviorId(N)].ConsumedUnits, 0.);
	}
	TestTrue(TEXT("stable member offsets differ"), !S.FindIndividual(BehaviorId(60))->PositionMeters.Equals(S.FindIndividual(BehaviorId(160))->PositionMeters, .001));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLBehaviorCompetitionTest, "CCL.Environment.Ecology.Behavior.CompetitionDietStarvationAndLease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLBehaviorCompetitionTest::RunTest(const FString& Parameters)
{
	FKSIEcologySimulation S;
	FString Error;
	if (!TestTrue(TEXT("fixture"), Setup(S, Error, false)))
	{
		return false;
	}

	FKSIEcologyForage F;
	F.Id = BehaviorId(80);
	F.HabitatId = BehaviorId(20);
	F.PositionMeters = FVector(5., 5., 2.);
	F.InitialUnits = F.AvailableUnits = F.CapacityUnits = .5;
	TestTrue(TEXT("finite source"), S.AddForage(F, Error));
	FKSIEcologyLease Lease;
	for (uint32 N = 0; N < 3; ++N)
	{
		const FGuid Id = BehaviorId(60 + N * 100);
		if (N)
		{
			TestTrue(TEXT("another competitor"), S.Materialize(BehaviorId(150 + N), BehaviorId(30), BehaviorId(40), Id, true, Error));
		}

		FKSIEcologyBehavior B;
		B.Id = Id;
		B.Energy = 0.;
		B.MetabolismPerSecond = 0.;
		B.DietMask = N == 2 ? 2 : 1;
		B.EnergyPerUnit = 1.;
		B.StarvationLimitSeconds = 5.;
		TestTrue(TEXT("competitor behavior"), S.AddBehavior(B, Error));
		if (N == 1)
		{
			TestTrue(TEXT("active starvation victim"), S.Acquire(Id, EKSIEcologyRepresentation::Mass, BehaviorId(300), Lease, Error));
		}
	}
	TestTrue(TEXT("finite food shared once"), S.AdvanceTo(2., Error));
	TestEqual(TEXT("first stable ID consumed source"), S.GetBehaviors()[BehaviorId(60)].ConsumedUnits, .5);
	TestEqual(TEXT("competing ID cannot double consume"), S.GetBehaviors()[BehaviorId(160)].ConsumedUnits, 0.);
	TestEqual(TEXT("wrong diet cannot eat"), S.GetBehaviors()[BehaviorId(260)].ConsumedUnits, 0.);
	TestEqual(TEXT("source exhausted"), S.GetForage()[F.Id].AvailableUnits, 0.);
	TestTrue(TEXT("starvation boundary"), S.AdvanceTo(6., Error));
	TestEqual(TEXT("exact two deaths"), S.GetPopulations()[BehaviorId(30)].Died, int64(2));
	TestFalse(TEXT("starvation revokes Mass lease"), S.IsLeaseCurrent(Lease));
	TestTrue(TEXT("starvation retry no duplicate death"), S.AdvanceTo(6., Error));
	TestEqual(TEXT("same two deaths"), S.GetPopulations()[BehaviorId(30)].Died, int64(2));
	FKSIEcologySimulation Restored;
	TestTrue(TEXT("terminal history restored"), Restored.Restore(Bytes(S), Error) && Bytes(Restored) == Bytes(S));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLBehaviorFailureTest, "CCL.Environment.Ecology.Behavior.ValidationBudgetAndOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLBehaviorFailureTest::RunTest(const FString& Parameters)
{
	FKSIEcologySimulation S;
	FString Error;
	if (!TestTrue(TEXT("fixture"), Setup(S, Error)))
	{
		return false;
	}

	const auto Before = Bytes(S);
	FKSIEcologyForage Bad;
	Bad.Id = BehaviorId(900);
	Bad.HabitatId = BehaviorId(20);
	Bad.PositionMeters = FVector(100.);
	TestFalse(TEXT("out of habitat source"), S.AddForage(Bad, Error));
	Bad.PositionMeters = FVector(5., 5., 2.);
	Bad.GrownUnits = 1.;
	TestFalse(TEXT("forged source history"), S.AddForage(Bad, Error));
	TestFalse(TEXT("duplicate behavior cannot reset energy"), S.AddBehavior(S.GetBehaviors()[BehaviorId(60)], Error));
	TestFalse(TEXT("boundary budget fails"), S.AdvanceTo(30., Error, 2));
	TestTrue(TEXT("failed candidate preserves all behavior bytes"), Before == Bytes(S));
	FKSIEcologyLease Lease;
	TestTrue(TEXT("acquire Actor"), S.Acquire(BehaviorId(60), EKSIEcologyRepresentation::Actor, BehaviorId(901), Lease, Error));
	TestFalse(TEXT("representation cannot overwrite autonomous position"), S.UpdatePosition(Lease, FVector(4., 4., 2.), Error));
	TestTrue(TEXT("release preserves persistent behavior"), S.Release(Lease, Error));
	TestTrue(TEXT("dormant movement still progresses"), S.AdvanceTo(10., Error));
	TestTrue(TEXT("dormant individual ate"), S.GetBehaviors()[BehaviorId(60)].ConsumedUnits > 0.);
	TestEqual(TEXT("reservation remains counted"), S.AggregateCount(BehaviorId(30)), int64(9));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLBehaviorLegacyTest, "CCL.Environment.Ecology.Behavior.Version3AndCorruptRestore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLBehaviorLegacyTest::RunTest(const FString& Parameters)
{
	FKSIEcologySimulation Old, Read;
	FString Error;
	if (!TestTrue(TEXT("legacy-shaped fixture"), Setup(Old, Error, false)))
	{
		return false;
	}

	auto Legacy = Bytes(Old);
	CCLMakeEcologyV5Fixture(Legacy);
	Legacy.RemoveAt(Legacy.Num() - 16, 12); // Three empty version-4 maps precede the CRC.
	uint32 Version = 3;
	FMemory::Memcpy(Legacy.GetData() + 4, &Version, 4);
	uint32 Crc = FCrc::MemCrc32(Legacy.GetData(), Legacy.Num() - 4);
	FMemory::Memcpy(Legacy.GetData() + Legacy.Num() - 4, &Crc, 4);
	TestTrue(TEXT("version 3 migrates without fabricated behavior or food"), Read.Restore(Legacy, Error));
	TestTrue(TEXT("legacy state unchanged"), Bytes(Old) == Bytes(Read));
	TestTrue(TEXT("new behavior fixture"), Setup(Read, Error));
	const auto Before = Bytes(Read);
	auto Corrupt = Before;
	Corrupt[Corrupt.Num() - 12] ^= 64;
	TestFalse(TEXT("corrupt restore rejected"), Read.Restore(Corrupt, Error));
	TestTrue(TEXT("corruption leaves behavior intact"), Bytes(Read) == Before);
	Corrupt = Before;
	const double ForgedConsumed = 1.;
	FMemory::Memcpy(Corrupt.GetData() + Corrupt.Num() - 30, &ForgedConsumed, sizeof(double));
	Crc = FCrc::MemCrc32(Corrupt.GetData(), Corrupt.Num() - 4);
	FMemory::Memcpy(Corrupt.GetData() + Corrupt.Num() - 4, &Crc, 4);
	TestFalse(TEXT("valid CRC cannot hide mismatched consumption histories"), Read.Restore(Corrupt, Error));
	TestTrue(TEXT("invalid history preserves original"), Bytes(Read) == Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLBehaviorWorldFailureTest, "CCL.Environment.Ecology.Behavior.WorldCandidateFailureDiscardsConsumption",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLBehaviorWorldFailureTest::RunTest(const FString& Parameters)
{
	FKSIEcologySimulation E;
	FKSIWorldClock Clock;
	FCCLLifeSimulation Life;
	FString Error;
	if (!TestTrue(TEXT("world setup"), Setup(E, Error) && Life.Initialize(FCCLLifeSimulation::MerchantScenario(42), Error)
		&& Clock.QueueGameTime(120., false, Error)))
	{
		return false;
	}

	const auto Before = Bytes(E);
	TestFalse(TEXT("later Agent failure rejects prepared foraging"), CCLWorldAdvance::Advance(Clock, Life, 120., 120., Error,
		0, nullptr, nullptr, nullptr, nullptr, nullptr, &E));
	TestTrue(TEXT("food movement and energy preserved"), Bytes(E) == Before);
	TestEqual(TEXT("world time unchanged"), Clock.GetWorldSeconds(), 0.);
	TestTrue(TEXT("retry accepts foraging exactly once"), CCLWorldAdvance::Advance(Clock, Life, 120., 120., Error,
		256, nullptr, nullptr, nullptr, nullptr, nullptr, &E));
	TestTrue(TEXT("committed food consumption exists"), E.GetForage()[BehaviorId(80)].ConsumedUnits > 0.);
	TestEqual(TEXT("behavior world and life clocks match"), E.GetWorldSeconds(), Life.GetTime());
	return true;
}
#endif
