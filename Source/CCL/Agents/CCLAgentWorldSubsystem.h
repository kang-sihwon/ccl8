#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CCLLifeSimulation.h"
#include "CCLAgentWorldSubsystem.generated.h"

UCLASS()
class CCL_API UCCLAgentSessionStore : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void ResetSession();

public:
	TArray<uint8> Snapshot;
	uint64 Session = 1;
};

UCLASS()
class CCL_API UCCLAgentWorldSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& World) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

public:
	bool Save(TArray<uint8>& Bytes);
	bool Restore(const TArray<uint8>& Bytes, FString& Error);
	FCCLLifeSimulation& GetSimulation() { return Simulation; }
	bool IsRunning() const { return bRunning != 0; }

private:
	void SpawnVillage();

private:
	FCCLLifeSimulation Simulation;
	double PendingSeconds = 0;
	uint64 Session = 0;
	uint8 bRunning = 0;
};
