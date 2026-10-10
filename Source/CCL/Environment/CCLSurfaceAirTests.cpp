#include "CCLSurfaceSimulation.h"

#include "CCLCelestialSystem.h"
#include "CCLWorldSnapshot.h"
#include "Agents/CCLLifeSimulation.h"
#include "CCLSurfaceQuery.h"
#include "Misc/AutomationTest.h"
#include "Serialization/MemoryWriter.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	const FGuid RegionId(901, 902, 903, 904);
	const FGuid SpaceId(911, 912, 913, 914);

	FCCLSurfaceGrid CoupledGrid(int32 Count = 1, bool bFuel = true)
	{
		FCCLSurfaceGrid G;
		G.RegionId = RegionId;
		G.Size = FIntPoint(Count, 1);
		G.SpacingMeters = 1.;
		G.Cells.SetNum(Count);
		G.Forcing.FlowConductance = 0.;
		G.Forcing.InfiltrationMetersPerWorldSecond = 0.;
		G.Forcing.DrainageMetersPerWorldSecond = 0.;
		G.Forcing.TemperatureCelsius = -40.;
		G.Weather.WindMetersPerSecond = 0.;
		if (bFuel)
		{
			G.Fire.Cells.SetNum(Count);
			FString Error;
			for (auto& C : G.Fire.Cells)
			{
				FCCLCombustionModel::Initialize(G.Fire.Material, 2., 20., C, Error);
			}
		}
		return G;
	}

	FCCLAirSpace CoupledRoom(double Temperature = 20., double Volume = 64.)
	{
		FCCLAirSpace S;
		S.SpaceId = SpaceId;
		S.MinimumMeters = FVector(0., 0., 0.);
		S.MaximumMeters = FVector(4., 4., 4.);
		S.VolumeCubicMeters = Volume;
		FString Error;
		S.Initialize(Temperature, 0.004, Error);
		return S;
	}

	FCCLWorldStep CoupledStep(const FCCLSurfaceSimulation& S, double Game, double World = 0.)
	{
		FCCLWorldStep R;
		R.StepId = S.GetStepId() + 1;
		R.GameFromSeconds = S.GetGameSeconds();
		R.GameToSeconds = R.GameFromSeconds + Game;
		R.WorldFromSeconds = S.GetWorldSeconds();
		R.WorldToSeconds = R.WorldFromSeconds + World;
		return R;
	}

	bool CoupledAdvance(FCCLSurfaceSimulation& S, double Game, FString& Error,
		const TArray<FCCLSurfaceOpening>& Openings = {}, double World = 0.)
	{
		return S.Advance(CoupledStep(S, Game, World), Error, nullptr, nullptr, nullptr, &Openings);
	}

	FCCLSurfaceOpening CoupledDoor(double Fraction)
	{
		FCCLSurfaceOpening D;
		D.OpeningId = FGuid(921, 922, 923, 924);
		D.SurfaceId = FGuid(931, 932, 933, 934);
		D.SpaceA = SpaceId;
		D.OpenFraction = Fraction;
		return D;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLFireAirConservationTest, "CCL.Environment.Air.FireCouplingConservesHeatAndSmoke",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLFireAirConservationTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLSurfaceSimulation S;
	TestTrue(TEXT("world"), S.Initialize(FGuid::NewGuid(), 0., 0., 0, Error));
	const auto G = CoupledGrid();
	const auto Room = CoupledRoom();
	TestTrue(TEXT("region"), S.AddRegion(G, Error));
	TestTrue(TEXT("room"), S.AddAirSpace(Room, Error));
	TestTrue(TEXT("ignition"), S.Ignite(RegionId, 0, 600000., Error));
	TestTrue(TEXT("coupled advance"), CoupledAdvance(S, 30., Error));
	const auto& Fire = S.FindRegion(RegionId)->Fire;
	const auto& Air = *S.GetAirSimulation().FindSpace(SpaceId);
	TestTrue(TEXT("fire heats closed room"), Air.TemperatureCelsius() > 20.);
	TestTrue(TEXT("room receives actual emitted smoke"), FMath::IsNearlyEqual(Air.Contents.SmokeKg, Fire.EmittedSmokeKg, 1.e-10));
	const double ExpectedHeat = Room.Initial.HeatJoules + G.Fire.Cells[0].InitialHeatJoules + 600000.
		+ Fire.Cells[0].ReleasedHeatJoules;
	TestTrue(TEXT("combined air and fuel heat balances"), FMath::IsNearlyEqual(Air.Contents.HeatJoules + Fire.Cells[0].HeatJoules, ExpectedHeat, 1.e-5));
	TestTrue(TEXT("all water heat smoke ledgers validate"), S.Validate(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLAirSurfacePhaseTest, "CCL.Environment.Air.FiniteHeatMeltsDriesAndFreezesSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLAirSurfacePhaseTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLSurfaceSimulation S;
	S.Initialize(FGuid::NewGuid(), 0., 0., 0, Error);
	auto G = CoupledGrid(2, false);
	G.Cells[0].SnowCubicMeters = 0.01;
	G.Cells[0].SnowVolumeCubicMeters = 0.1;
	G.Cells[1].SoilCubicMeters = 0.01;
	G.Ledger.InitialCubicMeters = G.TotalCubicMeters();
	const auto Room = CoupledRoom();
	TestTrue(TEXT("region"), S.AddRegion(G, Error));
	TestTrue(TEXT("room"), S.AddAirSpace(Room, Error));
	TestTrue(TEXT("indoor phase uses finite room heat despite cold regional forcing"), CoupledAdvance(S, 10., Error, {}, 600.));
	const auto& R = *S.FindRegion(RegionId);
	const auto& A = *S.GetAirSimulation().FindSpace(SpaceId);
	TestTrue(TEXT("snow melted"), R.Cells[0].SnowCubicMeters < G.Cells[0].SnowCubicMeters);
	TestTrue(TEXT("soil dried"), R.Cells[1].SoilCubicMeters < G.Cells[1].SoilCubicMeters);
	TestTrue(TEXT("vapor transferred once"), FMath::IsNearlyEqual(A.Contents.VaporKg - Room.Initial.VaporKg,
		R.Ledger.EvaporatedCubicMeters * 1000., 1.e-10));
	TestTrue(TEXT("room paid phase energy"), FMath::IsNearlyEqual(A.Contents.HeatJoules + R.AirHeatToSurfaceJoules, Room.Initial.HeatJoules, 1.e-5));
	TestTrue(TEXT("water preserved"), FMath::Abs(R.BalanceErrorCubicMeters()) < 1.e-10);

	FCCLSurfaceSimulation Cold;
	Cold.Initialize(FGuid::NewGuid(), 0., 0., 0, Error);
	auto Wet = CoupledGrid(1, false);
	Wet.Cells[0].WaterCubicMeters = 0.1;
	Wet.Ledger.InitialCubicMeters = 0.1;
	Wet.Forcing.TemperatureCelsius = 60.;
	const auto ColdRoom = CoupledRoom(-20.);
	TestTrue(TEXT("wet region"), Cold.AddRegion(Wet, Error));
	TestTrue(TEXT("cold room"), Cold.AddAirSpace(ColdRoom, Error));
	TestTrue(TEXT("indoor freezing"), CoupledAdvance(Cold, 10., Error, {}, 600.));
	const auto& Frozen = *Cold.FindRegion(RegionId);
	TestTrue(TEXT("ice forms in cold room despite warm outside"), Frozen.Cells[0].IceCubicMeters > 0.);
	TestTrue(TEXT("freezing releases heat to air"), FMath::IsNearlyEqual(Cold.GetAirSimulation().Total().HeatJoules,
		ColdRoom.Initial.HeatJoules + Frozen.SurfaceHeatToAirJoules, 1.e-5));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLAirFuelBudgetTest, "CCL.Environment.Air.ColdFuelCannotOverdrawRoomHeat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLAirFuelBudgetTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLSurfaceSimulation S;
	S.Initialize(FGuid::NewGuid(), 0., 0., 0, Error);
	auto G = CoupledGrid(4);
	G.Fire.Material.CoolingWattsPerKelvin = 1000000.;
	G.Fire.Material.SurfaceWattsPerKelvin = 0.;
	G.Fire.Material.BurnKgPerGameSecond = 0.;
	for (auto& Cell : G.Fire.Cells)
	{
		TestTrue(TEXT("very cold fuel"), FCCLCombustionModel::Initialize(G.Fire.Material, 1., -100., Cell, Error));
	}
	const auto Room = CoupledRoom(0., 0.1);
	TestTrue(TEXT("region"), S.AddRegion(G, Error));
	TestTrue(TEXT("tiny room"), S.AddAirSpace(Room, Error));
	TestTrue(TEXT("limited heat transfer succeeds"), CoupledAdvance(S, 0.25, Error));
	TestTrue(TEXT("fuel can withdraw only reserved half above minimum"), FMath::IsNearlyEqual(
		S.GetAirSimulation().FindSpace(SpaceId)->TemperatureCelsius(), -50., 1.e-8));
	double Received = 0.;
	for (const auto& Cell : S.FindRegion(RegionId)->Fire.Cells)
	{
		Received += Cell.AddedHeatJoules;
	}
	TestTrue(TEXT("every joule gained by fuel was removed from room"), FMath::IsNearlyEqual(Received + S.GetAirSimulation().Total().HeatJoules,
		Room.Initial.HeatJoules, 1.e-8));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLAirCommonStepTest, "CCL.Environment.Air.MultipleRegionsAdvanceVentilationOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLAirCommonStepTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLSurfaceSimulation S;
	FCCLAirSimulation Reference;
	S.Initialize(FGuid::NewGuid(), 0., 0., 0, Error);
	TestTrue(TEXT("region one"), S.AddRegion(CoupledGrid(1, false), Error));
	auto Other = CoupledGrid(1, false);
	Other.RegionId = FGuid(941, 942, 943, 944);
	Other.OriginMeters = FVector(10., 0., 0.);
	TestTrue(TEXT("region two"), S.AddRegion(Other, Error));
	auto Room = CoupledRoom(80.);
	Room.Initial.SmokeKg = Room.Contents.SmokeKg = 1.;
	TestTrue(TEXT("room"), S.AddAirSpace(Room, Error));
	TestTrue(TEXT("reference room"), Reference.AddSpace(Room, Error));
	const TArray<FCCLSurfaceOpening> Doors = {CoupledDoor(1.)};
	TMap<FGuid, FCCLAirExterior> Exteriors;
	Exteriors.Add(SpaceId, {});
	TestTrue(TEXT("reference once"), Reference.Advance(2., Doors, Exteriors, Error));
	TestTrue(TEXT("coupled regions"), CoupledAdvance(S, 2., Error, Doors));
	TestEqual(TEXT("region count does not multiply ventilation time"), S.GetAirSimulation().Total().SmokeKg, Reference.Total().SmokeKg);
	TestEqual(TEXT("region count does not multiply cooling"), S.GetAirSimulation().Total().HeatJoules, Reference.Total().HeatJoules);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLAirWorldResumeTest, "CCL.Environment.Air.CoupledSaveResumeAndFailedStep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLAirWorldResumeTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLSurfaceSimulation S, Loaded;
	S.Initialize(FGuid::NewGuid(), 0., 0., 0, Error);
	TestTrue(TEXT("region"), S.AddRegion(CoupledGrid(), Error));
	TestTrue(TEXT("room"), S.AddAirSpace(CoupledRoom(), Error));
	TestTrue(TEXT("ignite"), S.Ignite(RegionId, 0, 600000., Error));
	TestTrue(TEXT("advance"), CoupledAdvance(S, 2., Error));
	TArray<uint8> Bytes, A, B;
	TestTrue(TEXT("capture combined snapshot"), S.Capture(Bytes, Error));
	TestTrue(TEXT("restore combined snapshot"), Loaded.Restore(Bytes, Error));
	TestTrue(TEXT("advance original"), CoupledAdvance(S, 3., Error, {CoupledDoor(0.5)}));
	TestTrue(TEXT("advance loaded"), CoupledAdvance(Loaded, 3., Error, {CoupledDoor(0.5)}));
	S.Capture(A, Error);
	Loaded.Capture(B, Error);
	TestTrue(TEXT("all air fire water and clock state resumes identically"), A == B);
	TestFalse(TEXT("missing opening context refused"), S.Advance(CoupledStep(S, 1.), Error));
	auto Broken = CoupledDoor(1.);
	Broken.SpaceB = FGuid(99, 98, 97, 96);
	TestFalse(TEXT("failure after combustion calculation"), CoupledAdvance(S, 1., Error, {Broken}));
	S.Capture(B, Error);
	TestTrue(TEXT("failed ventilation retains fire water air and clock"), A == B);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLFireWallTest, "CCL.Environment.Fire.WallAndDoorBlockDirectHeat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLFireWallTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLSurfaceSimulation S;
	S.Initialize(FGuid::NewGuid(), 0., 0., 0, Error);
	auto G = CoupledGrid(2);
	G.Fire.Material.CoolingWattsPerKelvin = 0.;
	G.Fire.Material.SurfaceWattsPerKelvin = 0.;
	G.Fire.Material.BurnKgPerGameSecond = 0.;
	TestTrue(TEXT("region"), S.AddRegion(G, Error));
	TestTrue(TEXT("heat"), S.Ignite(RegionId, 0, 600000., Error));
	FCCLSurfacePatch Wall;
	Wall.SurfaceId = CoupledDoor(0.).SurfaceId;
	Wall.BodyId = TEXT("TestBody");
	Wall.MaterialId = TEXT("Stone");
	Wall.CenterMeters = FVector(1., 0.5, 1.);
	Wall.Normal = FVector::ForwardVector;
	Wall.TangentU = FVector::RightVector;
	Wall.TangentV = FVector::UpVector;
	Wall.HalfExtentsMeters = FVector2D(0.5, 1.);
	FCCLSurfaceScene Scene;
	auto Door = CoupledDoor(0.);
	TestTrue(TEXT("closed wall"), Scene.Replace({Wall}, {Door}, 1, Error));
	FCCLCelestialObserver Observer;
	Observer.BodyId = Wall.BodyId;
	TestTrue(TEXT("closed step"), S.Advance(CoupledStep(S, 0.25), Error, nullptr, &Observer, &Scene));
	TestEqual(TEXT("opaque wall prevents heat"), S.FindRegion(RegionId)->Fire.Cells[1].AddedHeatJoules, 0.);
	Door.OpenFraction = 1.;
	TestTrue(TEXT("opened wall"), Scene.Replace({Wall}, {Door}, 2, Error));
	TestTrue(TEXT("open step"), S.Advance(CoupledStep(S, 0.25), Error, nullptr, &Observer, &Scene));
	TestTrue(TEXT("actual aperture admits heat"), S.FindRegion(RegionId)->Fire.Cells[1].AddedHeatJoules > 0.);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLAirWorldGraphTest, "CCL.Environment.Air.WorldSnapshotRejectsDanglingSpaceConnection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLAirWorldGraphTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLWorldClock Clock;
	FCCLLifeSimulation Life;
	TestTrue(TEXT("life"), Life.Initialize(FCCLLifeSimulation::MerchantScenario(42), Error));
	FCCLWorldIdentity Identity;
	Identity.WorldId = FGuid::NewGuid();
	FCCLSurfaceSimulation Surface;
	Surface.Initialize(Identity.WorldId, 0., 0., 0, Error);
	TestTrue(TEXT("room"), Surface.AddAirSpace(CoupledRoom(), Error));
	auto Inputs = FCCLEnvironmentInputsCodec::MakeDefault(42);
	FCCLSurfacePatch Wall;
	Wall.SurfaceId = CoupledDoor(0.).SurfaceId;
	Wall.BodyId = Inputs.Observer.BodyId;
	Wall.MaterialId = TEXT("Stone");
	Inputs.Surfaces.Add(Wall);
	Inputs.Openings.Add(CoupledDoor(0.));
	FCCLWorldSnapshot Saved;
	TestTrue(TEXT("valid world generation"), FCCLWorldSnapshotCodec::Capture(Identity, Clock, Life, Saved, Error, &Inputs, &Surface));
	Saved.Environment.Openings.Last().SpaceB = FGuid(77, 78, 79, 80);
	TestFalse(TEXT("world rejects unknown indoor endpoint"), FCCLWorldSnapshotCodec::Validate(Saved, Error));
	TArray<uint8> Before, After;
	Surface.Capture(Before, Error);
	Clock.QueueGameTime(1., false, Error);
	TestFalse(TEXT("restore rejects before publishing"), FCCLWorldSnapshotCodec::Restore(Saved, Identity.Domain, Clock, Life, Error, &Surface));
	Surface.Capture(After, Error);
	TestTrue(TEXT("surface and air retained"), Before == After);
	TestEqual(TEXT("clock pending input retained"), Clock.GetPendingGameSeconds(), 1.);
	return true;
}
#endif
