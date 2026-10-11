#include "CCLSurfaceQuery.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CCLSurfaceSimulation.h"
#include "CCLTerrainSurface.h"
#include "Misc/AutomationTest.h"

namespace
{
	FCCLSurfacePatch VentWall()
	{
		FCCLSurfacePatch P;
		P.SurfaceId = FGuid(1101, 1102, 1103, 1104);
		P.BodyId = TEXT("World");
		P.MaterialId = TEXT("Stone");
		P.Normal = FVector::ForwardVector;
		P.TangentU = FVector::RightVector;
		P.TangentV = FVector::UpVector;
		P.HalfExtentsMeters = FVector2D(2., 2.);
		return P;
	}

	FCCLSurfaceOpening VentDoor()
	{
		FCCLSurfaceOpening O;
		O.OpeningId = FGuid(1111, 1112, 1113, 1114);
		O.SurfaceId = VentWall().SurfaceId;
		O.SpaceA = FGuid(1121, 1122, 1123, 1124);
		O.HalfExtentsMeters = FVector2D(1., 1.);
		O.OpenFraction = 1.;
		return O;
	}

	FCCLSurfacePatch VentBlocker()
	{
		auto P = VentWall();
		P.SurfaceId = FGuid(1131, 1132, 1133, 1134);
		P.CenterMeters.X = 0.25;
		return P;
	}

