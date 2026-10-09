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
	auto Surface = MakeShared<FCCLTerrainSurface>(Definition, Store.GetRevision(), Store.GetEpoch(), Transform, Meshes);
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
	FCCLTerrainSurface Reordered(Definition, Store.GetRevision(), Store.GetEpoch(), Transform, Meshes);
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
#endif
