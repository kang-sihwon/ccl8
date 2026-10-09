#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Async/Future.h"
#include "HAL/ThreadSafeBool.h"
#include "CCLTerrainMesher.h"
#include "CCLTerrainStore.h"
#include "CCLTerrainRegion.generated.h"

class UCCLTerrainChunkComponent;

struct FCCLTerrainMeshJobResult
{
	FCCLTerrainMesh Mesh;
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
	virtual bool Prepare(const FCCLTerrainCandidate& Candidate, FString& Error) = 0;
	virtual bool ValidateCommit(const FCCLTerrainCandidate& Candidate, FString& Error) const = 0;
	virtual void Commit(const FCCLTerrainCandidate& Candidate) = 0;
	virtual void Abort(const FCCLTerrainCandidate& Candidate) = 0;
};

UCLASS()
class CCL_API ACCLTerrainRegion : public AActor
{
	GENERATED_BODY()

public:
	ACCLTerrainRegion();
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

public:
	bool InitializeTerrain(const FCCLTerrainDefinition& Definition, FString& Error);
	bool SetEditAuthority(const FCCLTerrainAuthority& Authority, FString& Error);
	ECCLTerrainPrepareResult RequestEdit(const FCCLTerrainEdit& Request, FString& Error);
	bool AddParticipant(TSharedRef<ICCLTerrainEditParticipant> Participant, FString& Error);
	void CancelPendingEdit();

	bool IsTerrainReady() const { return bPublished != 0; }
	bool IsPreparing() const { return bPending != 0; }
	bool DidLastRequestSucceed() const { return bLastSucceeded != 0; }
	const FString& GetLastError() const { return LastError; }
	const FCCLTerrainStore& GetTerrainStore() const { return Store; }
	int32 GetActiveChunkCount() const { return ActiveChunks.Num(); }
	FGuid GetPendingTicket() const { return PendingTicket; }

#if WITH_DEV_AUTOMATION_TESTS
	void RejectNextCookForTesting() { bRejectNextCook = 1; }
#endif

private:
	void StartPreparation(const FCCLTerrainSnapshot& Snapshot, TArray<FIntVector> Chunks);
	void AdvancePreparation();
	void PublishPrepared();
	void FailPending(const FString& Error);
	bool CheckOccupancy(FString& Error) const;
	bool CanMutate() const;

private:
	UPROPERTY()
	TMap<FIntVector, TObjectPtr<UCCLTerrainChunkComponent>> ActiveChunks;

	UPROPERTY()
	TMap<FIntVector, TObjectPtr<UCCLTerrainChunkComponent>> PreparedChunks;

	FCCLTerrainStore Store;
	TMap<FGuid, FCCLTerrainAuthority> Authorities;
	TArray<TSharedRef<ICCLTerrainEditParticipant>> Participants;
	TArray<TSharedRef<ICCLTerrainEditParticipant>> PendingParticipants;
	FCCLTerrainCandidate PendingCandidate;
	FGuid PendingPrincipal;
	FGuid PendingTicket;
	TSharedPtr<const FCCLTerrainSnapshot, ESPMode::ThreadSafe> PendingSnapshot;
	TSharedPtr<FThreadSafeBool, ESPMode::ThreadSafe> Cancellation;
	TArray<FIntVector> PendingChunks;
	TArray<FCCLTerrainMeshJob> MeshJobs;
	FTransform InitialTransform;
	FString LastError;
	double PreparationStarted = 0.;
	int32 NextChunk = 0;
	uint8 bPublished = 0;
	uint8 bPending = 0;
	uint8 bInitialPreparation = 0;
	uint8 bLastSucceeded = 0;
	uint8 bRejectNextCook = 0;
};
