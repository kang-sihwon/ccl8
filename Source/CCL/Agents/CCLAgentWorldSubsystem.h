#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CCLLifeSimulation.h"
#include "Environment/CCLWorldGenerationStore.h"
#include "CCLAgentWorldSubsystem.generated.h"

class UCCLAgentWorldSubsystem;

DECLARE_MULTICAST_DELEGATE_OneParam(FCCLAgentPopulationEvent, UCCLAgentWorldSubsystem&);

UCLASS()
class CCL_API UCCLAgentSessionStore : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void ResetSession();
	void ResetDomain(ECCLWorldDomain Domain);
	uint64 SessionFor(ECCLWorldDomain Domain) const;
	TArray<uint8>& SnapshotFor(ECCLWorldDomain Domain);

public:
	TArray<uint8> Snapshot;
	uint64 Session = 1;
	TMap<ECCLWorldDomain, TArray<uint8>> Experiments;
	TMap<ECCLWorldDomain, uint64> ExperimentSessions;
	TMap<ECCLWorldDomain, TArray<uint8>> ExperimentInitialSnapshots;
	TMap<ECCLWorldDomain, FCCLWorldGenerationBundle> TravelSnapshots;
};

UCLASS()
class CCL_API UCCLAgentWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& World) override;
	virtual void Deinitialize() override;

public:
	// SessionContainer is saved by its personal session record and is excluded from the world capture.
	bool Save(TArray<uint8>& Bytes, FString& Error, const UObject* SessionContainer = nullptr);

	// Convenience form for callers that only branch on success. The rejection reason is logged.
	bool Save(TArray<uint8>& Bytes);
	bool SaveSessionForTravel(FString& Error);
	void CancelPreparedTravel() { bTravelPrepared = 0; }
	// Replaces the running world with a snapshot. Participants such as a staged host session join the same commit.
	bool Restore(const TArray<uint8>& Bytes, FString& Error);
	FCCLLifeSimulation& GetSimulation() { return Simulation; }
	const FCCLLifeSimulation& GetSimulation() const { return Simulation; }
	bool IsRunning() const { return bRunning != 0; }
	ECCLWorldDomain GetDomain() const { return Domain; }
	bool RequestExternalDeath(FGuid AgentId, FString& Error);
	bool FlushExternalDeaths(FString& Error);

private:
	void OnWorldBeginTearDown(UWorld* World);

public:
	// Broadcast before a restore replaces the population. Representations are removed without writing back.
	FCCLAgentPopulationEvent OnPopulationRetired;

	// Broadcast after a world starts or restores. Content layers create their representations here.
	FCCLAgentPopulationEvent OnPopulationReady;

private:
	FCCLLifeSimulation Simulation;
	ECCLWorldDomain Domain = ECCLWorldDomain::Campaign;
	TMap<FGuid, FGuid> PendingExternalDeaths;
	uint64 Session = 0;
	uint8 bRunning = 0;
	uint8 bTravelPrepared = 0;
};
