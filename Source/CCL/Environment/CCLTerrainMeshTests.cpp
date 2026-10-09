#include "CCLTerrainMesher.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "CCLTerrainStore.h"
#include "Misc/AutomationTest.h"
#include "VectorUtil.h"

namespace
{
	FCCLTerrainDefinition MeshDefinition()
	{
		FCCLTerrainDefinition D;
		D.WorldId = FGuid(11, 12, 13, 14);
		D.RegionId = FGuid(21, 22, 23, 24);
		D.DefinitionId = FGuid(31, 32, 33, 34);
		return D;
	}

	bool Excavate(FCCLTerrainStore& Store, const FVector& Center, double Radius, FString& Error)
	{
		FCCLTerrainAuthority Authority;
		Authority.PrincipalId = FGuid(100, 200, 300, 400);
		Authority.AllowedBoundsMeters = FCCLTerrainStore::EditableBoundsMeters(Store.GetSnapshot().Definition);
		Authority.bCanExcavate = 1;
		FCCLTerrainEdit Edit;
		Edit.PrincipalId = Authority.PrincipalId;
		Edit.Sequence = 1;
		Edit.Epoch = Store.GetEpoch();
		Edit.ExpectedRevision = Store.GetRevision();
		Edit.CenterMeters = Center;
		Edit.RadiusMeters = Radius;
		FCCLTerrainCandidate Candidate;
		return Store.PrepareEdit(Edit, Authority, Candidate, Error) == ECCLTerrainPrepareResult::Prepared
			&& Store.CommitEdit(Candidate, Authority, Error);
	}

	FString PointKey(const FVector& P)
	{
		return FString::Printf(TEXT("%lld,%lld,%lld"), FMath::RoundToInt64(P.X * 1.e8),
			FMath::RoundToInt64(P.Y * 1.e8), FMath::RoundToInt64(P.Z * 1.e8));
	}

