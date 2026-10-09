#include "CCLSurfaceSimulation.h"

#include "Misc/AutomationTest.h"
#include "Misc/Crc.h"
#include "Serialization/MemoryWriter.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	FCCLSurfaceGrid SnowTestGrid()
	{
		FCCLSurfaceGrid G;
		G.RegionId = FGuid(5, 5, 5, 5);
		G.Size = FIntPoint(32, 32);
		G.SpacingMeters = 0.125;
		G.Cells.SetNum(1024);
		G.Forcing.TemperatureCelsius = 0.;
		G.Forcing.InfiltrationMetersPerWorldSecond = 0.;
		G.Forcing.DrainageMetersPerWorldSecond = 0.;
		G.Forcing.FlowConductance = 0.;
		for (auto& C : G.Cells)
		{
			C.SnowVolumeCubicMeters = 0.6 * 0.125 * 0.125;
			C.SnowCubicMeters = C.SnowVolumeCubicMeters * 0.1;
		}
		G.Ledger.InitialCubicMeters = G.TotalCubicMeters();
		return G;
	}
	FCCLWorldStep SnowStep(const FCCLSurfaceSimulation& S, double Game = 0.25, double World = 15.)
	{
		FCCLWorldStep Step;
		Step.Ticket = FGuid::NewGuid();
		Step.StepId = S.GetStepId() + 1;
		Step.GameFromSeconds = S.GetGameSeconds();
		Step.GameToSeconds = S.GetGameSeconds() + Game;
		Step.WorldFromSeconds = S.GetWorldSeconds();
		Step.WorldToSeconds = S.GetWorldSeconds() + World;
		return Step;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLSnowConservationTest, "CCL.Environment.Snow.AccumulateCompressMelt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLSnowConservationTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLSurfaceSimulation S;
	S.Initialize(FGuid::NewGuid(), 0., 0., 0, Error);
	auto G = SnowTestGrid();
	G.Forcing.SnowMetersPerWorldSecond = 0.00001;
	TestTrue(TEXT("add"), S.AddRegion(G, Error));
	TestTrue(TEXT("accumulate"), S.Advance(SnowStep(S), Error));
	const double Mass = S.FindRegion(G.RegionId)->TotalCubicMeters();
	TestTrue(TEXT("world-time precipitation exactly once"), FMath::IsNearlyEqual(Mass - G.TotalCubicMeters(), 16. * 0.00001 * 15., 1.e-10));
	const FGuid Walker = FGuid::NewGuid();
	for (uint64 I = 1; I <= 80; ++I)
	{
		const FVector At(0.25 + (I % 28) * 0.125, 2., 0.);
		TestTrue(TEXT("compact and displace"), S.ApplySnowContact(G.RegionId, Walker, I, At, 0.27, Error));
	}
	TestTrue(TEXT("mass survives repeated traversals"), FMath::IsNearlyEqual(S.FindRegion(G.RegionId)->TotalCubicMeters(), Mass, 1.e-10));
	FCCLSnowSample Sample;
	TestTrue(TEXT("sample trench"), S.SampleSnow(FVector(2., 2., 0.), Sample));
	TestTrue(TEXT("knee-depth layer compresses"), Sample.DepthMeters < 0.3 && Sample.DensityRatio > 0.2);
	auto F = G.Forcing;
	F.SnowMetersPerWorldSecond = 0.;
	F.TemperatureCelsius = 20.;
	F.PhaseMetersPerDegreeWorldSecond = 0.01;
	TestTrue(TEXT("thaw forcing"), S.ChangeForcing(G.RegionId, F, Error));
	TestTrue(TEXT("melt"), S.Advance(SnowStep(S), Error));
	double Liquid = 0., Snow = 0.;
	for (const auto& C : S.FindRegion(G.RegionId)->Cells)
	{
		Liquid += C.WaterCubicMeters;
		Snow += C.SnowCubicMeters;
	}
	TestEqual(TEXT("all snow melted"), Snow, 0.);
	TestTrue(TEXT("melt is internal conversion"), FMath::IsNearlyEqual(Liquid, Mass, 1.e-10));
	TestTrue(TEXT("ledger"), S.Validate(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLSnowReceiptTest, "CCL.Environment.Snow.ReceiptRestoreAndRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLSnowReceiptTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLSurfaceSimulation S;
	S.Initialize(FGuid::NewGuid(), 0., 0., 0, Error);
	auto G = SnowTestGrid();
	S.AddRegion(G, Error);
	const FGuid Source = FGuid::NewGuid();
	TestTrue(TEXT("contact"), S.ApplySnowContact(G.RegionId, Source, 7, FVector(2., 2., 0.), 0.27, Error));
	TArray<uint8> Before, After;
	S.Capture(Before, Error);
	TestTrue(TEXT("duplicate no-op"), S.ApplySnowContact(G.RegionId, Source, 7, FVector(2., 2., 0.), 0.27, Error));
	S.Capture(After, Error);
	TestTrue(TEXT("identical bytes"), Before == After);
	FCCLSurfaceSimulation Restored;
	TestTrue(TEXT("restore"), Restored.Restore(Before, Error));
	TestTrue(TEXT("old receipt remains no-op after restore"), Restored.ApplySnowContact(G.RegionId, Source, 6, FVector(1., 1., 0.), 0.27, Error));
	Restored.Capture(After, Error);
	TestTrue(TEXT("restored trail and receipt exact"), Before == After);
	TestFalse(TEXT("outside rejected"), Restored.ApplySnowContact(G.RegionId, Source, 8, FVector(100., 100., 0.), 0.27, Error));
	Restored.Capture(After, Error);
	TestTrue(TEXT("rejection retains receipt and snow"), Before == After);
	TestEqual(TEXT("receipt not consumed by failure"), Restored.LastSnowContact(Source), uint64(7));
	TestTrue(TEXT("reinitialize clears receipts"), Restored.Initialize(FGuid::NewGuid(), 0., 0., 0, Error));
	TestEqual(TEXT("new world has no contact"), Restored.LastSnowContact(Source), uint64(0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLSnowTerrainTest, "CCL.Environment.Snow.TerrainSupportAndCapacity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLSnowTerrainTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLSurfaceSimulation S;
	S.Initialize(FGuid::NewGuid(), 0., 0., 0, Error);
	auto G = SnowTestGrid();
	G.TerrainId = FGuid::NewGuid();
	G.TerrainRevision = 1;
	S.AddRegion(G, Error);
	TArray<double> Beds;
	Beds.Init(-1., G.Cells.Num());
	TestTrue(TEXT("excavation moves snow to new support"), S.RebaseTerrain(G.RegionId, Beds, 2, Error));
	TestTrue(TEXT("snow mass retained"), FMath::IsNearlyEqual(G.TotalCubicMeters(), S.FindRegion(G.RegionId)->TotalCubicMeters(), 1.e-12));
	TestEqual(TEXT("depth is unchanged"), S.FindRegion(G.RegionId)->SnowDepthMeters(0), 0.6);
	TArray<uint8> Before, After;
	S.Capture(Before, Error);
	Beds.Init(31.9, G.Cells.Num());
	TestFalse(TEXT("deposit without snow room refused"), S.RebaseTerrain(G.RegionId, Beds, 3, Error));
	S.Capture(After, Error);
	TestTrue(TEXT("whole edit rolled back"), Before == After);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLSnowLegacyTest, "CCL.Environment.Snow.LegacySurfaceAndSplitInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLSnowLegacyTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLSurfaceSimulation S;
	const FGuid Id = FGuid::NewGuid();
	S.Initialize(Id, 0., 0., 0, Error);
	auto G = SnowTestGrid();
	G.Forcing.SnowMetersPerWorldSecond = 0.00001;
	S.AddRegion(G, Error);
	auto Split = S;
	TestTrue(TEXT("one interval"), S.Advance(SnowStep(S, 0.5, 30.), Error));
	TestTrue(TEXT("first partition"), Split.Advance(SnowStep(Split), Error));
	TestTrue(TEXT("second partition"), Split.Advance(SnowStep(Split), Error));
	TestTrue(TEXT("input independent of partition count"), FMath::IsNearlyEqual(S.FindRegion(G.RegionId)->TotalCubicMeters(), Split.FindRegion(G.RegionId)->TotalCubicMeters(), 1.e-10));
	TArray<uint8> Bytes;
	FMemoryWriter Ar(Bytes);
	uint32 Magic = 0x534C4343, Version = 1;
	FGuid World = Id;
	double Game = 2., Time = 120.;
	uint64 Step = 8;
	int32 Count = 0;
	Ar << Magic << Version << World << Game << Time << Step << Count;
	uint32 CRC = FCrc::MemCrc32(Bytes.GetData(), Bytes.Num());
	Ar << CRC;
	TestTrue(TEXT("legacy surface v1 readable"), S.Restore(Bytes, Error));
	TestEqual(TEXT("legacy clock"), S.GetWorldSeconds(), Time);
	TestEqual(TEXT("legacy lacks snow grids"), S.GetRegions().Num(), 0);

	// Nonempty v1 fixture: every old water field must survive migration to v2.
	TArray<uint8> Populated;
	FMemoryWriter Legacy(Populated);
	Count = 1;
	Legacy << Magic << Version << World << Game << Time << Step << Count;
	auto Old = SnowTestGrid();
	Old.Size = FIntPoint(2, 2);
	Old.Cells.SetNum(4);
	for (auto& C : Old.Cells)
	{
		C.SnowCubicMeters = C.SnowVolumeCubicMeters = 0.;
		C.WaterCubicMeters = 0.002;
		C.IceCubicMeters = 0.001;
		C.SoilCubicMeters = 0.0001;
	}
	Old.Ledger.InitialCubicMeters = Old.TotalCubicMeters();
	Legacy << Old.RegionId << Old.TerrainId << Old.OriginMeters << Old.Size << Old.SpacingMeters << Old.Revision << Old.TerrainRevision;
	auto& F = Old.Forcing;
	Legacy << F.RainMetersPerWorldSecond << F.EvaporationMetersPerWorldSecond << F.InfiltrationMetersPerWorldSecond
		<< F.DrainageMetersPerWorldSecond << F.SoilCapacityMeters << F.TemperatureCelsius << F.PhaseMetersPerDegreeWorldSecond
		<< F.FlowConductance << F.BoundaryLevelMeters << F.BoundaryConductance;
	auto& L = Old.Ledger;
	Legacy << L.InitialCubicMeters << L.RainCubicMeters << L.BoundaryInCubicMeters << L.BoundaryOutCubicMeters << L.EvaporatedCubicMeters;
	int32 CellCount = Old.Cells.Num();
	Legacy << CellCount;
	for (auto& C : Old.Cells)
	{
		Legacy << C.BedMeters << C.CeilingMeters << C.WaterCubicMeters << C.IceCubicMeters << C.SoilCubicMeters << C.RainExposure << C.bOpenBoundary;
	}
	CRC = FCrc::MemCrc32(Populated.GetData(), Populated.Num());
	Legacy << CRC;
	TestTrue(TEXT("nonempty v1 migration"), S.Restore(Populated, Error));
	const auto* Migrated = S.FindRegion(Old.RegionId);
	if (TestNotNull(TEXT("migrated region"), Migrated))
	{
		TestEqual(TEXT("water mass preserved"), Migrated->TotalCubicMeters(), Old.TotalCubicMeters());
		TestEqual(TEXT("ice preserved"), Migrated->Cells[0].IceCubicMeters, 0.001);
		TestEqual(TEXT("soil preserved"), Migrated->Cells[0].SoilCubicMeters, 0.0001);
		TestEqual(TEXT("snow defaults to zero"), Migrated->Cells[0].SnowVolumeCubicMeters, 0.);
		TestEqual(TEXT("snow forcing defaults to zero"), Migrated->Forcing.SnowMetersPerWorldSecond, 0.);
	}
	return true;
}
#endif
