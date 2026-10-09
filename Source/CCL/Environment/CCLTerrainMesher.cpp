#include "CCLTerrainMesher.h"

#include "CCLTerrainStore.h"
#include "Generators/MarchingCubes.h"

bool FCCLTerrainMesher::BuildChunk(const FCCLTerrainSnapshot& Snapshot, const FIntVector& Chunk, const FGuid& Epoch,
	FCCLTerrainMesh& OutMesh, FString& Error, TFunction<bool()> Cancel)
{
	Error.Reset();
	if (!Epoch.IsValid() || !FCCLTerrainStore::ValidateSnapshot(Snapshot, Error))
	{
		if (Error.IsEmpty())
		{
			Error = TEXT("Terrain extraction requires a valid execution epoch.");
		}

		return false;
	}

	const auto& D = Snapshot.Definition;
	const FIntVector First = FCCLTerrainStore::OwnerForSample(D.MinimumCell);
	const FIntVector Last = FCCLTerrainStore::OwnerForSample(D.MaximumCell - FIntVector(1));
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		if (Chunk[Axis] < First[Axis] || Chunk[Axis] > Last[Axis])
		{
			Error = TEXT("Terrain mesh chunk is outside the region cell bounds.");
			return false;
		}
	}

	FCCLTerrainMesh Mesh;
	Mesh.WorldId = D.WorldId;
	Mesh.RegionId = D.RegionId;
	Mesh.Epoch = Epoch;
	Mesh.Chunk = Chunk;
	Mesh.WorldRevision = Snapshot.Revision;
	const auto* StoredChunk = Snapshot.Chunks.Find(Chunk);
	Mesh.ChunkRevision = StoredChunk ? (*StoredChunk)->Revision : 1;
	const FIntVector GridOrigin = Chunk * FCCLTerrainStore::SamplesPerAxis;
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		Mesh.MinimumCell[Axis] = FMath::Max(GridOrigin[Axis], D.MinimumCell[Axis]);
		Mesh.MaximumCell[Axis] = FMath::Min(GridOrigin[Axis] + FCCLTerrainStore::SamplesPerAxis, D.MaximumCell[Axis]);
	}

	Mesh.OriginMeters = D.OriginMeters + FVector(Mesh.MinimumCell) * D.SampleSpacingMeters;
	bool bCancelled = false;
	auto CheckCancellation = [&]()
	{
		bCancelled = bCancelled || (Cancel && Cancel());
		return bCancelled;
	};
	if (CheckCancellation())
	{
		Error = TEXT("Terrain extraction cancelled before generation.");
		return false;
	}

	UE::Geometry::FMarchingCubes Generator;
	// The engine uses floor(dimension / CubeSize) + 1 cells. A half-cell adjustment fixes the loop count.
	Generator.Bounds = UE::Geometry::FAxisAlignedBox3d(FVector(Mesh.MinimumCell), FVector(Mesh.MaximumCell) - FVector(0.5));
	Generator.CubeSize = 1.;
	Generator.SafetyMaxDimension = FCCLTerrainStore::SamplesPerAxis;
	Generator.bParallelCompute = false;
	Generator.bEnableValueCaching = false;
	Generator.RootMode = UE::Geometry::ERootfindingModes::SingleLerp;
	Generator.CancelF = CheckCancellation;
	Generator.Implicit = [&Snapshot](FVector P)
	{
		const FIntVector Sample(FMath::RoundToInt(P.X), FMath::RoundToInt(P.Y), FMath::RoundToInt(P.Z));
		return double(FCCLTerrainStore::ReadSample(Snapshot, Sample).DistanceMillimeters);
	};
	Generator.Generate();
	if (CheckCancellation())
	{
		Error = TEXT("Terrain extraction cancelled; partial mesh discarded.");
		return false;
	}

	const FIntVector Cells = Mesh.MaximumCell - Mesh.MinimumCell;
	if (Generator.CellDimensions.X != Cells.X || Generator.CellDimensions.Y != Cells.Y || Generator.CellDimensions.Z != Cells.Z
		|| Generator.Vertices.Num() > 3 * 32 * 33 * 33 || Generator.Triangles.Num() > 5 * 32 * 32 * 32)
	{
		Error = TEXT("Terrain extraction dimensions or mesh budgets do not match the owned cells.");
		return false;
	}

	Mesh.VerticesMeters.Reserve(Generator.Vertices.Num());
	for (const FVector& P : Generator.Vertices)
	{
		if (P.ContainsNaN() || P.X < Mesh.MinimumCell.X || P.Y < Mesh.MinimumCell.Y || P.Z < Mesh.MinimumCell.Z
			|| P.X > Mesh.MaximumCell.X || P.Y > Mesh.MaximumCell.Y || P.Z > Mesh.MaximumCell.Z)
		{
			Error = TEXT("Terrain extraction generated a vertex outside the owned cell closure.");
			return false;
		}

		Mesh.VerticesMeters.Add((P - FVector(Mesh.MinimumCell)) * D.SampleSpacingMeters);
	}

	Mesh.Triangles.Reserve(Generator.Triangles.Num());
	Mesh.TriangleCells.Reserve(Generator.Triangles.Num());
	Mesh.TriangleMaterials.Reserve(Generator.Triangles.Num());
	for (const auto& T : Generator.Triangles)
	{
		if (!Generator.Vertices.IsValidIndex(T.A) || !Generator.Vertices.IsValidIndex(T.B) || !Generator.Vertices.IsValidIndex(T.C)
			|| T.A == T.B || T.A == T.C || T.B == T.C)
		{
			Error = TEXT("Terrain extraction generated an invalid triangle index.");
			return false;
		}

		const FVector& A = Generator.Vertices[T.A];
		const FVector& B = Generator.Vertices[T.B];
		const FVector& C = Generator.Vertices[T.C];
		if (FVector::CrossProduct(B - A, C - A).SizeSquared() <= 0.)
		{
			Error = TEXT("Terrain extraction generated a degenerate triangle.");
			return false;
		}

		const FVector Center = (A + B + C) / 3.;
		FIntVector Cell;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			Cell[Axis] = FMath::Clamp(FMath::FloorToInt(Center[Axis]), Mesh.MinimumCell[Axis], Mesh.MaximumCell[Axis] - 1);
		}

		int16 Lowest = MAX_int16;
		uint16 Material = D.BaseMaterial;
		for (int32 Z = 0; Z < 2; ++Z)
		{
			for (int32 Y = 0; Y < 2; ++Y)
			{
				for (int32 X = 0; X < 2; ++X)
				{
					const auto V = FCCLTerrainStore::ReadSample(Snapshot, Cell + FIntVector(X, Y, Z));
					if (V.Material != 0 && V.DistanceMillimeters < Lowest)
					{
						Lowest = V.DistanceMillimeters;
						Material = V.Material;
					}
				}
			}
		}

		// Positive density is air; reverse the generator winding for Unreal clockwise front faces.
		Mesh.Triangles.Add(FIntVector(T.A, T.C, T.B));
		Mesh.TriangleCells.Add(Cell);
		Mesh.TriangleMaterials.Add(Material);
	}

	if (CheckCancellation())
	{
		Error = TEXT("Terrain extraction cancelled before returning its result.");
		return false;
	}

	OutMesh = MoveTemp(Mesh);
	return true;
}
