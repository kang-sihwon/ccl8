#include "CCLCelestialSystem.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include <limits>

namespace
{
	constexpr double Pi = 3.14159265358979323846;

	FCCLCelestialDefinitionData SimpleSystem()
	{
		FCCLCelestialDefinitionData Result;
		Result.DefinitionId = FGuid(1, 2, 3, 4);
		FCCLCelestialBodyDefinition Star;
		Star.BodyId = TEXT("Star");
		Star.Kind = ECCLCelestialKind::Star;
		Star.RadiusKm = 10.;
		Star.LuminosityWatts = 4. * Pi * 1.e12;
		Result.Bodies.Add(Star);
		FCCLCelestialBodyDefinition World;
		World.BodyId = TEXT("World");
		World.ParentBodyId = Star.BodyId;
		World.SemiMajorAxisKm = 10000.;
		World.OrbitalPeriodSeconds = 400.;
		World.SpinPeriodSeconds = 10.;
		World.SpinPhaseDegrees = 180.;
		Result.Bodies.Add(World);
		return Result;
	}

	FCCLCelestialBodyDefinition SmallMoon()
	{
		FCCLCelestialBodyDefinition Moon;
		Moon.BodyId = TEXT("Moon");
		Moon.ParentBodyId = TEXT("World");
		Moon.Kind = ECCLCelestialKind::Moon;
		Moon.RadiusKm = 0.1;
		Moon.SemiMajorAxisKm = 100.;
		Moon.OrbitalPeriodSeconds = 40.;
		return Moon;
	}

