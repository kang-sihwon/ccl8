#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CCLWorldSnapshot.h"
#include "KSISurfaceSimulation.h"
#include "KSIEcologySimulation.h"
#include "KSIEnvironmentView.h"
#include "KSIWorldTime.h"
#include "CCLWorldSimulationSubsystem.generated.h"

class ACCLWorldEnvironmentState;
class IKSIWorldParticipant;
class ICCLWorldAdvanceParticipant;
class FCCLTerrainWaterParticipant;

UCLASS()
class CCL_API UCCLWorldSimulationSubsystem : public UTickableWorldSubsystem, public IKSIWorldTimeSource
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
    virtual bool ReadWorldTime(FKSIWorldTimeView& OutTime) const override;

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

	// Game systems that keep a section of the world snapshot register for capture and restore.
	void RegisterParticipant(IKSIWorldParticipant& Participant);
	void UnregisterParticipant(IKSIWorldParticipant& Participant);
	void RegisterAdvanceParticipant(ICCLWorldAdvanceParticipant& Participant);
	void UnregisterAdvanceParticipant(ICCLWorldAdvanceParticipant& Participant);

	bool SeedEcology(const FKSIEcologySimulation& Candidate, FString& Error);
	bool AddEcologyGroup(const FKSIEcologyGroup& Group, FString& Error);
	bool AddEcologyForage(FKSIEcologyForage Forage, FString& Error);
	bool AddEcologyBehavior(FKSIEcologyBehavior Behavior, FString& Error);
	bool AddEcologySpecies(const FKSIEcologySpecies& Species, FString& Error);
	bool AddEcologyHabitat(const FKSIEcologyHabitat& Habitat, FString& Error);
	bool AddEcologyPopulation(FKSIEcologyPopulation Population, FString& Error);
	bool ApplyEcologyEvent(const FKSIEcologyEvent& Event, FString& Error);
	bool AcquireEcologyIndividual(FGuid Id, EKSIEcologyRepresentation Representation, FGuid Writer,
		FKSIEcologyLease& Lease, FString& Error);
	bool ReleaseEcologyIndividual(const FKSIEcologyLease& Lease, FString& Error);
	bool SuspendEcologyIndividual(const FKSIEcologyLease& Lease, FString& Error);
	bool ReturnEcologyIndividual(FGuid Id, FString& Error);
	bool TransferEcologyIndividual(const FKSIEcologyLease& Lease, EKSIEcologyRepresentation Representation, FGuid Writer,
		FKSIEcologyLease& OutLease, FString& Error);
	bool UpdateEcologyPosition(const FKSIEcologyLease& Lease, const FVector& PositionMeters, FString& Error);
	bool FinishEcologyIndividual(const FKSIEcologyLease& Lease, const FKSIEcologyEvent& Event, FString& Error);
	bool AddSurfaceRegion(FKSISurfaceGrid Region, FString& Error);
	bool AddAirSpace(const FKSIAirSpace& Space, FString& Error);
	bool IgniteSurface(FGuid RegionId, int32 CellIndex, double HeatJoules, FString& Error);
	bool ChangeClimate(FGuid RegionId, const FKSIClimate& Climate, FString& Error);
	bool ChangeSurfaceWind(FGuid RegionId, double MetersPerSecond, FString& Error);
	bool ChangeSurfaceForcing(FGuid RegionId, const FKSISurfaceForcing& Forcing, FString& Error);
	bool ApplySnowContact(FGuid RegionId, FGuid SourceId, uint64 Sequence, const FVector& PositionMeters, double RadiusMeters, FString& Error);
	FGuid GetEpoch() const { return Epoch; }
	bool ReplaceEnvironmentInputs(const FKSIEnvironmentInputs& Candidate, FString& Error);
	void SetGeometryProvider(FGuid Id, TSharedPtr<const IKSISurfaceProvider> Provider);
	bool ObserveCelestials(FKSICelestialObservation& Observation, FString& Error) const;

	const FKSIEcologySimulation& GetEcologySimulation() const { return EcologySimulation; }
	const FKSISurfaceSimulation& GetSurfaceSimulation() const { return SurfaceSimulation; }
	const FKSIEnvironmentInputs& GetEnvironmentInputs() const { return EnvironmentInputs; }
	const IKSISurfaceProvider& GetSurfaceProvider() const { return SurfaceScene; }
	const FKSIWorldClock& GetClock() const { return Clock; }
	const FCCLWorldIdentity& GetIdentity() const { return Identity; }
	const FString& GetLastError() const { return LastError; }
	bool IsRunning() const { return bRunning != 0; }
	static ECCLWorldDomain DomainForWorld(const UWorld* World);

private:
	friend class FCCLTerrainWaterParticipant;
	void Publish();
	bool BuildEnvironmentView(FKSIEnvironmentView& OutView, FString& Error) const;
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

	FKSIWorldClock Clock;
	FKSISurfaceSimulation SurfaceSimulation;
	FKSIEcologySimulation EcologySimulation;
	FKSIEnvironmentInputs EnvironmentInputs;
	FKSICelestialSystem CelestialSystem;
	FKSISurfaceScene SurfaceScene;
	FCCLWorldIdentity Identity;
	FGuid Epoch;
	FString LastError;
	TWeakObjectPtr<UObject> ManualAdvanceOwner;

	// Sorted by world order. Participants are world subsystems that unregister while deinitializing.
	TArray<IKSIWorldParticipant*> Participants;
	TArray<ICCLWorldAdvanceParticipant*> AdvanceParticipants;
	double UnqueuedGameSeconds = 0;
	uint8 bRunning = 0;
};
