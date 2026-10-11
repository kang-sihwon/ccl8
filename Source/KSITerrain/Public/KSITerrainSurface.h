#pragma once

#include "CoreMinimal.h"
#include "KSISurfaceQuery.h"
#include "KSITerrainMesher.h"
#include "KSITerrainStore.h"

// Immutable geometry captured at the same publication boundary as terrain collision.
class KSITERRAIN_API FKSITerrainSurface final : public IKSISurfaceProvider
{
public:
	FKSITerrainSurface(const FKSITerrainSnapshot& InSnapshot, FGuid InEpoch,
		const FTransform& InTransform, TArray<TSharedRef<const FKSITerrainMesh>> InMeshes);

	virtual bool QuerySurfaces(const FKSISurfaceQuery& Query, TArray<FKSISurfaceSample>& OutSamples, FString& Error) const override;
	virtual bool QuerySolidPoint(FName BodyId, const FVector3d& PositionMeters, bool& bOutSolid, FString& Error) const override;
	virtual uint64 GetRevision() const override { return Revision; }
	virtual FGuid GetEpoch() const override { return Epoch; }

private:
	FKSITerrainSnapshot Snapshot;
	FKSITerrainDefinition Definition;
	uint64 Revision = 0;
	FGuid Epoch;
	FTransform Transform;
	TArray<TSharedRef<const FKSITerrainMesh>> Meshes;
	TArray<FBox> MeshBounds;
};
