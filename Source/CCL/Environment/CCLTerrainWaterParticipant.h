#pragma once

#include "CoreMinimal.h"
#include "CCLTerrainRegion.h"
#include "CCLSurfaceSimulation.h"

class UCCLWorldSimulationSubsystem;

// Geometry is prepared once, but water is sampled again immediately before collision publication.
class CCL_API FCCLTerrainWaterParticipant final : public ICCLTerrainEditParticipant
{
public:
	FCCLTerrainWaterParticipant(UCCLWorldSimulationSubsystem* Owner, FGuid RegionId, const FTransform& TerrainTransform);
	virtual bool Prepare(const FCCLTerrainCandidate& Candidate, FString& Error) override;
	virtual bool ValidateCommit(const FCCLTerrainCandidate& Candidate, FString& Error) const override;
	virtual void Commit(const FCCLTerrainCandidate& Candidate) override;
	virtual void Abort(const FCCLTerrainCandidate& Candidate) override;
	virtual bool ValidateRestore(const FCCLTerrainSnapshot& Terrain, const FCCLWorldSnapshot* World, FString& Error) const override;
	virtual void CommitRestore() override;

	static bool ReadBeds(const FCCLTerrainSnapshot& Terrain, const FTransform& Transform,
		const FCCLSurfaceGrid& Grid, TArray<double>& Beds, FString& Error);

private:
	TWeakObjectPtr<UCCLWorldSimulationSubsystem> Owner;
	FGuid RegionId;
	FTransform Transform;
	FGuid Ticket;
	TArray<double> Beds;
	mutable FCCLSurfaceSimulation Prepared;
	mutable uint8 bRestoreWorld = 0;
};
