#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Async/Future.h"
#include "HAL/ThreadSafeBool.h"
#include "KSITerrainMesher.h"
#include "KSITerrainStore.h"
#include "CCLTerrainRegion.generated.h"

class UKSITerrainChunkComponent;
class FKSITerrainSurface;
class IKSISurfaceProvider;
class ANavigationData;
struct FCCLWorldSnapshot;

struct FCCLTerrainMeshJobResult
{
	FKSITerrainMesh Mesh;
	FString Error;
	uint8 bSucceeded = 0;
};

struct FCCLTerrainMeshJob
{
	TFuture<FCCLTerrainMeshJobResult> Future;
};

// Prepare may allocate. Commit is a non-failing swap of already prepared state on the game thread.
class CCL_API ICCLTerrainEditParticipant
{
public:
	virtual ~ICCLTerrainEditParticipant() = default;
	virtual bool Prepare(const FKSITerrainCandidate& Candidate, FString& Error) = 0;
	virtual bool ValidateCommit(const FKSITerrainCandidate& Candidate, FString& Error) const = 0;
	virtual void Commit(const FKSITerrainCandidate& Candidate) = 0;
	virtual void Abort(const FKSITerrainCandidate& Candidate) = 0;
	virtual bool ValidateRestore(const FKSITerrainSnapshot& Terrain, const FCCLWorldSnapshot* World, FString& Error) const
	{
		Error = TEXT("Terrain participant does not support coordinated restore.");
		return false;
	}
	virtual void CommitRestore() {}
};

UCLASS()
class CCL_API ACCLTerrainRegion : public AActor
{
	GENERATED_BODY()

public:
	ACCLTerrainRegion();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

public:
	bool InitializeTerrain(const FKSITerrainDefinition& Definition, FString& Error, const TArray<uint8>& SavedBytes = {});
	bool SetEditAuthority(const FKSITerrainAuthority& Authority, FString& Error);
	EKSITerrainPrepareResult RequestEdit(const FKSITerrainEdit& Request, FString& Error);
	bool ApplyReplica(const TArray<uint8>& Bytes, uint64 Serial, FString& Error);
	bool CaptureReplica(TArray<uint8>& Bytes, FString& Error) const;
	uint64 GetPublicationSerial() const { return PublicationSerial; }
	uint64 GetReplicaSerial() const { return ReplicaSerial; }
	bool IsReplicaReady() const { return bPublished && !bPending && ReplicaSerial >= PublicationSerial && GFrameCounter > NavigationPublicationFrame + 1; }

	bool RequestRestore(const TArray<uint8>& Bytes, const FKSITerrainSaveContext& Context, FString& Error, TFunction<bool(FString&)> BeforePublish = {},
		const FCCLWorldSnapshot* WorldState = nullptr);
	bool AddParticipant(TSharedRef<ICCLTerrainEditParticipant> Participant, FString& Error);
	void CancelPendingEdit();

	const IKSISurfaceProvider* GetSurfaceProvider() const;

	bool IsNavigationReady() const { return bPublished && NavigationRevision == Store.GetRevision() && NavigationEpoch == Store.GetEpoch(); }
	FBox GetWorldTerrainBounds() const;

	bool IsTerrainReady() const { return bPublished != 0; }
	bool IsPreparing() const { return bPending != 0; }
	bool DidLastRequestSucceed() const { return bLastSucceeded != 0; }
	const FString& GetLastError() const { return LastError; }
	const FKSITerrainStore& GetTerrainStore() const { return Store; }
	int32 GetActiveChunkCount() const { return ActiveChunks.Num(); }
	FGuid GetPendingTicket() const { return PendingTicket; }

#if WITH_DEV_AUTOMATION_TESTS
	void RejectNextCookForTesting() { bRejectNextCook = 1; }
#endif

private:
	UFUNCTION()
	void OnRep_Publication();

	UFUNCTION()
	void NavigationFinished(ANavigationData* NavData);

	void UpdateNavigationReadiness();
	void StartPreparation(const FKSITerrainSnapshot& Snapshot, TArray<FIntVector> Chunks);
	void AdvancePreparation();
	void PublishPrepared();
	void FailPending(const FString& Error);
	bool CheckOccupancy(FString& Error) const;
	bool CanMutate() const;

private:
	UPROPERTY()
	TMap<FIntVector, TObjectPtr<UKSITerrainChunkComponent>> ActiveChunks;

	UPROPERTY()
	TMap<FIntVector, TObjectPtr<UKSITerrainChunkComponent>> PreparedChunks;

	UPROPERTY(ReplicatedUsing = OnRep_Publication)
	uint64 PublicationSerial = 0;

	uint64 ReplicaSerial = 0;
	uint64 PendingReplicaSerial = 0;
	TSharedPtr<const FKSITerrainSurface> SurfaceProvider;
	TMap<FIntVector, TSharedRef<const FKSITerrainMesh>> SurfaceMeshes;
	FKSITerrainStore Store;
	FKSITerrainStore RestoreStore;
	TFunction<bool(FString&)> RestoreBeforePublish;
	TSharedPtr<const FCCLWorldSnapshot> RestoreWorldState;
	FGuid PreparationEpoch;
	TMap<FGuid, FKSITerrainAuthority> Authorities;
	TArray<TSharedRef<ICCLTerrainEditParticipant>> Participants;
	TArray<TSharedRef<ICCLTerrainEditParticipant>> PendingParticipants;
	FKSITerrainCandidate PendingCandidate;
	FGuid PendingPrincipal;
	FGuid PendingTicket;
	TSharedPtr<const FKSITerrainSnapshot, ESPMode::ThreadSafe> PendingSnapshot;
	TSharedPtr<FThreadSafeBool, ESPMode::ThreadSafe> Cancellation;
	TArray<FIntVector> PendingChunks;
	TArray<FCCLTerrainMeshJob> MeshJobs;
	FTransform InitialTransform;
	FString LastError;
	double PreparationStarted = 0.;
	int32 NextChunk = 0;
	uint64 NavigationRevision = 0;
	uint64 NavigationPublicationFrame = 0;
	FGuid NavigationEpoch;
	uint8 bNavigationObserved = 0;
	uint8 bRestoring = 0;
	uint8 bPublished = 0;
	uint8 bPending = 0;
	uint8 bInitialPreparation = 0;
	uint8 bLastSucceeded = 0;
	uint8 bRejectNextCook = 0;
};