	TSharedRef<FCCLTerrainSurface> VentTerrain(const FCCLTerrainStore& Store, const FTransform& Transform, FString& Error)
	{
		TArray<TSharedRef<const FCCLTerrainMesh>> Meshes;
		for (int32 Z = -1; Z < 1; ++Z)
		{
			for (int32 Y = -1; Y < 1; ++Y)
			{
				for (int32 X = -1; X < 1; ++X)
				{
					auto Mesh = MakeShared<FCCLTerrainMesh>();
					if (!FCCLTerrainMesher::BuildChunk(Store.GetSnapshot(), FIntVector(X, Y, Z), Store.GetEpoch(), *Mesh, Error))
					{
						return MakeShared<FCCLTerrainSurface>(Store.GetSnapshot(), Store.GetEpoch(), Transform, MoveTemp(Meshes));
					}

					Meshes.Add(Mesh);
				}
			}
		}

		return MakeShared<FCCLTerrainSurface>(Store.GetSnapshot(), Store.GetEpoch(), Transform, MoveTemp(Meshes));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLVentApertureTest, "CCL.Environment.Air.GeometryApertureAndTransmission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLVentApertureTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLSurfaceScene Scene;
	auto Door = VentDoor();
	auto Blocker = VentBlocker();
	double Fraction = -1.;
	TestTrue(TEXT("open scene"), Scene.Replace({VentWall()}, {Door}, 1, Error));
	TestTrue(TEXT("clear opening queried"), Scene.QueryOpeningFraction(Door, Fraction, Error));
	TestEqual(TEXT("full aperture"), Fraction, 1.);
	TestTrue(TEXT("opaque blocker"), Scene.Replace({VentWall(), Blocker}, {Door}, 2, Error));
	TestTrue(TEXT("blocked queried"), Scene.QueryOpeningFraction(Door, Fraction, Error));
	TestEqual(TEXT("blocked aperture"), Fraction, 0.);
	Blocker.HalfExtentsMeters.X = 0.5;
	Blocker.CenterMeters.Y = -0.5;
	TestTrue(TEXT("half blocked"), Scene.Replace({VentWall(), Blocker}, {Door}, 3, Error));
	TestTrue(TEXT("half queried"), Scene.QueryOpeningFraction(Door, Fraction, Error));
	TestEqual(TEXT("half area remains"), Fraction, 0.5);
	Door.OpenFraction = 0.5;
	Scene.Replace({VentWall(), Blocker}, {Door}, 4, Error);
	TestTrue(TEXT("actual low-U opening queried"), Scene.QueryOpeningFraction(Door, Fraction, Error));
	TestEqual(TEXT("opened half coincides with blocker"), Fraction, 0.);
	Blocker.Transmission.Wind = 0.25;
	Scene.Replace({VentWall(), Blocker}, {Door}, 5, Error);
	TestTrue(TEXT("porous blocker queried"), Scene.QueryOpeningFraction(Door, Fraction, Error));
	TestEqual(TEXT("porosity times opened area"), Fraction, 0.125);
	Door.OpenFraction = 0.;
	Scene.Replace({VentWall(), Blocker}, {Door}, 6, Error);
	TestTrue(TEXT("closed queried"), Scene.QueryOpeningFraction(Door, Fraction, Error));
	TestEqual(TEXT("closed door"), Fraction, 0.);
	Door.OpenFraction = 1.;
	Fraction = 42.;
	TestFalse(TEXT("stale opening rejected"), Scene.QueryOpeningFraction(Door, Fraction, Error));
	TestEqual(TEXT("failure preserves output"), Fraction, 42.);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLVentBuriedTest, "CCL.Environment.Air.BuriedApertureAndCommittedExcavation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLVentBuriedTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLTerrainDefinition D;
	D.WorldId = FGuid(1141, 1142, 1143, 1144);
	D.RegionId = FGuid(1151, 1152, 1153, 1154);
	D.DefinitionId = FGuid(1161, 1162, 1163, 1164);
	D.MinimumCell = FIntVector(-16, -16, -20);
	D.MaximumCell = FIntVector(16, 16, 8);
	FCCLTerrainStore Store;
	TestTrue(TEXT("terrain"), Store.Initialize(D, Error));
	const FTransform Transform(FRotator::ZeroRotator, FVector(1000., 2000., 3000.));
	auto Wall = VentWall();
	Wall.CenterMeters = FVector(10., 20., 26.);
	const auto Door = VentDoor();
	FCCLSurfaceScene Scene;
	TestTrue(TEXT("buried scene"), Scene.Replace({Wall}, {Door}, 1, Error));
	const auto Original = VentTerrain(Store, Transform, Error);
	TestTrue(TEXT("original mesh built"), Error.IsEmpty());
	Scene.SetGeometryProvider(D.RegionId, Original);
	double Fraction = -1.;
	TestTrue(TEXT("buried query"), Scene.QueryOpeningFraction(Door, Fraction, Error));
	TestEqual(TEXT("solid interior blocks despite no nearby triangle"), Fraction, 0.);
	FCCLTerrainAuthority Authority;
	Authority.PrincipalId = FGuid(1171, 1172, 1173, 1174);
	Authority.AllowedBoundsMeters = FCCLTerrainStore::EditableBoundsMeters(D);
	Authority.bCanExcavate = 1;
	Authority.bCanDeposit = 1;
	FCCLTerrainEdit Edit;
	Edit.PrincipalId = Authority.PrincipalId;
	Edit.Sequence = 1;
	Edit.ExpectedRevision = Store.GetRevision();
	Edit.Epoch = Store.GetEpoch();
	Edit.CenterMeters = FVector(0., 0., -4.);
	Edit.RadiusMeters = 2.;
	FCCLTerrainCandidate Candidate;
	const auto ExcavationResult = Store.PrepareEdit(Edit, Authority, Candidate, Error);
	if (!TestTrue(FString::Printf(TEXT("prepare excavation: %s"), *Error), ExcavationResult == ECCLTerrainPrepareResult::Prepared))
	{
		return false;
	}
	TestTrue(TEXT("pending excavation query"), Scene.QueryOpeningFraction(Door, Fraction, Error));
	TestEqual(TEXT("uncommitted excavation cannot open passage"), Fraction, 0.);
	TestTrue(TEXT("commit excavation"), Store.CommitEdit(Candidate, Authority, Error));
	TestTrue(TEXT("old publication queried"), Scene.QueryOpeningFraction(Door, Fraction, Error));
	TestEqual(TEXT("scalar commit alone cannot replace geometry"), Fraction, 0.);
	Scene.SetGeometryProvider(D.RegionId, VentTerrain(Store, Transform, Error));
	TestTrue(TEXT("excavated mesh built"), Error.IsEmpty());
	TestTrue(TEXT("published cavity queried"), Scene.QueryOpeningFraction(Door, Fraction, Error));
	TestEqual(TEXT("cavity reopens aperture"), Fraction, 1.);
	bool bOldSolid = false;
	TestTrue(TEXT("old immutable density queried"), Original->QuerySolidPoint(D.BodyId, Wall.CenterMeters, bOldSolid, Error));
	TestTrue(TEXT("previous publication retains solid volume"), bOldSolid);
	Edit.Sequence++;
	Edit.ExpectedRevision = Store.GetRevision();
	Edit.Kind = ECCLTerrainEditKind::Deposit;
	const auto DepositResult = Store.PrepareEdit(Edit, Authority, Candidate, Error);
	if (!TestTrue(FString::Printf(TEXT("prepare deposit: %s"), *Error), DepositResult == ECCLTerrainPrepareResult::Prepared))
	{
		return false;
	}
	TestTrue(TEXT("commit deposit"), Store.CommitEdit(Candidate, Authority, Error));
	Scene.SetGeometryProvider(D.RegionId, VentTerrain(Store, Transform, Error));
	TestTrue(TEXT("deposited mesh built"), Error.IsEmpty());
	TestTrue(TEXT("refilled queried"), Scene.QueryOpeningFraction(Door, Fraction, Error));
	TestEqual(TEXT("deposit blocks again"), Fraction, 0.);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLVentCoupledTest, "CCL.Environment.Air.GeometryControlsExchangeAndRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLVentCoupledTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLSurfaceSimulation Original;
	TestTrue(TEXT("world"), Original.Initialize(FGuid::NewGuid(), 0., 0., 0, Error));
	FCCLAirSpace Room;
	Room.SpaceId = VentDoor().SpaceA;
	Room.MinimumMeters = FVector(-4., -4., -4.);
	Room.MaximumMeters = FVector(0., 4., 4.);
	Room.VolumeCubicMeters = 64.;
	TestTrue(TEXT("room initialized"), Room.Initialize(20., 0.004, Error));
	Room.Initial.SmokeKg = Room.Contents.SmokeKg = 1.;
	TestTrue(TEXT("room added"), Original.AddAirSpace(Room, Error));
	TArray<FCCLSurfaceOpening> Doors = {VentDoor()};
	FCCLSurfaceScene Scene;
	FKSIWorldStep Step;
	Step.StepId = 1;
	Step.GameToSeconds = 0.25;
	Scene.Replace({VentWall()}, Doors, 1, Error);
	auto Clear = Original;
	TestTrue(TEXT("unobstructed exchange"), Clear.Advance(Step, Error, nullptr, nullptr, &Scene, &Doors));
	const double FullExport = Clear.GetAirSimulation().GetExteriorExported().SmokeKg;
	TestTrue(TEXT("smoke leaves"), FullExport > 0.);
	auto Blocker = VentBlocker();
	Scene.Replace({VentWall(), Blocker}, Doors, 2, Error);
	auto Blocked = Original;
	TestTrue(TEXT("blocked exchange"), Blocked.Advance(Step, Error, nullptr, nullptr, &Scene, &Doors));
	TestEqual(TEXT("no export through blocker"), Blocked.GetAirSimulation().GetExteriorExported().SmokeKg, 0.);
	Blocker.HalfExtentsMeters.X = 0.5;
	Blocker.CenterMeters.Y = -0.5;
	Scene.Replace({VentWall(), Blocker}, Doors, 3, Error);
	auto Half = Original;
	TestTrue(TEXT("half exchange"), Half.Advance(Step, Error, nullptr, nullptr, &Scene, &Doors));
	TestTrue(TEXT("half area yields half export"), FMath::IsNearlyEqual(Half.GetAirSimulation().GetExteriorExported().SmokeKg, FullExport * 0.5, 1.e-12));
	TArray<uint8> Before, After;
	Original.Capture(Before, Error);
	Scene.Replace({VentWall()}, {}, 4, Error);
	TestFalse(TEXT("stale geometry context rejected"), Original.Advance(Step, Error, nullptr, nullptr, &Scene, &Doors));
	Original.Capture(After, Error);
	TestTrue(TEXT("failed geometry retains all state"), Before == After);
	return true;
}
#endif
