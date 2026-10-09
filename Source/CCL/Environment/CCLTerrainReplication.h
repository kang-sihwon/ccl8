#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CCLTerrainReplication.generated.h"

class ACCLTerrainRegion;
class UCharacterMovementComponent;

struct FCCLTerrainPeerState
{
	uint64 Serial = 0;
	TArray<uint8> Bytes;
};

UCLASS()
class CCL_API UCCLTerrainReplication : public UActorComponent
{
	GENERATED_BODY()

public:
	UCCLTerrainReplication();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(EEndPlayReason::Type Reason) override;

public:
	UFUNCTION(Client, Reliable)
	void ClientBegin(ACCLTerrainRegion* Region, FGuid Transfer, uint64 Serial, uint64 BaseSerial, int32 Total, int32 Prefix, int32 Suffix, uint32 Checksum);

	UFUNCTION(Client, Reliable)
	void ClientChunk(FGuid Transfer, int32 Offset, const TArray<uint8>& Bytes);

	UFUNCTION(Server, Reliable)
	void ServerAck(FGuid Transfer, int32 Offset, bool bReady);

	bool IsClientReady(ACCLTerrainRegion* Region) const;

private:
	void BeginSend(ACCLTerrainRegion* Region);
	void ResetTransfer();
	void UpdateMovementHold();
	void ReleaseMovementHold();

private:
	friend class UCCLTerrainNetworkSmokeSubsystem;
	uint8 bDropNextReadyAckForTesting = 0;
	TMap<TWeakObjectPtr<ACCLTerrainRegion>, FCCLTerrainPeerState> States;
	TWeakObjectPtr<ACCLTerrainRegion> ActiveRegion;
	TWeakObjectPtr<UCharacterMovementComponent> HeldMovement;
	uint8 PreviousMovementMode = 0;
	uint8 PreviousCustomMode = 0;
	uint8 bMovementHeld = 0;
	FGuid TransferId;
	uint64 TargetSerial = 0;
	TArray<uint8> FullBytes;
	TArray<uint8> Payload;
	int32 PrefixBytes = 0;
	int32 SuffixBytes = 0;
	int32 TotalBytes = 0;
	int32 NextOffset = 0;
	int32 SentOffset = 0;
	uint32 ExpectedCRC = 0;
	double LastProgress = 0.;
	double NextTransferAttempt = 0.;
	uint8 bWaiting = 0;
	uint8 bApplying = 0;
};
