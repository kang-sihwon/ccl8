#include "CCLSurfaceSimulation.h"

#include "Misc/AutomationTest.h"
#include "Misc/Crc.h"
#include "Serialization/MemoryWriter.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	FCCLSurfaceGrid FireGrid(int32 X = 1, int32 Y = 1)
	{
		FCCLSurfaceGrid G;
		G.RegionId = FGuid(701, 702, 703, 704);
		G.Size = FIntPoint(X, Y);
		G.SpacingMeters = 1.;
		G.Cells.SetNum(X * Y);
		G.Fire.Cells.SetNum(X * Y);
		G.Forcing.TemperatureCelsius = 20.;
		G.Forcing.InfiltrationMetersPerWorldSecond = 0.;
		G.Forcing.DrainageMetersPerWorldSecond = 0.;
		G.Forcing.FlowConductance = 0.;
		G.Forcing.PhaseMetersPerDegreeWorldSecond = 0.;
		G.Weather.WindMetersPerSecond = 0.;
		FString Error;
		for (auto& Cell : G.Fire.Cells)
		{
			FCCLCombustionModel::Initialize(G.Fire.Material, 2., 20., Cell, Error);
		}
		return G;
	}

	FCCLWorldStep FireStep(const FCCLSurfaceSimulation& S, double GameDelta, double WorldDelta = 0.)
	{
		FCCLWorldStep Step;
		Step.StepId = S.GetStepId() + 1;
		Step.GameFromSeconds = S.GetGameSeconds();
		Step.GameToSeconds = S.GetGameSeconds() + GameDelta;
		Step.WorldFromSeconds = S.GetWorldSeconds();
		Step.WorldToSeconds = S.GetWorldSeconds() + WorldDelta;
		return Step;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLSurfaceFirePhaseTest, "CCL.Environment.Fire.SurfaceMeltDryAndQuench",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLSurfaceFirePhaseTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLSurfaceSimulation S;
	TestTrue(TEXT("world"), S.Initialize(FGuid::NewGuid(), 0., 0., 0, Error));
	auto G = FireGrid(3);
	G.Fire.SpreadWattsPerKelvin = 0.;
	G.Cells[0].IceCubicMeters = 0.000001;
	G.Cells[0].SnowCubicMeters = 0.0001;
	G.Cells[0].SnowVolumeCubicMeters = 0.001;
	G.Cells[1].SoilCubicMeters = 0.001;
	G.Cells[2].WaterCubicMeters = 0.001;
	G.Ledger.InitialCubicMeters = G.TotalCubicMeters();
	TestTrue(TEXT("region"), S.AddRegion(G, Error));
	for (int32 I = 0; I < 3; ++I)
	{
		TestTrue(TEXT("heat"), S.Ignite(G.RegionId, I, 600000., Error));
	}
	TestTrue(TEXT("thermal coupling"), S.Advance(FireStep(S, 5.), Error));
	const auto& R = *S.FindRegion(G.RegionId);
	TestTrue(TEXT("fire melts ice"), R.Cells[0].IceCubicMeters < G.Cells[0].IceCubicMeters);
	TestTrue(TEXT("fire melts snow"), R.Cells[0].SnowCubicMeters < G.Cells[0].SnowCubicMeters);
	TestTrue(TEXT("snow volume follows remaining mass"), FMath::IsNearlyEqual(R.Cells[0].SnowVolumeCubicMeters,
		R.Cells[0].SnowCubicMeters * 10., 1.e-12));
	TestTrue(TEXT("hot wet soil dries"), R.Cells[1].SoilCubicMeters < G.Cells[1].SoilCubicMeters);
	TestTrue(TEXT("standing water cools fuel"), R.Cells[2].WaterCubicMeters < G.Cells[2].WaterCubicMeters);
	TestEqual(TEXT("wet soil inhibits burning"), R.Fire.Cells[1].BurnedKg, 0.);
	TestEqual(TEXT("standing water inhibits burning"), R.Fire.Cells[2].BurnedKg, 0.);
	TestTrue(TEXT("dry snowy fuel initially burns"), R.Fire.Cells[0].BurnedKg > 0.);
	TestTrue(TEXT("water remains accounted"), FMath::Abs(R.BalanceErrorCubicMeters()) < 1.e-12);
	TestTrue(TEXT("vapour reported once"), FMath::IsNearlyEqual(R.Ledger.EvaporatedCubicMeters * 1000., R.Fire.VaporizedWaterKg, 1.e-10));
	TestTrue(TEXT("all heat transfers accounted"), R.Fire.Validate(R.Cells.Num(), Error));
	TestEqual(TEXT("no erosion from burning"), R.Cells[0].BedMeters, G.Cells[0].BedMeters);

	auto Rain = R.Forcing;
	Rain.RainMetersPerWorldSecond = 0.01;
	TestTrue(TEXT("rain forcing"), S.ChangeForcing(G.RegionId, Rain, Error));
	const double Burned = S.FindRegion(G.RegionId)->Fire.Cells[0].BurnedKg;
	TestTrue(TEXT("rain extinguishes"), S.Advance(FireStep(S, 1., 1.), Error));
	TestEqual(TEXT("rain stops further fuel loss"), S.FindRegion(G.RegionId)->Fire.Cells[0].BurnedKg, Burned);
	TestFalse(TEXT("wet flame off"), S.FindRegion(G.RegionId)->Fire.Cells[0].bBurning != 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLSurfaceFireSpreadTest, "CCL.Environment.Fire.NeighbourSpreadAndFrozenBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLSurfaceFireSpreadTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLSurfaceSimulation S;
	S.Initialize(FGuid::NewGuid(), 0., 0., 0, Error);
	auto G = FireGrid(3, 3);
	G.Fire.Material.CoolingWattsPerKelvin = 0.;
	G.Fire.Material.SurfaceWattsPerKelvin = 0.;
	G.Fire.Material.BurnKgPerGameSecond = 0.;
	G.Fire.SpreadWattsPerKelvin = 1.e6;
	TestTrue(TEXT("region"), S.AddRegion(G, Error));
	TestTrue(TEXT("central heat"), S.Ignite(G.RegionId, 4, 800000., Error));
	TestTrue(TEXT("bounded spread"), S.Advance(FireStep(S, 0.25), Error));
	const auto& Fire = S.FindRegion(G.RegionId)->Fire;
	const double Initial = G.Fire.Cells[0].HeatJoules;
	TestEqual(TEXT("four sides share half donor excess"), Fire.Cells[4].HeatJoules, Initial + 400000.);
	for (int32 I : {1, 3, 5, 7})
	{
		TestEqual(TEXT("symmetric neighbour heat"), Fire.Cells[I].HeatJoules, Initial + 100000.);
	}
	TestEqual(TEXT("newly received heat not spent in same step"), Fire.Cells[0].HeatJoules, Initial);
	TestTrue(TEXT("frozen transfer ledger"), Fire.Validate(9, Error));

	auto Spread = FireGrid(2);
	Spread.Fire.Material.CoolingWattsPerKelvin = 0.;
	Spread.Fire.Material.SurfaceWattsPerKelvin = 0.;
	FCCLSurfaceSimulation A, B;
	A.Initialize(FGuid::NewGuid(), 0., 0., 0, Error);
	B.Initialize(FGuid::NewGuid(), 0., 0., 0, Error);
	TestTrue(TEXT("connected region"), A.AddRegion(Spread, Error));
	Spread.Cells[1].BedMeters = 33.;
	Spread.Cells[1].CeilingMeters = 34.;
	TestTrue(TEXT("disconnected elevation"), B.AddRegion(Spread, Error));
	A.Ignite(Spread.RegionId, 0, 800000., Error);
	B.Ignite(Spread.RegionId, 0, 800000., Error);
	TestTrue(TEXT("connected fire advances"), A.Advance(FireStep(A, 60.), Error));
	TestTrue(TEXT("disconnected fire advances"), B.Advance(FireStep(B, 60.), Error));
	TestTrue(TEXT("neighbour ignites"), A.FindRegion(Spread.RegionId)->Fire.Cells[1].BurnedKg > 0.);
	TestEqual(TEXT("disconnected cell remains unburned"), B.FindRegion(Spread.RegionId)->Fire.Cells[1].BurnedKg, 0.);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLSurfaceFireResumeTest, "CCL.Environment.Fire.SurfaceSaveResumeAndRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLSurfaceFireResumeTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLSurfaceSimulation S, Restored;
	S.Initialize(FGuid::NewGuid(), 0., 0., 0, Error);
	auto G = FireGrid(2);
	G.Cells[1].SnowCubicMeters = 0.001;
	G.Cells[1].SnowVolumeCubicMeters = 0.01;
	G.Ledger.InitialCubicMeters = G.TotalCubicMeters();
	TestTrue(TEXT("region"), S.AddRegion(G, Error));
	TestTrue(TEXT("ignite"), S.Ignite(G.RegionId, 0, 800000., Error));
	TestTrue(TEXT("advance"), S.Advance(FireStep(S, 7.), Error));
	TArray<uint8> Bytes, A, B;
	TestTrue(TEXT("capture"), S.Capture(Bytes, Error));
	TestTrue(TEXT("restore"), Restored.Restore(Bytes, Error));
	TestTrue(TEXT("original resume"), S.Advance(FireStep(S, 9., 540.), Error));
	TestTrue(TEXT("loaded resume"), Restored.Advance(FireStep(Restored, 9., 540.), Error));
	TestTrue(TEXT("capture original"), S.Capture(A, Error));
	TestTrue(TEXT("capture restored"), Restored.Capture(B, Error));
	TestTrue(TEXT("fuel heat smoke water and time identical"), A == B);
	TestFalse(TEXT("invalid ignition refused"), S.Ignite(G.RegionId, 0, std::numeric_limits<double>::quiet_NaN(), Error));
	TestFalse(TEXT("invalid index refused"), S.Ignite(G.RegionId, 4, 5., Error));
	S.Capture(B, Error);
	TestTrue(TEXT("failed ignition retains bytes and revision"), A == B);
	G.Forcing.TemperatureCelsius = -500.;
	TestTrue(TEXT("forcing valid for surface but outside fire model"), S.ChangeForcing(G.RegionId, G.Forcing, Error));
	S.Capture(A, Error);
	TestFalse(TEXT("downstream combustion failure"), S.Advance(FireStep(S, 1., 1.), Error));
	S.Capture(B, Error);
	TestTrue(TEXT("downstream failure retains complete generation"), A == B);
	Bytes[Bytes.Num() / 2] ^= 1;
	TestFalse(TEXT("corrupt payload refused"), S.Restore(Bytes, Error));
	S.Capture(B, Error);
	TestTrue(TEXT("failed restore retains previous generation"), A == B);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLSurfaceFireLegacyTest, "CCL.Environment.Fire.LegacySurfaceVersions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLSurfaceFireLegacyTest::RunTest(const FString& Parameters)
{
	FString Error;
	for (uint32 Version = 1; Version <= 6; ++Version)
	{
		auto G = FireGrid();
		if (Version < 5)
		{
			G.Fire.Cells.Reset();
		}
		G.Cells[0].WaterCubicMeters = 0.5;
		G.Ledger.InitialCubicMeters = 0.5;
		TArray<uint8> Bytes;
		FMemoryWriter Ar(Bytes);
		uint32 Magic = 0x534C4343;
		FGuid World(1, 2, 3, 4);
		double Game = 0., Time = 0.;
		uint64 Step = 0;
		int32 Count = 1, Contacts = 0;
		Ar << Magic << Version << World << Game << Time << Step << Count;
		Ar << G.RegionId << G.TerrainId << G.OriginMeters << G.Size << G.SpacingMeters << G.Revision << G.TerrainRevision;
		auto& F = G.Forcing;
		Ar << F.RainMetersPerWorldSecond << F.EvaporationMetersPerWorldSecond << F.InfiltrationMetersPerWorldSecond
			<< F.DrainageMetersPerWorldSecond << F.SoilCapacityMeters << F.TemperatureCelsius << F.PhaseMetersPerDegreeWorldSecond
			<< F.FlowConductance << F.BoundaryLevelMeters << F.BoundaryConductance;
		if (Version >= 2)
		{
			Ar << F.SnowMetersPerWorldSecond;
		}
		if (Version >= 3)
		{
			FCCLWeatherModel::Serialize(Ar, G.Climate, G.Weather);
		}
		if (Version >= 4)
		{
			Ar << G.bDynamicShelter;
		}
		auto& L = G.Ledger;
		Ar << L.InitialCubicMeters << L.RainCubicMeters << L.BoundaryInCubicMeters << L.BoundaryOutCubicMeters << L.EvaporatedCubicMeters;
		if (Version >= 2)
		{
			Ar << L.SnowCubicMeters;
		}
		Ar << Count;
		auto& C = G.Cells[0];
		Ar << C.BedMeters << C.CeilingMeters << C.WaterCubicMeters << C.IceCubicMeters << C.SoilCubicMeters << C.RainExposure << C.bOpenBoundary;
		if (Version >= 2)
		{
			Ar << C.SnowCubicMeters << C.SnowVolumeCubicMeters;
		}
		if (Version >= 4)
		{
			Ar << C.SunExposure << C.WindExposure;
		}
		if (Version >= 5)
		{
			G.Fire.Serialize(Ar, Count, false);
		}
		if (Version >= 6)
		{
			Ar << G.AirHeatToSurfaceJoules << G.SurfaceHeatToAirJoules;
		}

		if (Version >= 2)
		{
			Ar << Contacts;
		}

		if (Version >= 6)
		{
			FCCLAirSimulation Air;
			Air.Serialize(Ar);
		}

		uint32 CRC = FCrc::MemCrc32(Bytes.GetData(), Bytes.Num());
		Ar << CRC;
		FCCLSurfaceSimulation Loaded;
		TestTrue(TEXT("legacy payload accepted"), Loaded.Restore(Bytes, Error));
		const auto* Region = Loaded.FindRegion(G.RegionId);
		if (!TestNotNull(TEXT("region restored"), Region))
		{
			return false;
		}
		TestTrue(TEXT("legacy has no invented vegetation"), Region->Vegetation.Cells.IsEmpty());
		TestTrue(TEXT("legacy fuel state preserved"), Version < 5 ? Region->Fire.Cells.IsEmpty() : Region->Fire.Cells.Num() == 1 && Region->Fire.Cells[0].FuelKg == 2.);
		TestTrue(TEXT("old saves do not gain rooms"), Loaded.GetAirSimulation().GetSpaces().IsEmpty());
		TestEqual(TEXT("water preserved"), Region->Cells[0].WaterCubicMeters, 0.5);
	}
	return true;
}
#endif
