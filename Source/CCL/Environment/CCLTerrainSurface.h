#pragma once

#include "CoreMinimal.h"
#include "CCLSurfaceQuery.h"
#include "CCLTerrainMesher.h"
#include "CCLTerrainStore.h"

// Immutable geometry captured at the same publication boundary as terrain collision.
class CCL_API FCCLTerrainSurface final : public ICCLSurfaceProvider
{
public:
	FCCLTerrainSurface(const FCCLTerrainDefinition& InDefinition, uint64 InRevision, FGuid InEpoch,
		const FTransform& InTransform, TArray<TSharedRef<const FCCLTerrainMesh>> InMeshes);

	virtual bool QuerySurfaces(const FCCLSurfaceQuery& Query, TArray<FCCLSurfaceSample>& OutSamples, FString& Error) const override;
	virtual uint64 GetRevision() const override { return Revision; }
	virtual FGuid GetEpoch() const override { return Epoch; }

private:
	FCCLTerrainDefinition Definition;
	uint64 Revision = 0;
	FGuid Epoch;
	FTransform Transform;
	TArray<TSharedRef<const FCCLTerrainMesh>> Meshes;
};
