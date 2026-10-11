#pragma once

#include "CoreMinimal.h"
#include "CCLTerrainRegion.h"
#include "KSISurfaceSimulation.h"

class UCCLWorldSimulationSubsystem;

// Geometry is prepared once, but water is sampled again immediately before collision publication.
class CCL_API FCCLTerrainWaterParticipant final : public ICCLTerrainEditParticipant
{
public:
	FCCLTerrainWaterParticipant(UCCLWorldSimulationSubsystem* Owner, FGuid RegionId, const FTransform& TerrainTransform);
	virtual bool Prepare(const FKSITerrainCandidate& Candidate, FString& Error) override;
	virtual bool ValidateCommit(const FKSITerrainCandidate& Candidate, FString& Error) const override;
	virtual void Commit(const FKSITerrainCandidate& Candidate) override;
	virtual void Abort(const FKSITerrainCandidate& Candidate) override;
	virtual bool ValidateRestore(const FKSITerrainSnapshot& Terrain, const FCCLWorldSnapshot* World, FString& Error) const override;
	virtual void CommitRestore() override;

	static bool ReadBeds(const FKSITerrainSnapshot& Terrain, const FTransform& Transform,
		const FKSISurfaceGrid& Grid, TArray<double>& Beds, FString& Error);

private:
	TWeakObjectPtr<UCCLWorldSimulationSubsystem> Owner;
	FGuid RegionId;
	FTransform Transform;
	FGuid Ticket;
	TArray<double> Beds;
	mutable FKSISurfaceSimulation Prepared;
	mutable uint8 bRestoreWorld = 0;
};
