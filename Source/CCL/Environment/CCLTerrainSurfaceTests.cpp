#include "CCLTerrainSurface.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Algo/Reverse.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLTerrainSurfaceTest, "CCL.Environment.Terrain.SurfaceLayers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLTerrainSurfaceTest::RunTest(const FString& Parameters)
{
	FCCLTerrainDefinition Definition;
	Definition.WorldId = FGuid(1, 2, 3, 4);
	Definition.RegionId = FGuid(5, 6, 7, 8);
	Definition.DefinitionId = FGuid(9, 10, 11, 12);
	Definition.MinimumCell = FIntVector(-16, -16, -16);
	Definition.MaximumCell = FIntVector(16, 16, 8);
	FCCLTerrainStore Store;
	FString Error;
	if (!TestTrue(TEXT("initialize"), Store.Initialize(Definition, Error)))
	{
		return false;
	}

	FCCLTerrainAuthority Authority;
	Authority.PrincipalId = FGuid(11, 12, 13, 14);
	Authority.AllowedBoundsMeters = FCCLTerrainStore::EditableBoundsMeters(Definition);
	Authority.bCanExcavate = 1;
	FCCLTerrainEdit Edit;
	Edit.PrincipalId = Authority.PrincipalId;
	Edit.Sequence = 1;
	Edit.ExpectedRevision = Store.GetRevision();
	Edit.Epoch = Store.GetEpoch();
	Edit.CenterMeters = FVector(0., 0., -4.);
	Edit.RadiusMeters = 1.5;
	FCCLTerrainCandidate Candidate;
	if (!TestTrue(TEXT("cavity prepared"), Store.PrepareEdit(Edit, Authority, Candidate, Error) == ECCLTerrainPrepareResult::Prepared)
		|| !TestTrue(TEXT("cavity committed"), Store.CommitEdit(Candidate, Authority, Error)))
	{
		return false;
	}

	TArray<TSharedRef<const FCCLTerrainMesh>> Meshes;
	for (int32 Z = -1; Z < 1; ++Z)
	{
		for (int32 Y = -1; Y < 1; ++Y)
		{
			for (int32 X = -1; X < 1; ++X)
			{
				auto Mesh = MakeShared<FCCLTerrainMesh>();
				if (!TestTrue(TEXT("mesh extraction"), FCCLTerrainMesher::BuildChunk(Store.GetSnapshot(), FIntVector(X, Y, Z), Store.GetEpoch(), *Mesh, Error)))
				{
					return false;
				}

				Meshes.Add(Mesh);
			}
		}
	}

	const FTransform Transform(FRotator::ZeroRotator, FVector(1000., 2000., 3000.));
	auto Surface = MakeShared<FCCLTerrainSurface>(Store.GetSnapshot(), Store.GetEpoch(), Transform, Meshes);
	FCCLSurfaceQuery Query;
	Query.BodyId = Definition.BodyId;
	Query.OriginMeters = FVector(10.13, 20.17, 32.);
	Query.Direction = -FVector::ZAxisVector;
	Query.MaximumDistanceMeters = 12.;
	Query.RequiredRevision = Store.GetRevision();
	Query.RequiredEpoch = Store.GetEpoch();
	TArray<FCCLSurfaceSample> Hits;
	if (!TestTrue(TEXT("multi-layer query"), Surface->QuerySurfaces(Query, Hits, Error)) || !TestEqual(TEXT("ground ceiling floor"), Hits.Num(), 3))
	{
		return false;
	}

	TestTrue(TEXT("ground normal"), Hits[0].Normal.Z > 0.99);
	TestTrue(TEXT("cave ceiling normal"), Hits[1].Normal.Z < -0.9);
	TestTrue(TEXT("cave floor normal"), Hits[2].Normal.Z > 0.9);
	TestEqual(TEXT("soil material"), Hits[2].MaterialId, FName(TEXT("Soil")));
	TestTrue(TEXT("separate stable elements"), Hits[0].SurfaceId != Hits[1].SurfaceId && Hits[1].SurfaceId != Hits[2].SurfaceId);
	const auto Original = Hits;
	Query.MaximumResults = 2;
	TestFalse(TEXT("no partial query"), Surface->QuerySurfaces(Query, Hits, Error));
	TestEqual(TEXT("failure preserves output"), Hits.Num(), 3);
	Query.MaximumResults = 256;
	Query.RequiredRevision++;
	TestFalse(TEXT("reject stale revision"), Surface->QuerySurfaces(Query, Hits, Error));
	Query.RequiredRevision = Store.GetRevision();
	Query.RequiredEpoch = FGuid::NewGuid();
	TestFalse(TEXT("reject stale epoch"), Surface->QuerySurfaces(Query, Hits, Error));
	Query.RequiredEpoch = Store.GetEpoch();
	Algo::Reverse(Meshes);
	FCCLTerrainSurface Reordered(Store.GetSnapshot(), Store.GetEpoch(), Transform, Meshes);
	TestTrue(TEXT("reordered chunks queried"), Reordered.QuerySurfaces(Query, Hits, Error));
	for (int32 Index = 0; Index < Hits.Num(); ++Index)
	{
		TestEqual(TEXT("ID independent of chunk iteration"), Hits[Index].SurfaceId, Original[Index].SurfaceId);
	}

	FCCLSurfaceScene Scene;
	TestTrue(TEXT("empty fixture scene"), Scene.Replace({}, {}, 1, Error));
	const FGuid OldEpoch = Scene.GetEpoch();
	Scene.SetGeometryProvider(Definition.RegionId, Surface);
	TestTrue(TEXT("scene epoch invalidated"), Scene.GetEpoch() != OldEpoch);
	Query.RequiredEpoch = Scene.GetEpoch();
	Query.RequiredRevision = Scene.GetRevision();
	TestTrue(TEXT("terrain in scene"), Scene.QuerySurfaces(Query, Hits, Error));
	TestEqual(TEXT("composite layers"), Hits.Num(), 3);
	TestEqual(TEXT("source revision retained"), Hits[0].SourceRevision, Store.GetRevision());
	TestEqual(TEXT("scene revision"), Hits[0].Revision, Scene.GetRevision());
	Scene.SetGeometryProvider(Definition.RegionId, nullptr);
	TestFalse(TEXT("removal invalidates old query"), Scene.QuerySurfaces(Query, Hits, Error));
	Query.RequiredEpoch = Scene.GetEpoch();
	TestTrue(TEXT("removed terrain query"), Scene.QuerySurfaces(Query, Hits, Error));
	TestEqual(TEXT("removed terrain has no hits"), Hits.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLTerrainSurfaceBoundsTest, "CCL.Environment.Terrain.SurfaceChunkBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLTerrainSurfaceBoundsTest::RunTest(const FString& Parameters)
{
	FCCLTerrainDefinition Definition;
	Definition.WorldId = FGuid(1, 2, 3, 4);
	Definition.RegionId = FGuid(5, 6, 7, 8);
	Definition.DefinitionId = FGuid(9, 10, 11, 12);
	FCCLTerrainStore Store;
	FString Error;
	if (!TestTrue(TEXT("initialize layered query fixture"), Store.Initialize(Definition, Error))) { return false; }
	TArray<TSharedRef<const FCCLTerrainMesh>> Meshes;
	for (int32 Layer = 0; Layer < 2; ++Layer)
	{
		auto Mesh = MakeShared<FCCLTerrainMesh>();
		Mesh->OriginMeters = FVector(0., 0., -4. * Layer);
		Mesh->VerticesMeters = {FVector(0., 0., 0.), FVector(0., 4., 0.), FVector(4., 4., 0.), FVector(4., 0., 0.)};
		Mesh->Triangles = {FIntVector(0, 1, 2), FIntVector(0, 2, 3)};
		Mesh->TriangleCells = {FIntVector(0, 0, -4 * Layer), FIntVector(0, 0, -4 * Layer)};
		Mesh->TriangleMaterials = {0, 0};
		Meshes.Add(Mesh);
	}
	Meshes.Add(MakeShared<FCCLTerrainMesh>());
	for (const FRotator Rotation : {FRotator::ZeroRotator, FRotator(17., 63., -12.)})
	{
		const FTransform Transform(Rotation, FVector(1000., 2000., 3000.));
		const FCCLTerrainSurface Surface(Store.GetSnapshot(), Store.GetEpoch(), Transform, Meshes);
		FCCLSurfaceQuery Query;
		Query.BodyId = Definition.BodyId;
		Query.RequiredRevision = Store.GetRevision();
		Query.RequiredEpoch = Store.GetEpoch();
		Query.MaximumDistanceMeters = 12.;
		Query.Direction = Transform.TransformVectorNoScale(-FVector::ZAxisVector);
		TArray<FCCLSurfaceSample> Hits;
		for (const FVector2D XY : {FVector2D(2., 2.), FVector2D(0., 0.), FVector2D(4., 4.),
			FVector2D(-2.e-10, 2.), FVector2D(4. + 2.e-10, 2.)})
		{
			Query.OriginMeters = Transform.TransformPosition(FVector(XY.X, XY.Y, 2.) * 100.) / 100.;
			if (!TestTrue(TEXT("axis, vertex and tolerant edge rays query"), Surface.QuerySurfaces(Query, Hits, Error))
				|| !TestEqual(TEXT("both layers survive chunk bounds"), Hits.Num(), 2)) { return false; }
			TestTrue(TEXT("top distance unchanged"), FMath::IsNearlyEqual(Hits[0].DistanceMeters, 2., 1.e-7));
			TestTrue(TEXT("bottom distance unchanged"), FMath::IsNearlyEqual(Hits[1].DistanceMeters, 6., 1.e-7));
		}
		Query.OriginMeters = Transform.TransformPosition(FVector(2., 2., 2.) * 100.) / 100.;
		Query.MaximumDistanceMeters = 1.9;
		TestTrue(TEXT("short segment valid"), Surface.QuerySurfaces(Query, Hits, Error));
		TestEqual(TEXT("short segment cannot reach terrain"), Hits.Num(), 0);
		Query.MinimumDistanceMeters = 2.1;
		Query.MaximumDistanceMeters = 6.1;
		TestTrue(TEXT("minimum distance clips near layer"), Surface.QuerySurfaces(Query, Hits, Error));
		TestEqual(TEXT("only distant layer remains"), Hits.Num(), 1);
		Query.MinimumDistanceMeters = 0.;
		Query.MaximumDistanceMeters = 12.;
		Query.OriginMeters = Transform.TransformPosition(FVector(2., 2., -6.) * 100.) / 100.;
		Query.Direction = -Query.Direction;
		TestTrue(TEXT("reverse direction valid"), Surface.QuerySurfaces(Query, Hits, Error));
		TestEqual(TEXT("reverse direction preserves both layers"), Hits.Num(), 2);
		Query.OriginMeters = Transform.TransformPosition(FVector(200., 200., 2.) * 100.) / 100.;
		Query.Direction = -Query.Direction;
		TestTrue(TEXT("distant parallel ray valid"), Surface.QuerySurfaces(Query, Hits, Error));
		TestEqual(TEXT("distant ray has no false hits"), Hits.Num(), 0);
	}
	return true;
}
#endif
