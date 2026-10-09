#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CCLWorldSnapshot.h"
#include "CCLEnvironmentView.h"
#include "CCLWorldSimulationSubsystem.generated.h"

class ACCLWorldEnvironmentState;

UCLASS()
class CCL_API UCCLWorldSimulationSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

public:
	bool Start(FCCLLifeSimulation& Life, ECCLWorldDomain Domain, const TArray<uint8>& Saved, FString& Error);
	bool QueueGameTime(double Seconds, FString& Error);
	bool AdvancePending(double MaxGameSeconds, double MaxWorldSeconds, FString& Error, int32 MaxLifeSlices = 256);
	bool ChangeTimeScale(double Scale, FString& Error);
	bool Save(const FCCLLifeSimulation& Life, TArray<uint8>& Bytes, FString& Error);
	bool Restore(FCCLLifeSimulation& Life, const TArray<uint8>& Bytes, FString& Error);

	bool ReplaceEnvironmentInputs(const FCCLEnvironmentInputs& Candidate, FString& Error);
	bool ObserveCelestials(FCCLCelestialObservation& Observation, FString& Error) const;

	const FCCLEnvironmentInputs& GetEnvironmentInputs() const { return EnvironmentInputs; }
	const ICCLSurfaceProvider& GetSurfaceProvider() const { return SurfaceScene; }
	const FCCLWorldClock& GetClock() const { return Clock; }
	const FCCLWorldIdentity& GetIdentity() const { return Identity; }
	const FString& GetLastError() const { return LastError; }
	bool IsRunning() const { return bRunning != 0; }
	static ECCLWorldDomain DomainForWorld(const UWorld* World);

private:
	void Publish();
	bool BuildEnvironmentView(FCCLEnvironmentView& OutView, FString& Error) const;
	bool CheckAuthority(FString& Error) const;
	void ReportFailure(const FString& Error);

private:
	UPROPERTY(Transient)
	TObjectPtr<ACCLWorldEnvironmentState> ReplicatedState;

	FCCLWorldClock Clock;
	FCCLEnvironmentInputs EnvironmentInputs;
	FCCLCelestialSystem CelestialSystem;
	FCCLSurfaceScene SurfaceScene;
	FCCLWorldIdentity Identity;
	FGuid Epoch;
	FString LastError;
	double UnqueuedGameSeconds = 0;
	uint8 bRunning = 0;
};