	TMap<FString, int32> SeamEdges(const FCCLTerrainMesh& Mesh)
	{
		TMap<FString, int32> Edges;
		for (const FIntVector& T : Mesh.Triangles)
		{
			for (int32 I = 0; I < 3; ++I)
			{
				const FVector A = Mesh.OriginMeters + Mesh.VerticesMeters[T[I]];
				const FVector B = Mesh.OriginMeters + Mesh.VerticesMeters[T[(I + 1) % 3]];
				if (FMath::Abs(A.X) < 1.e-9 && FMath::Abs(B.X) < 1.e-9)
				{
					FString First = PointKey(A);
					FString Second = PointKey(B);
					if (First > Second)
					{
						Swap(First, Second);
					}

					++Edges.FindOrAdd(First + TEXT("/") + Second);
				}
			}
		}

		return Edges;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLTerrainPlaneMeshTest, "CCL.Environment.Terrain.Mesh.OwnedCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLTerrainPlaneMeshTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLTerrainStore Store;
	Store.Initialize(MeshDefinition(), Error);
	FCCLTerrainMesh Mesh;
	if (!TestTrue(TEXT("extract flat ground chunk"), FCCLTerrainMesher::BuildChunk(Store.GetSnapshot(), FIntVector(0, 0, -1), Store.GetEpoch(), Mesh, Error)))
	{
		AddError(Error);
		return false;
	}

	TestEqual(TEXT("exactly 32 by 32 cells, no extra strip"), Mesh.Triangles.Num(), 2 * 32 * 32);
	TestEqual(TEXT("one vertex for each ground lattice column"), Mesh.VerticesMeters.Num(), 33 * 33);
	TestEqual(TEXT("source revision attached"), Mesh.WorldRevision, Store.GetRevision());
	TestEqual(TEXT("execution epoch attached"), Mesh.Epoch, Store.GetEpoch());
	TestEqual(TEXT("one material per triangle"), Mesh.TriangleMaterials.Num(), Mesh.Triangles.Num());
	for (const FVector& P : Mesh.VerticesMeters)
	{
		TestTrue(TEXT("no vertex spills over chunk XY bounds"), P.X >= 0. && P.X <= 16. && P.Y >= 0. && P.Y <= 16.);
		TestTrue(TEXT("zero surface respects engine interpolation tolerance"), FMath::Abs((P + Mesh.OriginMeters).Z) < 1.e-6);
	}

	for (const FIntVector& T : Mesh.Triangles)
	{
		const FVector N = UE::Geometry::VectorUtil::Normal(Mesh.VerticesMeters[T.X],
			Mesh.VerticesMeters[T.Y], Mesh.VerticesMeters[T.Z]);
		TestTrue(TEXT("ground triangle faces empty space above it"), N.Z > 0.);
	}

	TestTrue(TEXT("extract above-ground chunk"), FCCLTerrainMesher::BuildChunk(Store.GetSnapshot(), FIntVector::ZeroValue, Store.GetEpoch(), Mesh, Error));
	TestEqual(TEXT("ground is not duplicated by the upper chunk"), Mesh.Triangles.Num(), 0);
	auto Partial = MeshDefinition();
	Partial.MinimumCell = FIntVector(-35, -2, -4);
	Partial.MaximumCell = FIntVector(2, 4, 4);
	Store.Initialize(Partial, Error);
	TestTrue(TEXT("extract partial negative boundary chunk"), FCCLTerrainMesher::BuildChunk(Store.GetSnapshot(), FIntVector(-2, -1, -1), Store.GetEpoch(), Mesh, Error));
	TestEqual(TEXT("partial chunk is three by two ground cells"), Mesh.Triangles.Num(), 12);
	TestEqual(TEXT("partial minimum preserved"), Mesh.MinimumCell, FIntVector(-35, -2, -4));
	TestEqual(TEXT("partial maximum preserved"), Mesh.MaximumCell, FIntVector(-32, 0, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLTerrainSeamMeshTest, "CCL.Environment.Terrain.Mesh.SharedSeam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLTerrainSeamMeshTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLTerrainStore Store;
	Store.Initialize(MeshDefinition(), Error);
	TestTrue(TEXT("excavate across signed chunk boundary"), Excavate(Store, FVector(0., 0., -0.3), 2.3, Error));
	FCCLTerrainMesh Left;
	FCCLTerrainMesh Right;
	if (!TestTrue(TEXT("both seam meshes extract"), FCCLTerrainMesher::BuildChunk(Store.GetSnapshot(), FIntVector(-1, 0, -1), Store.GetEpoch(), Left, Error)
		&& FCCLTerrainMesher::BuildChunk(Store.GetSnapshot(), FIntVector(0, 0, -1), Store.GetEpoch(), Right, Error)))
	{
		AddError(Error);
		return false;
	}

	const auto LeftEdges = SeamEdges(Left);
	const auto RightEdges = SeamEdges(Right);
	TestTrue(TEXT("seam contains actual edges"), LeftEdges.Num() > 0);
	TestEqual(TEXT("same number of seam segments"), LeftEdges.Num(), RightEdges.Num());
	for (const auto& E : LeftEdges)
	{
		const int32* Other = RightEdges.Find(E.Key);
		TestTrue(TEXT("each seam edge has matching endpoint positions and single ownership"), E.Value == 1 && Other && *Other == 1);
	}

	TestTrue(TEXT("pit descends below the base plane"), Right.VerticesMeters.ContainsByPredicate([&](const FVector& P)
	{
		return (P + Right.OriginMeters).Z < -2.;
	}));
	FCCLTerrainMesh Again;
	TestTrue(TEXT("extract same revision again"), FCCLTerrainMesher::BuildChunk(Store.GetSnapshot(), Right.Chunk, Store.GetEpoch(), Again, Error));
	TestTrue(TEXT("same input has stable geometry ordering"), Again.VerticesMeters == Right.VerticesMeters && Again.Triangles == Right.Triangles
		&& Again.TriangleMaterials == Right.TriangleMaterials && Again.TriangleCells == Right.TriangleCells);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLTerrainCaveMeshTest, "CCL.Environment.Terrain.Mesh.ClosedCavity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLTerrainCaveMeshTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLTerrainStore Store;
	Store.Initialize(MeshDefinition(), Error);
	const FVector Center(8., 8., -8.);
	TestTrue(TEXT("create completely underground cavity"), Excavate(Store, Center, 2.3, Error));
	FCCLTerrainMesh Mesh;
	if (!TestTrue(TEXT("extract cavity and ground in one volume"), FCCLTerrainMesher::BuildChunk(Store.GetSnapshot(), FIntVector(0, 0, -1), Store.GetEpoch(), Mesh, Error)))
	{
		AddError(Error);
		return false;
	}

	TMap<uint64, int32> CavityEdges;
	int32 CavityTriangles = 0;
	int32 CeilingTriangles = 0;
	int32 FloorTriangles = 0;
	for (const FIntVector& T : Mesh.Triangles)
	{
		const FVector A = Mesh.OriginMeters + Mesh.VerticesMeters[T.X];
		const FVector B = Mesh.OriginMeters + Mesh.VerticesMeters[T.Y];
		const FVector C = Mesh.OriginMeters + Mesh.VerticesMeters[T.Z];
		const FVector Mid = (A + B + C) / 3.;
		if (Mid.Z < -4.)
		{
			++CavityTriangles;
			CeilingTriangles += Mid.Z > Center.Z + 1.;
			FloorTriangles += Mid.Z < Center.Z - 1.;
			TestTrue(TEXT("cavity wall faces its empty interior"), FVector::DotProduct(UE::Geometry::VectorUtil::Normal(A, B, C), Center - Mid) > 0.);
			for (int32 I = 0; I < 3; ++I)
			{
				const uint32 Low = FMath::Min(T[I], T[(I + 1) % 3]);
				const uint32 High = FMath::Max(T[I], T[(I + 1) % 3]);
				++CavityEdges.FindOrAdd((uint64(Low) << 32) | High);
			}
		}
	}

	TestTrue(TEXT("cavity has a floor, walls and ceiling"), CavityTriangles > 0 && CeilingTriangles > 0 && FloorTriangles > 0);
	for (const auto& E : CavityEdges)
	{
		TestEqual(TEXT("every cavity edge has two incident triangles; no open seam"), E.Value, 2);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLTerrainCancelledMeshTest, "CCL.Environment.Terrain.Mesh.Cancellation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLTerrainCancelledMeshTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLTerrainStore Store;
	Store.Initialize(MeshDefinition(), Error);
	FCCLTerrainMesh Mesh;
	Mesh.WorldRevision = 1234;
	Mesh.VerticesMeters.Add(FVector(1., 2., 3.));
	int32 Calls = 0;
	TestFalse(TEXT("cancel during generation"), FCCLTerrainMesher::BuildChunk(Store.GetSnapshot(), FIntVector(0, 0, -1), Store.GetEpoch(), Mesh, Error, [&]()
	{
		return ++Calls >= 12;
	}));
	TestTrue(TEXT("cancellation polled during extraction"), Calls >= 12 && Error.Contains(TEXT("cancelled")));
	TestEqual(TEXT("partial work did not publish revision"), Mesh.WorldRevision, uint64(1234));
	TestEqual(TEXT("partial work did not replace vertices"), Mesh.VerticesMeters.Num(), 1);
	TestFalse(TEXT("invalid chunk rejected before grid arithmetic"), FCCLTerrainMesher::BuildChunk(Store.GetSnapshot(), FIntVector(MAX_int32), Store.GetEpoch(), Mesh, Error));
	TestFalse(TEXT("missing epoch rejected"), FCCLTerrainMesher::BuildChunk(Store.GetSnapshot(), FIntVector::ZeroValue, FGuid(), Mesh, Error));
	TestEqual(TEXT("all errors leave output untouched"), Mesh.VerticesMeters[0], FVector(1., 2., 3.));
	return true;
}
#endif
