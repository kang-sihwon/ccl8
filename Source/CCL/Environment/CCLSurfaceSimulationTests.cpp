#include "CCLSurfaceSimulation.h"

#include "CCLWorldAdvance.h"
#include "CCLWorldSnapshot.h"
#include "Agents/CCLLifeSimulation.h"
#include "Misc/AutomationTest.h"
#include "Misc/Crc.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	FCCLSurfaceGrid Grid(int32 X = 3, int32 Y = 3)
	{
		FCCLSurfaceGrid G;
		G.RegionId = FGuid(41, 42, 43, 44);
		G.Size = FIntPoint(X, Y);
		G.SpacingMeters = 1.;
		G.Cells.SetNum(X * Y);
		G.Forcing.InfiltrationMetersPerWorldSecond = 0.;
		G.Forcing.DrainageMetersPerWorldSecond = 0.;
		G.Forcing.TemperatureCelsius = 0.;
		return G;
	}

	FCCLWorldStep Step(const FCCLSurfaceSimulation& S, double GameDelta, double WorldDelta)
	{
		FCCLWorldStep Result;
		Result.Ticket = FGuid::NewGuid();
		Result.StepId = S.GetStepId() + 1;
		Result.GameFromSeconds = S.GetGameSeconds();
		Result.GameToSeconds = S.GetGameSeconds() + GameDelta;
		Result.WorldFromSeconds = S.GetWorldSeconds();
		Result.WorldToSeconds = S.GetWorldSeconds() + WorldDelta;
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWaterFlowTest, "CCL.Environment.Water.FrozenFluxAndConservation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLWaterFlowTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLSurfaceSimulation S;
	TestTrue(TEXT("initialize"), S.Initialize(FGuid::NewGuid(), 0., 0., 0, Error));
	auto G = Grid();
	G.Cells[4].WaterCubicMeters = 4.;
	G.Ledger.InitialCubicMeters = 4.;
	G.Forcing.FlowConductance = 1000.;
	TestTrue(TEXT("grid"), S.AddRegion(G, Error));
	TestTrue(TEXT("step"), S.Advance(Step(S, 0.25, 0.), Error));
	const auto& C = S.FindRegion(G.RegionId)->Cells;
	TestEqual(TEXT("all four donors share one frozen limit"), C[1].WaterCubicMeters, 1.);
	TestEqual(TEXT("east symmetric"), C[5].WaterCubicMeters, C[1].WaterCubicMeters);
	TestEqual(TEXT("west symmetric"), C[3].WaterCubicMeters, C[1].WaterCubicMeters);
	TestEqual(TEXT("south symmetric"), C[7].WaterCubicMeters, C[1].WaterCubicMeters);
	for (int32 I = 0; I < 200; ++I)
	{
		if (!TestTrue(TEXT("stable repeated flow"), S.Advance(Step(S, 0.25, 15.), Error)))
		{
			AddError(Error);
			return false;
		}
	}
	TestTrue(TEXT("closed grid conserves mass"), FMath::Abs(S.FindRegion(G.RegionId)->BalanceErrorCubicMeters()) < 1.e-9);
	TestTrue(TEXT("finite nonnegative cells"), S.Validate(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWaterInputsTest, "CCL.Environment.Water.WorldInputsPhaseAndBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLWaterInputsTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLSurfaceSimulation S;
	S.Initialize(FGuid::NewGuid(), 0., 0., 0, Error);
	auto G = Grid(2, 1);
	G.Forcing.RainMetersPerWorldSecond = 0.001;
	G.Cells[1].RainExposure = 0.;
	G.Forcing.FlowConductance = 0.;
	TestTrue(TEXT("grid"), S.AddRegion(G, Error));
	TestTrue(TEXT("accelerated input"), S.Advance(Step(S, 1., 600.), Error));
	TestTrue(TEXT("rain integrated once across substeps"), FMath::IsNearlyEqual(S.FindRegion(G.RegionId)->TotalCubicMeters(), 0.6, 1.e-9));
	TestEqual(TEXT("roof excludes rain"), S.FindRegion(G.RegionId)->Cells[1].WaterCubicMeters, 0.);
	auto F = G.Forcing;
	F.RainMetersPerWorldSecond = 0.;
	F.TemperatureCelsius = -10.;
	F.PhaseMetersPerDegreeWorldSecond = 0.01;
	TestTrue(TEXT("freeze input"), S.ChangeForcing(G.RegionId, F, Error));
	TestTrue(TEXT("freeze"), S.Advance(Step(S, 0.25, 10.), Error));
	TestTrue(TEXT("ice remains water equivalent"), FMath::IsNearlyEqual(S.FindRegion(G.RegionId)->Cells[0].IceCubicMeters, 0.6, 1.e-9));
	F.TemperatureCelsius = 10.;
	F.InfiltrationMetersPerWorldSecond = 0.01;
	F.SoilCapacityMeters = 0.1;
	TestTrue(TEXT("thaw input"), S.ChangeForcing(G.RegionId, F, Error));
	TestTrue(TEXT("thaw and wet"), S.Advance(Step(S, 0.5, 10.), Error));
	TestTrue(TEXT("mud derived from thaw water and soil"), S.FindRegion(G.RegionId)->Mud(0) > 0.9);
	F.EvaporationMetersPerWorldSecond = 0.001;
	S.ChangeForcing(G.RegionId, F, Error);
	TestTrue(TEXT("evaporation"), S.Advance(Step(S, 0.25, 100.), Error));
	TestTrue(TEXT("phase and evaporation ledger"), FMath::Abs(S.FindRegion(G.RegionId)->BalanceErrorCubicMeters()) < 1.e-9);

	FCCLSurfaceSimulation Boundary;
	Boundary.Initialize(FGuid::NewGuid(), 0., 0., 0, Error);
	auto Coast = Grid(1, 1);
	Coast.Cells[0].bOpenBoundary = 1;
	Coast.Forcing.BoundaryLevelMeters = 1.;
	Coast.Forcing.BoundaryConductance = 4.;
	TestTrue(TEXT("coast"), Boundary.AddRegion(Coast, Error));
	TestTrue(TEXT("coast inflow"), Boundary.Advance(Step(Boundary, 0.25, 0.), Error));
	TestEqual(TEXT("explicit reservoir inflow"), Boundary.FindRegion(Coast.RegionId)->Ledger.BoundaryInCubicMeters, 1.);
	Coast.Forcing.BoundaryLevelMeters = 0.;
	Boundary.ChangeForcing(Coast.RegionId, Coast.Forcing, Error);
	TestTrue(TEXT("coast outflow"), Boundary.Advance(Step(Boundary, 0.25, 0.), Error));
	TestEqual(TEXT("explicit reservoir outflow"), Boundary.FindRegion(Coast.RegionId)->Ledger.BoundaryOutCubicMeters, 1.);
	TestEqual(TEXT("empty grid creates no water"), Boundary.FindRegion(Coast.RegionId)->TotalCubicMeters(), 0.);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWaterTerrainTest, "CCL.Environment.Water.TerrainDisplacementRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLWaterTerrainTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLSurfaceSimulation S;
	S.Initialize(FGuid::NewGuid(), 0., 0., 0, Error);
	auto G = Grid(2, 1);
	for (auto& C : G.Cells)
	{
		C.CeilingMeters = 2.;
	}
	G.Cells[0].WaterCubicMeters = 2.;
	G.Ledger.InitialCubicMeters = 2.;
	TestTrue(TEXT("grid"), S.AddRegion(G, Error));
	TestTrue(TEXT("deposit redistributes within connected capacity"), S.RebaseTerrain(G.RegionId, {1.5, 0.}, 1, Error));
	TestEqual(TEXT("remaining volume"), S.FindRegion(G.RegionId)->Cells[0].WaterCubicMeters, 0.5);
	TestEqual(TEXT("displaced volume"), S.FindRegion(G.RegionId)->Cells[1].WaterCubicMeters, 1.5);
	TArray<uint8> Before, After;
	S.Capture(Before, Error);
	TestFalse(TEXT("sealed deposit rejects without losing mass"), S.RebaseTerrain(G.RegionId, {2., 2.}, 2, Error));
	S.Capture(After, Error);
	TestTrue(TEXT("failed terrain is byte identical"), Before == After);
	TestTrue(TEXT("excavation increases capacity"), S.RebaseTerrain(G.RegionId, {-1., 0.}, 2, Error));
	TestTrue(TEXT("flow reacts to lower bed"), S.Advance(Step(S, 0.25, 0.), Error));
	TestTrue(TEXT("flow enters excavated cell"), S.FindRegion(G.RegionId)->Cells[0].WaterCubicMeters > 0.5);
	TestEqual(TEXT("terrain did not create or destroy water"), S.FindRegion(G.RegionId)->TotalCubicMeters(), 2.);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWaterAtomicTest, "CCL.Environment.Water.WorldCommitAndRetry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLWaterAtomicTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLLifeSimulation Life;
	FCCLWorldClock Clock;
	FCCLSurfaceSimulation S;
	TestTrue(TEXT("life"), Life.Initialize(FCCLLifeSimulation::MerchantScenario(42), Error));
	S.Initialize(FGuid::NewGuid(), 0., 0., 0, Error);
	auto G = Grid(1, 1);
	G.Forcing.RainMetersPerWorldSecond = 0.00001;
	S.AddRegion(G, Error);
	Clock.QueueGameTime(120., false, Error);
	TArray<uint8> Before, After;
	S.Capture(Before, Error);
	TestFalse(TEXT("life budget failure rejects prepared surface"), CCLWorldAdvance::Advance(Clock, Life, 120., 7200., Error, 1, &S));
	S.Capture(After, Error);
	TestTrue(TEXT("surface unchanged after life failure"), Before == After);
	TestEqual(TEXT("clock unchanged"), Clock.GetGameSeconds(), 0.);
	TestTrue(TEXT("retry"), CCLWorldAdvance::Advance(Clock, Life, 120., 7200., Error, 2, &S));
	TestTrue(TEXT("rain applied exactly once"), FMath::IsNearlyEqual(S.FindRegion(G.RegionId)->TotalCubicMeters(), 0.072, 1.e-9));
	TestEqual(TEXT("three domains agree"), S.GetWorldSeconds(), Life.GetTime());
	S.Capture(Before, Error);
	TestFalse(TEXT("surface budget rejects oversized physical step"), S.Advance(Step(S, 1025., 0.), Error));
	S.Capture(After, Error);
	TestTrue(TEXT("surface budget failure unchanged"), Before == After);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWaterSnapshotTest, "CCL.Environment.Water.BoundedSnapshotAndIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLWaterSnapshotTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLSurfaceSimulation S;
	const FGuid World = FGuid::NewGuid();
	S.Initialize(World, 0., 0., 0, Error);
	auto G = Grid();
	G.Cells[0].WaterCubicMeters = 3.;
	G.Ledger.InitialCubicMeters = 3.;
	S.AddRegion(G, Error);
	TArray<uint8> Bytes, Again;
	TestTrue(TEXT("capture"), S.Capture(Bytes, Error));
	FCCLSurfaceSimulation Restored;
	TestTrue(TEXT("restore"), Restored.Restore(Bytes, Error));
	TestTrue(TEXT("recapture"), Restored.Capture(Again, Error));
	TestTrue(TEXT("canonical roundtrip"), Bytes == Again);
	Bytes[32] ^= 1;
	TestFalse(TEXT("corrupt input rejected"), Restored.Restore(Bytes, Error));
	Restored.Capture(Bytes, Error);
	TestTrue(TEXT("corruption retains state"), Bytes == Again);
	const int32 Huge = MAX_int32;
	FMemory::Memcpy(Bytes.GetData() + 48, &Huge, 4);
	uint32 CRC = FCrc::MemCrc32(Bytes.GetData(), Bytes.Num() - 4);
	FMemory::Memcpy(Bytes.GetData() + Bytes.Num() - 4, &CRC, 4);
	TestFalse(TEXT("malicious region count with valid CRC"), Restored.Restore(Bytes, Error));
	FCCLLifeSimulation Life;
	FCCLWorldClock Clock;
	Life.Initialize(FCCLLifeSimulation::MerchantScenario(42), Error);
	FCCLWorldIdentity Identity;
	Identity.WorldId = World;
	FCCLWorldSnapshot Snapshot;
	TestTrue(TEXT("world envelope includes surface"), FCCLWorldSnapshotCodec::Capture(Identity, Clock, Life, Snapshot, Error, nullptr, &S));
	TestTrue(TEXT("world capture"), FCCLWorldSnapshotCodec::Encode(Snapshot, Bytes, Error));
	Identity.WorldId = FGuid::NewGuid();
	TestFalse(TEXT("different surface world cannot be saved"), FCCLWorldSnapshotCodec::Capture(Identity, Clock, Life, Snapshot, Error, nullptr, &S));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWaterFrontTest, "CCL.Environment.Water.DryFrontRoundoffRegression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLWaterFrontTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLSurfaceSimulation S;
	S.Initialize(FGuid::NewGuid(), 0., 0., 0, Error);
	auto G = Grid(32, 32);
	G.SpacingMeters = 0.5;
	G.Forcing.InfiltrationMetersPerWorldSecond = 0.0000005;
	G.Forcing.DrainageMetersPerWorldSecond = 0.00000005;
	for (int32 I = 0; I < G.Cells.Num(); ++I)
	{
		G.Cells[I].BedMeters = 4.;
		const int32 X = I % 32, Y = I / 32;
		G.Cells[I].WaterCubicMeters = X >= 12 && X < 20 && Y >= 12 && Y < 23 ? 0.125 : 0.;
	}
	G.Ledger.InitialCubicMeters = G.TotalCubicMeters();
	TestTrue(TEXT("sharp wet/dry front"), S.AddRegion(G, Error));
	for (int32 I = 0; I < 1200; ++I)
	{
		if (!S.Advance(Step(S, 0.25, 15.), Error))
		{
			AddError(FString::Printf(TEXT("step %d: %s"), I, *Error));
			return false;
		}
	}
	TestTrue(TEXT("thousands of wet/dry exchanges retain all mass"), FMath::Abs(S.FindRegion(G.RegionId)->BalanceErrorCubicMeters()) < 1.e-8);
	return true;
}
#endif
