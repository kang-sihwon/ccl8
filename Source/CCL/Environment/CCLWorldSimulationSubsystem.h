#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CCLWorldSnapshot.h"
#include "CCLSurfaceSimulation.h"
#include "CCLEcologySimulation.h"
#include "CCLEnvironmentView.h"
#include "CCLWorldSimulationSubsystem.generated.h"

class ACCLWorldEnvironmentState;
class ICCLWorldParticipant;
class FCCLTerrainWaterParticipant;

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
	bool AcquireManualAdvance(UObject* Owner, FString& Error);
	void ReleaseManualAdvance(const UObject* Owner);
	bool QueueGameTime(double Seconds, FString& Error);
	bool AdvancePending(double MaxGameSeconds, double MaxWorldSeconds, FString& Error, int32 MaxLifeSlices = 256);
	bool ChangeTimeScale(double Scale, FString& Error);
	// SessionContainer is excluded only when its caller stores it in the same session envelope.
	bool Save(const FCCLLifeSimulation& Life, TArray<uint8>& Bytes, FString& Error, const UObject* SessionContainer = nullptr);
	bool Restore(FCCLLifeSimulation& Life, const TArray<uint8>& Bytes, FString& Error);

	// Game systems that keep a section of the world snapshot register for capture, restore and advance.
	void RegisterParticipant(ICCLWorldParticipant& Participant);
	void UnregisterParticipant(ICCLWorldParticipant& Participant);

	bool SeedEcology(const FCCLEcologySimulation& Candidate, FString& Error);
	bool AddEcologyGroup(const FCCLEcologyGroup& Group, FString& Error);
	bool AddEcologyForage(FCCLEcologyForage Forage, FString& Error);
	bool AddEcologyBehavior(FCCLEcologyBehavior Behavior, FString& Error);
	bool AddEcologySpecies(const FCCLEcologySpecies& Species, FString& Error);
	bool AddEcologyHabitat(const FCCLEcologyHabitat& Habitat, FString& Error);
	bool AddEcologyPopulation(FCCLEcologyPopulation Population, FString& Error);
	bool ApplyEcologyEvent(const FCCLEcologyEvent& Event, FString& Error);
	bool AcquireEcologyIndividual(FGuid Id, ECCLEcologyRepresentation Representation, FGuid Writer,
		FCCLEcologyLease& Lease, FString& Error);
	bool ReleaseEcologyIndividual(const FCCLEcologyLease& Lease, FString& Error);
	bool SuspendEcologyIndividual(const FCCLEcologyLease& Lease, FString& Error);
	bool ReturnEcologyIndividual(FGuid Id, FString& Error);
	bool TransferEcologyIndividual(const FCCLEcologyLease& Lease, ECCLEcologyRepresentation Representation, FGuid Writer,
		FCCLEcologyLease& OutLease, FString& Error);
	bool UpdateEcologyPosition(const FCCLEcologyLease& Lease, const FVector& PositionMeters, FString& Error);
	bool FinishEcologyIndividual(const FCCLEcologyLease& Lease, const FCCLEcologyEvent& Event, FString& Error);
	bool AddSurfaceRegion(FCCLSurfaceGrid Region, FString& Error);
	bool AddAirSpace(const FCCLAirSpace& Space, FString& Error);
	bool IgniteSurface(FGuid RegionId, int32 CellIndex, double HeatJoules, FString& Error);
	bool ChangeClimate(FGuid RegionId, const FCCLClimate& Climate, FString& Error);
	bool ChangeSurfaceForcing(FGuid RegionId, const FCCLSurfaceForcing& Forcing, FString& Error);
	bool ApplySnowContact(FGuid RegionId, FGuid SourceId, uint64 Sequence, const FVector& PositionMeters, double RadiusMeters, FString& Error);
	FGuid GetEpoch() const { return Epoch; }
	bool ReplaceEnvironmentInputs(const FCCLEnvironmentInputs& Candidate, FString& Error);
	void SetGeometryProvider(FGuid Id, TSharedPtr<const ICCLSurfaceProvider> Provider);
	bool ObserveCelestials(FCCLCelestialObservation& Observation, FString& Error) const;

	const FCCLEcologySimulation& GetEcologySimulation() const { return EcologySimulation; }
	const FCCLSurfaceSimulation& GetSurfaceSimulation() const { return SurfaceSimulation; }
	const FCCLEnvironmentInputs& GetEnvironmentInputs() const { return EnvironmentInputs; }
	const ICCLSurfaceProvider& GetSurfaceProvider() const { return SurfaceScene; }
	const FCCLWorldClock& GetClock() const { return Clock; }
	const FCCLWorldIdentity& GetIdentity() const { return Identity; }
	const FString& GetLastError() const { return LastError; }
	bool IsRunning() const { return bRunning != 0; }
	static ECCLWorldDomain DomainForWorld(const UWorld* World);

private:
	friend class FCCLTerrainWaterParticipant;
	void Publish();
	bool BuildEnvironmentView(FCCLEnvironmentView& OutView, FString& Error) const;
	bool CheckAuthority(FString& Error) const;
	void ReportFailure(const FString& Error);
	bool CaptureParticipants(FCCLWorldSnapshot& Snapshot, const UObject* SessionContainer, FString& Error);
	bool PrepareParticipants(const FCCLWorldSnapshot* Snapshot, FGuid WorldId, double WorldSeconds, bool bStarting,
		int32& OutPrepared, FString& Error);
	bool CommitParticipants(int32 Index, int32 Prepared, TFunctionRef<bool()> CommitCore);
	void CancelParticipants(int32 Prepared);

private:
	UPROPERTY(Transient)
	TObjectPtr<ACCLWorldEnvironmentState> ReplicatedState;

	FCCLWorldClock Clock;
	FCCLSurfaceSimulation SurfaceSimulation;
	FCCLEcologySimulation EcologySimulation;
	FCCLEnvironmentInputs EnvironmentInputs;
	FCCLCelestialSystem CelestialSystem;
	FCCLSurfaceScene SurfaceScene;
	FCCLWorldIdentity Identity;
	FGuid Epoch;
	FString LastError;
	TWeakObjectPtr<UObject> ManualAdvanceOwner;

	// Sorted by world order. Participants are world subsystems that unregister while deinitializing.
	TArray<ICCLWorldParticipant*> Participants;
	double UnqueuedGameSeconds = 0;
	uint8 bRunning = 0;
};
