#pragma once

#include "CoreMinimal.h"

struct FCCLTerrainSnapshot;

struct CCL_API FCCLTerrainMesh
{
	FGuid WorldId;
	FGuid RegionId;
	FGuid Epoch;
	FIntVector Chunk = FIntVector::ZeroValue;
	uint64 WorldRevision = 0;
	uint64 ChunkRevision = 0;
	FVector OriginMeters = FVector::ZeroVector;
	FIntVector MinimumCell = FIntVector::ZeroValue;
	FIntVector MaximumCell = FIntVector::ZeroValue;
	TArray<FVector> VerticesMeters;
	TArray<FIntVector> Triangles;
	TArray<FIntVector> TriangleCells;
	TArray<uint16> TriangleMaterials;
};

// CPU-only extraction; the caller owns publication and collision readiness.
class CCL_API FCCLTerrainMesher
{
public:
	static bool BuildChunk(const FCCLTerrainSnapshot& Snapshot, const FIntVector& Chunk, const FGuid& Epoch,
		FCCLTerrainMesh& OutMesh, FString& Error, TFunction<bool()> Cancel = {});
};
