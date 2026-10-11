#include "KSIWeatherModel.h"

#include "KSISurfaceSimulation.h"
#include "KSIWorldClock.h"
#include "KSICelestialSystem.h"
#include "KSIEnvironmentInputs.h"
#include "CCLWorldAdvance.h"
#include "Agents/CCLLifeSimulation.h"
#include "Misc/AutomationTest.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	FKSISurfaceGrid WeatherGrid()
	{
		FKSISurfaceGrid G;
		G.RegionId = FGuid(201, 202, 203, 204);
		G.Climate.bEnabled = 1;
		G.Size = FIntPoint(2, 1);
		G.SpacingMeters = 1.;
		G.Cells.SetNum(2);
		G.Cells[1].RainExposure = 0.;
		G.Forcing.FlowConductance = 0.;
		return G;
	}

	FKSIWorldStep WeatherStep(const FKSISurfaceSimulation& S, double Delta = 15.)
	{
		FKSIWorldStep Step;
		Step.StepId = S.GetStepId() + 1;
		Step.GameFromSeconds = S.GetGameSeconds();
		Step.GameToSeconds = Step.GameFromSeconds + 0.25;
		Step.WorldFromSeconds = S.GetWorldSeconds();
		Step.WorldToSeconds = Step.WorldFromSeconds + Delta;
		return Step;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWeatherResponseTest, "CCL.Environment.Weather.SolarMoistureAndValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLWeatherResponseTest::RunTest(const FString& Parameters)
{
	FKSIClimate Climate;
	Climate.bEnabled = 1;
	FKSIWeatherState Day, Night;
	FString Error;
	double Rain = 0., Snow = 0.;
	for (int32 I = 0; I < 480; ++I)
	{
		if (!FKSIWeatherModel::Advance(Climate, I * 15., 15., 1000., Day, Error)
			|| !FKSIWeatherModel::Advance(Climate, I * 15., 15., 0., Night, Error))
		{
			AddError(Error);
			return false;
		}
		Rain += Day.RainMetersPerSecond * 15.;
		Snow += Night.SnowMetersPerSecond * 15.;
	}
	TestTrue(TEXT("sunlight warms without forced temperature"), Day.TemperatureCelsius > Night.TemperatureCelsius + 8.);
	TestTrue(TEXT("cold atmosphere generates snowfall"), Snow > 0.0001);
	TestTrue(TEXT("warm atmosphere generates rain"), Rain > 0.);
	const auto Before = Day;
	TestTrue(TEXT("zero world time"), FKSIWeatherModel::Advance(Climate, 7200., 0., 0., Day, Error));
	TestEqual(TEXT("zero scale retains vapor"), Day.VaporMeters, Before.VaporMeters);
	TestEqual(TEXT("zero scale retains temperature"), Day.TemperatureCelsius, Before.TemperatureCelsius);
	TestFalse(TEXT("unbounded work rejected"), FKSIWeatherModel::Advance(Climate, 7200., 61., 0., Day, Error));
	TestFalse(TEXT("NaN solar input rejected"), FKSIWeatherModel::Advance(Climate, 7200., 15., std::numeric_limits<double>::quiet_NaN(), Day, Error));
	TestEqual(TEXT("invalid input preserves vapor"), Day.VaporMeters, Before.VaporMeters);
	Climate.ThermalResponseSeconds = 0.;
	TestFalse(TEXT("invalid climate rejected"), FKSIWeatherModel::Validate(Climate, Day, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWeatherSurfaceTest, "CCL.Environment.Weather.SurfaceConservationAndResume",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLWeatherSurfaceTest::RunTest(const FString& Parameters)
{
	FString Error;
	FKSISurfaceSimulation S;
	auto Inputs = FKSIEnvironmentInputsCodec::MakeDefault(42);
	FKSICelestialSystem Celestial;
	TestTrue(TEXT("celestial definition"), Celestial.Initialize(Inputs.Celestial, Error));
	TestTrue(TEXT("surface init"), S.Initialize(FGuid::NewGuid(), 0., 0., 0, Error));
	const auto G = WeatherGrid();
	TestTrue(TEXT("weather region"), S.AddRegion(G, Error));
	TArray<uint8> Before, After;
	S.Capture(Before, Error);
	TestFalse(TEXT("natural mode requires celestial context"), S.Advance(WeatherStep(S), Error));
	S.Capture(After, Error);
	TestTrue(TEXT("failure is byte identical"), Before == After);
	for (int32 I = 0; I < 240; ++I)
	{
		if (!S.Advance(WeatherStep(S), Error, &Celestial, &Inputs.Observer))
		{
			AddError(Error);
			return false;
		}
	}
	const auto* Region = S.FindRegion(G.RegionId);
	TestTrue(TEXT("natural precipitation reaches surface"), Region->Ledger.RainCubicMeters + Region->Ledger.SnowCubicMeters > 0.);
	TestTrue(TEXT("water equivalent conserved"), FMath::Abs(Region->BalanceErrorCubicMeters()) < 1.e-8);
	TestEqual(TEXT("sheltered cell stays dry"), Region->Cells[1].WaterCubicMeters + Region->Cells[1].SnowCubicMeters + Region->Cells[1].SoilCubicMeters + Region->Cells[1].IceCubicMeters, 0.);
	TestTrue(TEXT("save weather"), S.Capture(Before, Error));
	FKSISurfaceSimulation Restored;
	TestTrue(TEXT("restore weather"), Restored.Restore(Before, Error));
	for (int32 I = 0; I < 30; ++I)
	{
		TestTrue(TEXT("continuous"), S.Advance(WeatherStep(S), Error, &Celestial, &Inputs.Observer));
		TestTrue(TEXT("resumed"), Restored.Advance(WeatherStep(Restored), Error, &Celestial, &Inputs.Observer));
	}
	S.Capture(Before, Error);
	Restored.Capture(After, Error);
	TestTrue(TEXT("resume deterministic including moisture and wind"), Before == After);
	auto F = S.FindRegion(G.RegionId)->Forcing;
	F.RainMetersPerWorldSecond = 0.00001;
	TestTrue(TEXT("manual override"), S.ChangeForcing(G.RegionId, F, Error));
	TestFalse(TEXT("forced mode is explicit"), S.FindRegion(G.RegionId)->Climate.bEnabled != 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWeatherAtomicTest, "CCL.Environment.Weather.LifeFailureDoesNotConsumeRain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLWeatherAtomicTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLLifeSimulation Life;
	FKSIWorldClock Clock;
	FKSISurfaceSimulation Surface;
	const auto Inputs = FKSIEnvironmentInputsCodec::MakeDefault(42);
	FKSICelestialSystem Celestial;
	TestTrue(TEXT("life init"), Life.Initialize(FCCLLifeSimulation::MerchantScenario(42), Error));
	TestTrue(TEXT("clock init"), Clock.Reset(Life.GetTime(), 60., Error));
	TestTrue(TEXT("surface init"), Surface.Initialize(FGuid::NewGuid(), 0., Life.GetTime(), 0, Error));
	TestTrue(TEXT("region"), Surface.AddRegion(WeatherGrid(), Error));
	TestTrue(TEXT("celestial"), Celestial.Initialize(Inputs.Celestial, Error));
	Clock.QueueGameTime(0.25, false, Error);
	TArray<uint8> Before, After;
	Surface.Capture(Before, Error);
	TestFalse(TEXT("life budget failure"), CCLWorldAdvance::Advance(Clock, Life, 0.25, 15., Error, 0, &Surface, &Celestial, &Inputs.Observer));
	Surface.Capture(After, Error);
	TestTrue(TEXT("weather and surface unchanged"), Before == After);
	TestEqual(TEXT("time still pending"), Clock.GetPendingGameSeconds(), 0.25);
	TestTrue(TEXT("retry"), CCLWorldAdvance::Advance(Clock, Life, 0.25, 15., Error, 256, &Surface, &Celestial, &Inputs.Observer));
	TestEqual(TEXT("single completed step"), Surface.GetStepId(), uint64(1));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWeatherSeasonTest, "CCL.Environment.Weather.AxialTiltChangesClimate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLWeatherSeasonTest::RunTest(const FString& Parameters)
{
	auto Definition = FKSICelestialSystem::MakeDefaultDefinition(42);
	// Compare opposite hemispheres at the same time, with identical regional climates.
	FKSICelestialObserver North, South;
	for (const auto& Body : Definition.Bodies)
	{
		if (Body.Kind == EKSICelestialKind::Planet)
		{
			North.BodyId = South.BodyId = Body.BodyId;
			break;
		}
	}
	North.LatitudeDegrees = 60.;
	South.LatitudeDegrees = -60.;
	FKSICelestialSystem Celestial;
	FString Error;
	TestTrue(TEXT("definition"), Celestial.Initialize(Definition, Error));
	FKSIClimate Climate;
	Climate.bEnabled = 1;
	Climate.MoistureSupplyMetersPerSecond = 0.;
	Climate.ThermalResponseSeconds = 3600.;
	FKSIWeatherState A, B;
	A.VaporMeters = B.VaporMeters = 0.;
	double NorthSun = 0., SouthSun = 0.;
	for (int32 I = 0; I < 1440; ++I)
	{
		FKSICelestialObservation N, S;
		if (!Celestial.Observe(I * 60. + 30., North, N, Error) || !Celestial.Observe(I * 60. + 30., South, S, Error)
			|| !FKSIWeatherModel::Advance(Climate, I * 60., 60., N.TotalHorizontalIrradianceWattsPerM2, A, Error)
			|| !FKSIWeatherModel::Advance(Climate, I * 60., 60., S.TotalHorizontalIrradianceWattsPerM2, B, Error))
		{
			AddError(Error);
			return false;
		}
		NorthSun += N.TotalHorizontalIrradianceWattsPerM2;
		SouthSun += S.TotalHorizontalIrradianceWattsPerM2;
	}
	TestTrue(TEXT("tilt gives distinct daily insolation"), FMath::Abs(NorthSun - SouthSun) > 1000.);
	TestTrue(TEXT("climate follows astronomical input"), (A.TemperatureCelsius - B.TemperatureCelsius) * (NorthSun - SouthSun) > 0.);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWeatherLargeStepTest, "CCL.Environment.Weather.LargeWorldStepIsAtomicAndSubdivided",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLWeatherLargeStepTest::RunTest(const FString& Parameters)
{
	FString Error;
	FKSISurfaceSimulation Large;
	const auto Inputs = FKSIEnvironmentInputsCodec::MakeDefault(42);
	FKSICelestialSystem Celestial;
	TestTrue(TEXT("celestial"), Celestial.Initialize(Inputs.Celestial, Error));
	TestTrue(TEXT("initialize"), Large.Initialize(FGuid::NewGuid(), 0., 0., 0, Error));
	const auto Grid = WeatherGrid();
	TestTrue(TEXT("region"), Large.AddRegion(Grid, Error));
	FKSISurfaceSimulation Small = Large;
	auto Step = WeatherStep(Large, 7200.);
	Step.GameToSeconds = 120.;
	TestTrue(TEXT("large interval supported"), Large.Advance(Step, Error, &Celestial, &Inputs.Observer));
	for (int32 I = 0; I < 120; ++I)
	{
		auto Slice = WeatherStep(Small, 60.);
		Slice.GameToSeconds = Slice.GameFromSeconds + 1.;
		if (!Small.Advance(Slice, Error, &Celestial, &Inputs.Observer))
		{
			AddError(Error);
			return false;
		}
	}
	const auto* A = Large.FindRegion(Grid.RegionId);
	const auto* B = Small.FindRegion(Grid.RegionId);
	TestEqual(TEXT("same weather subintervals"), A->Weather.TemperatureCelsius, B->Weather.TemperatureCelsius);
	TestEqual(TEXT("no duplicated atmospheric consumption"), A->Weather.VaporMeters, B->Weather.VaporMeters);
	TestTrue(TEXT("same deposited volume"), FMath::IsNearlyEqual(A->TotalCubicMeters(), B->TotalCubicMeters(), 1.e-12));
	TArray<uint8> Before, After;
	Large.Capture(Before, Error);
	TestFalse(TEXT("weather work budget rejects oversize request"), Large.Advance(WeatherStep(Large, 250000.), Error, &Celestial, &Inputs.Observer));
	Large.Capture(After, Error);
	TestTrue(TEXT("oversize rejection preserves committed bytes"), Before == After);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWeatherShelterTest, "CCL.Environment.Weather.DynamicRoofOpeningAndRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLWeatherShelterTest::RunTest(const FString& Parameters)
{
	FString Error;
	auto Inputs = FKSIEnvironmentInputsCodec::MakeDefault(42);
	FKSICelestialSystem Celestial;
	TestTrue(TEXT("celestial"), Celestial.Initialize(Inputs.Celestial, Error));
	FKSISurfaceScene Scene;
	FKSISurfacePatch Roof;
	Roof.SurfaceId = FGuid(90, 91, 92, 93);
	Roof.BodyId = Inputs.Observer.BodyId;
	Roof.MaterialId = TEXT("Roof");
	Roof.CenterMeters = FVector(0.5, 0.5, 3.);
	Roof.HalfExtentsMeters = FVector2D(0.49, 0.49);
	Roof.Transmission.Sun = 0.3;
	FKSISurfaceOpening Opening;
	Opening.OpeningId = FGuid(94, 95, 96, 97);
	Opening.SurfaceId = Roof.SurfaceId;
	Opening.HalfExtentsMeters = FVector2D(0.48, 0.48);
	Opening.SpaceA = FGuid(98, 99, 100, 101);
	TestTrue(TEXT("closed roof"), Scene.Replace({Roof}, {Opening}, 1, Error));
	FKSISurfaceSimulation S;
	auto Grid = WeatherGrid();
	Grid.bDynamicShelter = 1;
	Grid.Climate.bEnabled = 0;
	Grid.Forcing.RainMetersPerWorldSecond = 0.0001;
	Grid.Forcing.InfiltrationMetersPerWorldSecond = 0.;
	Grid.Forcing.DrainageMetersPerWorldSecond = 0.;
	TestTrue(TEXT("initialize"), S.Initialize(FGuid::NewGuid(), 0., 0., 0, Error));
	TestTrue(TEXT("grid"), S.AddRegion(Grid, Error));
	TArray<uint8> Before, After;
	S.Capture(Before, Error);
	TestFalse(TEXT("missing provider fails even forced weather"), S.Advance(WeatherStep(S), Error, &Celestial, &Inputs.Observer));
	S.Capture(After, Error);
	TestTrue(TEXT("failure retains all committed bytes"), Before == After);
	TestTrue(TEXT("closed roof advance"), S.Advance(WeatherStep(S), Error, &Celestial, &Inputs.Observer, &Scene));
	const auto* R = S.FindRegion(Grid.RegionId);
	TestEqual(TEXT("covered cell remains dry"), R->Cells[0].WaterCubicMeters, 0.);
	TestTrue(TEXT("outside receives rain despite initial fixed zero"), R->Cells[1].WaterCubicMeters > 0.);
	TestEqual(TEXT("roof is not wind enclosure"), R->Cells[0].WindExposure, 1.);
	Opening.OpenFraction = 1.;
	TestTrue(TEXT("open skylight"), Scene.Replace({Roof}, {Opening}, 2, Error));
	TestTrue(TEXT("opening changes live precipitation"), S.Advance(WeatherStep(S), Error, &Celestial, &Inputs.Observer, &Scene));
	R = S.FindRegion(Grid.RegionId);
	TestTrue(TEXT("opened roof admits water"), R->Cells[0].WaterCubicMeters > 0.);
	TestTrue(TEXT("mass conserved"), FMath::Abs(R->BalanceErrorCubicMeters()) < 1.e-10);
	S.Capture(Before, Error);
	FKSISurfaceSimulation Restored;
	TestTrue(TEXT("restore dynamic shelter state"), Restored.Restore(Before, Error));
	Restored.Capture(After, Error);
	TestTrue(TEXT("roundtrip canonical"), Before == After && Restored.FindRegion(Grid.RegionId)->bDynamicShelter);
	return true;
}
#endif