	const FCCLCelestialBodyState* Find(const TArray<FCCLCelestialBodyState>& States, FName Id)
	{
		return States.FindByPredicate([Id](const auto& State) { return State.BodyId == Id; });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLCelestialOrbitTest, "CCL.Environment.Celestial.HierarchyAndOrientation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLCelestialOrbitTest::RunTest(const FString& Parameters)
{
	FCCLCelestialSystem System;
	FCCLCelestialDefinitionData Definition = SimpleSystem();
	Definition.Bodies.Add(SmallMoon());
	FString Error;
	if (!TestTrue(TEXT("valid hierarchy"), System.Initialize(Definition, Error)))
	{
		return false;
	}

	TArray<FCCLCelestialBodyState> States;
	TestTrue(TEXT("epoch"), System.Evaluate(0., States, Error));
	TestTrue(TEXT("planet starts on X axis"), Find(States, TEXT("World"))->PositionKm.Equals(FVector3d(10000., 0., 0.), 1.e-8));
	TestTrue(TEXT("moon includes parent translation"), Find(States, TEXT("Moon"))->PositionKm.Equals(FVector3d(10100., 0., 0.), 1.e-8));
	TestTrue(TEXT("quarter parent orbit"), System.Evaluate(100., States, Error));
	TestTrue(TEXT("planet quarter orbit"), Find(States, TEXT("World"))->PositionKm.Equals(FVector3d(0., 10000., 0.), 1.e-8));
	TestTrue(TEXT("moon relative orbit stays in inertial frame"), Find(States, TEXT("Moon"))->PositionKm.Equals(FVector3d(-100., 10000., 0.), 1.e-8));
	Definition.Bodies.SetNum(2);
	Definition.Bodies[1].InclinationDegrees = 90.;
	Definition.Bodies[1].AscendingNodeDegrees = 90.;
	TestTrue(TEXT("inclined orbit"), System.Initialize(Definition, Error));
	TestTrue(TEXT("ascending node at epoch"), System.Evaluate(0., States, Error));
	TestTrue(TEXT("node turns X to Y"), Find(States, TEXT("World"))->PositionKm.Equals(FVector3d(0., 10000., 0.), 1.e-8));
	TestTrue(TEXT("inclined quarter"), System.Evaluate(100., States, Error));
	TestTrue(TEXT("inclination turns orbital Y to Z"), Find(States, TEXT("World"))->PositionKm.Equals(FVector3d(0., 0., 10000.), 1.e-8));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLCelestialEllipseTest, "CCL.Environment.Celestial.AnalyticalEllipse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLCelestialEllipseTest::RunTest(const FString& Parameters)
{
	FCCLCelestialDefinitionData Definition = SimpleSystem();
	Definition.Bodies[1].SemiMajorAxisKm = 1000.;
	Definition.Bodies[1].Eccentricity = 0.6;
	Definition.Bodies[1].OrbitalPeriodSeconds = 100.;
	FCCLCelestialSystem System;
	FString Error;
	TestTrue(TEXT("elliptical definition"), System.Initialize(Definition, Error));
	TArray<FCCLCelestialBodyState> States;
	TestTrue(TEXT("periapsis"), System.Evaluate(0., States, Error));
	TestTrue(TEXT("known periapsis 400 km"), Find(States, TEXT("World"))->PositionKm.Equals(FVector3d(400., 0., 0.), 1.e-8));
	TestTrue(TEXT("apoapsis"), System.Evaluate(50., States, Error));
	TestTrue(TEXT("known apoapsis 1600 km"), Find(States, TEXT("World"))->PositionKm.Equals(FVector3d(-1600., 0., 0.), 1.e-8));
	// At E=pi/2, Kepler's equation gives M=pi/2-0.6 and coordinates (-600,800).
	TestTrue(TEXT("analytical eccentric anomaly"), System.Evaluate((Pi * 0.5 - 0.6) / (2. * Pi) * 100., States, Error));
	TestTrue(TEXT("nonuniform orbital motion"), Find(States, TEXT("World"))->PositionKm.Equals(FVector3d(-600., 800., 0.), 1.e-8));
	Definition.Bodies[1].Eccentricity = 0.95;
	TestTrue(TEXT("supported high eccentricity"), System.Initialize(Definition, Error));
	TestTrue(TEXT("large time remains finite"), System.Evaluate(1.e12, States, Error));
	TestTrue(TEXT("high eccentricity periapsis"), Find(States, TEXT("World"))->PositionKm.Equals(FVector3d(50., 0., 0.), 1.e-8));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLCelestialSeasonsTest, "CCL.Environment.Celestial.LatitudeAndSeasons",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLCelestialSeasonsTest::RunTest(const FString& Parameters)
{
	FCCLCelestialDefinitionData Definition = SimpleSystem();
	Definition.Bodies[1].SemiMajorAxisKm = 1.e8;
	Definition.Bodies[1].OrbitalPeriodSeconds = 360. * 86400.;
	Definition.Bodies[1].SpinPeriodSeconds = 86400.;
	Definition.Bodies[1].ObliquityDegrees = 30.;
	FCCLCelestialSystem System;
	FString Error;
	TestTrue(TEXT("tilted planet"), System.Initialize(Definition, Error));
	FCCLCelestialObserver Observer;
	Observer.BodyId = TEXT("World");
	Observer.LatitudeDegrees = 45.;
	FCCLCelestialObservation Observation;
	TestTrue(TEXT("northern winter noon"), System.Observe(0., Observer, Observation, Error));
	TestTrue(TEXT("winter noon elevation 15 degrees"), FMath::IsNearlyEqual(Observation.Stars[0].ElevationDegrees, 15., 1.e-5));
	TestTrue(TEXT("negative winter declination"), FMath::IsNearlyEqual(Observation.Stars[0].DeclinationDegrees, -30., 1.e-5));
	Observer.LongitudeDegrees = 180.;
	TestTrue(TEXT("northern summer noon"), System.Observe(180. * 86400., Observer, Observation, Error));
	TestTrue(TEXT("summer noon elevation 75 degrees"), FMath::IsNearlyEqual(Observation.Stars[0].ElevationDegrees, 75., 1.e-5));
	Observer.LongitudeDegrees = 90.;
	TestTrue(TEXT("equinox noon"), System.Observe(90. * 86400., Observer, Observation, Error));
	TestTrue(TEXT("equinox noon elevation 45 degrees"), FMath::IsNearlyEqual(Observation.Stars[0].ElevationDegrees, 45., 1.e-5));
	Observer.LongitudeDegrees = 0.;
	Observer.LatitudeDegrees = -45.;
	TestTrue(TEXT("southern summer at northern winter"), System.Observe(0., Observer, Observation, Error));
	TestTrue(TEXT("southern seasons reversed"), FMath::IsNearlyEqual(Observation.Stars[0].ElevationDegrees, 75., 1.e-5));
	Observer.LatitudeDegrees = 90.;
	for (int32 Quarter = 0; Quarter < 4; ++Quarter)
	{
		TestTrue(TEXT("polar winter throughout rotation"), System.Observe(Quarter * 21600., Observer, Observation, Error));
		TestTrue(TEXT("polar night"), Observation.Stars[0].ElevationDegrees < -29.);
		TestEqual(TEXT("no direct solar input at night"), Observation.TotalHorizontalIrradianceWattsPerM2, 0.);
		TestTrue(TEXT("polar summer throughout rotation"), System.Observe(180. * 86400. + Quarter * 21600., Observer, Observation, Error));
		TestTrue(TEXT("polar day"), Observation.Stars[0].ElevationDegrees > 29.);
	}

	Definition.Bodies[1].ObliquityDegrees = 0.;
	TestTrue(TEXT("untilted planet"), System.Initialize(Definition, Error));
	Observer.LatitudeDegrees = 0.;
	TestTrue(TEXT("no tilt seasonal declination"), System.Observe(123. * 86400., Observer, Observation, Error));
	TestTrue(TEXT("zero declination"), FMath::Abs(Observation.Stars[0].DeclinationDegrees) < 1.e-5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLCelestialSolarTimeTest, "CCL.Environment.Celestial.SolarAndSiderealTime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLCelestialSolarTimeTest::RunTest(const FString& Parameters)
{
	FCCLCelestialDefinitionData Definition = SimpleSystem();
	Definition.Bodies[1].SemiMajorAxisKm = 1.e8;
	Definition.Bodies[1].OrbitalPeriodSeconds = 100.;
	Definition.Bodies[1].SpinPeriodSeconds = 10.;
	FCCLCelestialSystem System;
	FString Error;
	TestTrue(TEXT("clock definition"), System.Initialize(Definition, Error));
	FCCLCelestialObserver Observer;
	Observer.BodyId = TEXT("World");
	FCCLCelestialObservation Observation;
	TestTrue(TEXT("epoch noon"), System.Observe(0., Observer, Observation, Error));
	TestTrue(TEXT("noon is 12 solar hours"), FMath::IsNearlyEqual(Observation.Stars[0].SolarHours, 12., 1.e-6));
	TestTrue(TEXT("one sidereal rotation"), System.Observe(10., Observer, Observation, Error));
	TestTrue(TEXT("one spin is not one solar day"), FMath::IsNearlyEqual(Observation.Stars[0].SolarHours, 9.6, 1.e-6));
	TestTrue(TEXT("synodic day"), System.Observe(1. / (1. / 10. - 1. / 100.), Observer, Observation, Error));
	TestTrue(TEXT("solar noon returns after synodic period"), FMath::IsNearlyEqual(Observation.Stars[0].SolarHours, 12., 1.e-6));
	Observer.LongitudeDegrees = 90.;
	TestTrue(TEXT("east longitude"), System.Observe(0., Observer, Observation, Error));
	TestTrue(TEXT("east longitude advances local hours"), FMath::IsNearlyEqual(Observation.Stars[0].SolarHours, 18., 1.e-6));
	Definition.Bodies[1].SpinPeriodSeconds = -10.;
	TestTrue(TEXT("retrograde spin"), System.Initialize(Definition, Error));
	Observer.LongitudeDegrees = 0.;
	TestTrue(TEXT("retrograde quarter rotation"), System.Observe(2.5, Observer, Observation, Error));
	TestTrue(TEXT("retrograde solar hours decrease"), FMath::IsNearlyEqual(Observation.Stars[0].SolarHours, 5.4, 1.e-6));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLCelestialReproducibilityTest, "CCL.Environment.Celestial.SeedOrderAndReevaluation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLCelestialReproducibilityTest::RunTest(const FString& Parameters)
{
	FCCLCelestialDefinitionData Definition = FCCLCelestialSystem::MakeDefaultDefinition(42);
	FCCLCelestialSystem First;
	FCCLCelestialSystem Second;
	FString Error;
	TestTrue(TEXT("default definition"), First.Initialize(Definition, Error));
	Swap(Definition.Bodies[0], Definition.Bodies[3]);
	TestTrue(TEXT("unordered input"), Second.Initialize(Definition, Error));
	TArray<FCCLCelestialBodyState> A;
	TArray<FCCLCelestialBodyState> B;
	TestTrue(TEXT("direct evaluation"), First.Evaluate(123456789., A, Error));
	TestTrue(TEXT("distant intermediate time"), Second.Evaluate(1.e12, B, Error));
	TestTrue(TEXT("rewind reevaluation"), Second.Evaluate(123456789., B, Error));
	for (int32 Index = 0; Index < A.Num(); ++Index)
	{
		TestEqual(TEXT("canonical stable order"), A[Index].BodyId, B[Index].BodyId);
		TestTrue(TEXT("same time independent of history"), A[Index].PositionKm == B[Index].PositionKm);
		TestEqual(TEXT("spin reproducible"), A[Index].SpinRadians, B[Index].SpinRadians);
	}

	TestTrue(TEXT("different seed"), Second.Initialize(FCCLCelestialSystem::MakeDefaultDefinition(43), Error));
	TestTrue(TEXT("different seed evaluation"), Second.Evaluate(123456789., B, Error));
	TestFalse(TEXT("seed changes moon phase"), Find(A, TEXT("Moon"))->PositionKm.Equals(Find(B, TEXT("Moon"))->PositionKm, 1.));
	TestTrue(TEXT("seed does not move main planet"), Find(A, TEXT("World"))->PositionKm == Find(B, TEXT("World"))->PositionKm);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLCelestialInvalidTest, "CCL.Environment.Celestial.InvalidInputIsAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLCelestialInvalidTest::RunTest(const FString& Parameters)
{
	FCCLCelestialSystem System;
	const FCCLCelestialDefinitionData Valid = SimpleSystem();
	FString Error;
	TestTrue(TEXT("valid starting definition"), System.Initialize(Valid, Error));
	TArray<FCCLCelestialDefinitionData> Invalid;
	FCCLCelestialDefinitionData Bad = Valid;
	Bad.Bodies.Add(Valid.Bodies[1]);
	Invalid.Add(Bad);
	Bad = Valid;
	Bad.Bodies[1].ParentBodyId = TEXT("Missing");
	Invalid.Add(Bad);
	Bad = Valid;
	Bad.Bodies.Add(SmallMoon());
	Bad.Bodies[1].ParentBodyId = TEXT("Moon");
	Invalid.Add(Bad);
	Bad = Valid;
	Bad.Bodies[1].SpinPeriodSeconds = 0.;
	Invalid.Add(Bad);
	Bad = Valid;
	Bad.Bodies[1].Eccentricity = 1.;
	Invalid.Add(Bad);
	Bad = Valid;
	Bad.Bodies[1].RadiusKm = std::numeric_limits<double>::quiet_NaN();
	Invalid.Add(Bad);
	Bad = Valid;
	Bad.Bodies[0].LuminosityWatts = -1.;
	Invalid.Add(Bad);
	Bad = Valid;
	Bad.Bodies[1].SemiMajorAxisKm = 5.;
	Invalid.Add(Bad);
	for (const auto& Candidate : Invalid)
	{
		TestFalse(TEXT("invalid candidate rejected"), System.Initialize(Candidate, Error));
		TestFalse(TEXT("failure explains reason"), Error.IsEmpty());
		TestEqual(TEXT("previous definition remains intact"), System.GetDefinition().Bodies.Num(), 2);
	}

	TArray<FCCLCelestialBodyState> States;
	TestTrue(TEXT("previous definition remains evaluable"), System.Evaluate(0., States, Error));
	const FVector3d Position = States[1].PositionKm;
	TestFalse(TEXT("negative world time rejected"), System.Evaluate(-1., States, Error));
	TestFalse(TEXT("nonfinite world time rejected"), System.Evaluate(std::numeric_limits<double>::infinity(), States, Error));
	TestTrue(TEXT("failed evaluation preserves output"), States[1].PositionKm == Position);
	FCCLCelestialObserver Observer;
	Observer.BodyId = TEXT("World");
	FCCLCelestialObservation Observation;
	TestTrue(TEXT("valid observer"), System.Observe(0., Observer, Observation, Error));
	Observer.LatitudeDegrees = 91.;
	TestFalse(TEXT("invalid latitude"), System.Observe(1., Observer, Observation, Error));
	TestEqual(TEXT("failed observation preserves time"), Observation.WorldSeconds, 0.);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLCelestialRadiationTest, "CCL.Environment.Celestial.RadiationOccultationAndMoonPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLCelestialRadiationTest::RunTest(const FString& Parameters)
{
	FCCLCelestialDefinitionData Definition = SimpleSystem();
	FCCLCelestialSystem System;
	FString Error;
	TestTrue(TEXT("radiation fixture"), System.Initialize(Definition, Error));
	FCCLCelestialObserver Observer;
	Observer.BodyId = TEXT("World");
	FCCLCelestialObservation Observation;
	TestTrue(TEXT("near side noon"), System.Observe(0., Observer, Observation, Error));
	const double NearRadiation = Observation.Stars[0].NormalIrradianceWattsPerM2;
	TestTrue(TEXT("analytical irradiance uses meters squared"), FMath::IsNearlyEqual(NearRadiation, 1.e12 / FMath::Square(9999. * 1000.), 1.e-12));
	Definition.Bodies[1].SemiMajorAxisKm = 19999.;
	TestTrue(TEXT("double observer distance"), System.Initialize(Definition, Error));
	TestTrue(TEXT("farther noon"), System.Observe(0., Observer, Observation, Error));
	TestTrue(TEXT("inverse square law"), FMath::IsNearlyEqual(Observation.Stars[0].NormalIrradianceWattsPerM2, NearRadiation * 0.25, 1.e-12));
	Definition = SimpleSystem();
	Definition.Bodies.Add(SmallMoon());
	TestTrue(TEXT("full moon definition"), System.Initialize(Definition, Error));
	TestTrue(TEXT("full moon observation"), System.Observe(0., Observer, Observation, Error));
	TestFalse(TEXT("moon behind observer cannot occult star"), Observation.Stars[0].bOcculted != 0);
	TestTrue(TEXT("full moon geometric phase"), FMath::IsNearlyEqual(Observation.SkyBodies[0].IlluminatedFraction, 1., 1.e-9));
	Definition.Bodies[2].MeanAnomalyDegrees = 180.;
	TestTrue(TEXT("new moon definition"), System.Initialize(Definition, Error));
	TestTrue(TEXT("new moon observation"), System.Observe(0., Observer, Observation, Error));
	TestTrue(TEXT("aligned moon occludes point source"), Observation.Stars[0].bOcculted != 0);
	TestEqual(TEXT("occultation removes direct input"), Observation.TotalHorizontalIrradianceWattsPerM2, 0.);
	TestTrue(TEXT("new moon geometric phase"), FMath::IsNearlyEqual(Observation.SkyBodies[0].IlluminatedFraction, 0., 1.e-9));
	Definition.Bodies[2].MeanAnomalyDegrees = 90.;
	TestTrue(TEXT("off-axis moon definition"), System.Initialize(Definition, Error));
	TestTrue(TEXT("off-axis observation"), System.Observe(0., Observer, Observation, Error));
	TestFalse(TEXT("off-axis moon does not occult"), Observation.Stars[0].bOcculted != 0);
	FCCLCelestialBodyDefinition OtherStar = Definition.Bodies[0];
	OtherStar.BodyId = TEXT("OtherStar");
	OtherStar.ParentBodyId = TEXT("Star");
	OtherStar.SemiMajorAxisKm = 30000.;
	OtherStar.MeanAnomalyDegrees = 180.;
	OtherStar.LuminosityWatts *= 100.;
	Definition.Bodies.Add(OtherStar);
	TestTrue(TEXT("hierarchical binary fixture"), System.Initialize(Definition, Error));
	TestTrue(TEXT("multiple sources observed"), System.Observe(0., Observer, Observation, Error));
	TestEqual(TEXT("both stars contribute"), Observation.Stars.Num(), 2);
	TestEqual(TEXT("dominant source chosen by irradiance"), Observation.DominantStarId, FName(TEXT("OtherStar")));
	TestTrue(TEXT("sum includes both stars"), FMath::IsNearlyEqual(Observation.TotalHorizontalIrradianceWattsPerM2,
		Observation.Stars[0].HorizontalIrradianceWattsPerM2 + Observation.Stars[1].HorizontalIrradianceWattsPerM2, 1.e-12));
	return true;
}
#endif
