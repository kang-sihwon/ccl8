#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KSISurfaceSimulation.h"
#include "CCLSurfaceReplication.generated.h"

struct FCCLSurfacePublication;

struct FCCLSurfaceHistoryFrame
{
	uint64 Serial = 0;
	double SentAt = 0.;
	TSharedPtr<const FCCLSurfacePublication> Publication;
};

struct FCCLPredictedSnowContact
{
	FGuid SourceId;
	uint64 Sequence = 0;
	FVector PositionMeters = FVector::ZeroVector;
	double CreatedAt = 0.;
};

// Each player receives bounded, acknowledged chunks of one immutable surface publication.
UCLASS()
class CCL_API UCCLSurfaceReplication : public UActorComponent
{
	GENERATED_BODY()

public:
	UCCLSurfaceReplication();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function) override;

	UFUNCTION(Client, Reliable)
	void ClientBegin(FGuid Transfer, uint64 Serial, int32 Total, uint32 Checksum, const TArray<FGuid>& TerrainIds, const TArray<uint64>& TerrainSerials);
	UFUNCTION(Client, Reliable)
	void ClientChunk(FGuid Transfer, int32 Offset, const TArray<uint8>& Bytes);
	UFUNCTION(Server, Reliable)
	void ServerAck(FGuid Transfer, int32 Offset);

	bool SampleHistoricalSnow(uint64 Serial, const FVector& PositionMeters, FKSISnowSample& Sample) const;
	void PredictSnowContact(FGuid SourceId, uint64 Sequence, const FVector& PositionMeters);
	double PredictedSnowScale(const FVector& PositionMeters) const;
	bool HasCurrentState(const FKSISurfaceSimulation& Surface) const;
	const FKSISurfaceSimulation& GetReplica() const { return Replica; }
	uint64 GetSerial() const { return PublishedSerial; }
	static const FKSISurfaceSimulation* View(const UWorld* World);
	static bool IsGeometryReady(const UWorld* World, const FKSISurfaceSimulation& Surface, const FKSISurfaceGrid& Grid);

private:
	void ClearTransfer();

private:
	FKSISurfaceSimulation Replica;
	TArray<FCCLSurfaceHistoryFrame> History;
	TArray<FCCLPredictedSnowContact> PredictedContacts;
	TMap<FGuid, uint64> TransferTerrainSerials;
	TMap<FGuid, uint64> AcknowledgedTerrainSerials;
	TMap<FGuid, uint64> ReplicaTerrainSerials;
	TArray<uint8> Payload;
	TSharedPtr<const FCCLSurfacePublication> TransferPublication;
	FGuid AcknowledgedEpoch;
	FGuid TransferId;
	uint64 TransferSerial = 0;
	uint64 PublishedSerial = 0;
	uint64 NextSerial = 0;
	uint32 ExpectedCRC = 0;
	uint32 LastCRC = 0;
	int32 TotalBytes = 0;
	int32 SentOffset = 0;
	int32 AcknowledgedOffset = 0;
	double StartedAt = 0.;
	double NextSendAt = 0.;
	double NextChunkAt = 0.;
	uint8 bWaiting = 0;
	uint8 bHasAcknowledgedSnapshot = 0;
};
