#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CCLLifeSimulation.h"
#include "Environment/CCLWorldSnapshot.h"
#include "CCLAgentWorldSubsystem.generated.h"

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
	bool Save(TArray<uint8>& Bytes);
	bool Restore(const TArray<uint8>& Bytes, FString& Error);
	FCCLLifeSimulation& GetSimulation() { return Simulation; }
	const FCCLLifeSimulation& GetSimulation() const { return Simulation; }
	bool IsRunning() const { return bRunning != 0; }

private:
	void SpawnVillage();

private:
	FCCLLifeSimulation Simulation;
	uint64 Session = 0;
	uint8 bRunning = 0;
};
