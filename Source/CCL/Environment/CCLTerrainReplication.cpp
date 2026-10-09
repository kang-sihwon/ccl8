#include "CCLTerrainReplication.h"

#include "CCLTerrainRegion.h"
#include "EngineUtils.h"
#include "Engine/NetConnection.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/Crc.h"

UCCLTerrainReplication::UCCLTerrainReplication()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	PrimaryComponentTick.TickInterval = 0.02f;
}

void UCCLTerrainReplication::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	auto* PC = Cast<APlayerController>(GetOwner());
	if (!PC || (PC->HasAuthority() && PC->IsLocalController()))
	{
		return;
	}

	UpdateMovementHold();
	for (auto It = States.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid()) { It.RemoveCurrent(); }
	}

	if (TransferId.IsValid() && (!ActiveRegion.IsValid() || FPlatformTime::Seconds() - LastProgress > 60.))
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("CCLTerrainNetworkSmoke")))
		{
			UE_LOG(LogTemp, Display, TEXT("CCL_TERRAIN_NETWORK TRANSFER_TIMEOUT Serial=%llu"), TargetSerial);
		}

		ResetTransfer();
	}

	if (!PC->HasAuthority())
	{
		if (bApplying && ActiveRegion.IsValid())
		{
			if (ActiveRegion->GetReplicaSerial() == TargetSerial && ActiveRegion->IsReplicaReady())
			{
				auto& State = States.FindOrAdd(ActiveRegion);
				State.Serial = TargetSerial;
				State.Bytes = MoveTemp(FullBytes);
				ServerAck(TransferId, NextOffset, true);
				ResetTransfer();
			}
			else if (!ActiveRegion->IsPreparing() && ActiveRegion->GetReplicaSerial() != TargetSerial)
			{
                if (FParse::Param(FCommandLine::Get(), TEXT("CCLTerrainNetworkSmoke")))
                {
                    UE_LOG(LogTemp,Display,TEXT("CCL_TERRAIN_RECEIVE prepare failed %s"),*ActiveRegion->GetLastError());
                }
				ServerAck(TransferId, -1, false);
				ResetTransfer();
			}
		}

		return;
	}

	if (ActiveRegion.IsValid() && ActiveRegion->GetPublicationSerial() != TargetSerial)
	{
		ResetTransfer();
	}

	if (!TransferId.IsValid() && FPlatformTime::Seconds() >= NextTransferAttempt)
	{
		for (TActorIterator<ACCLTerrainRegion> It(GetWorld()); It; ++It)
		{
			if (It->IsTerrainReady() && States.FindOrAdd(*It).Serial != It->GetPublicationSerial())
			{
				BeginSend(*It);
				break;
			}
		}
	}

	if (TransferId.IsValid() && !bWaiting && FPlatformTime::Seconds() >= NextChunkAt)
	{
		// Each of the two bulk streams uses at most 20% of the connection budget.
		// ACK pacing alone saturates loopback and can starve initial GameState replication.
		const auto* Connection = PC->GetNetConnection();
		const double BytesPerSecond = FMath::Max(1, Connection ? Connection->CurrentNetSpeed : 10000) * 0.2;
		const int32 Count = FMath::Min3(8192, Payload.Num() - NextOffset, FMath::Max(1, FMath::FloorToInt32(BytesPerSecond * 0.1)));
		NextChunkAt = FPlatformTime::Seconds() + (Count + 128.) / BytesPerSecond;
		TArray<uint8> Chunk;
		Chunk.Append(Payload.GetData() + NextOffset, Count);
		SentOffset = NextOffset + Count;
		bWaiting = 1;
		ClientChunk(TransferId, NextOffset, Chunk);
	}
}

void UCCLTerrainReplication::BeginSend(ACCLTerrainRegion* Region)
{
	FString Error;
	if (!Region->CaptureReplica(FullBytes, Error))
	{
		return;
	}

	ActiveRegion = Region;
	Region->ForceNetUpdate();
	TargetSerial = Region->GetPublicationSerial();
	TransferId = FGuid::NewGuid();
	TotalBytes = FullBytes.Num();
	const auto& Base = States.FindOrAdd(Region);
	PrefixBytes = 0;
	SuffixBytes = 0;
	while (PrefixBytes < FMath::Min(Base.Bytes.Num(), FullBytes.Num()) && Base.Bytes[PrefixBytes] == FullBytes[PrefixBytes])
	{
		++PrefixBytes;
	}

	while (SuffixBytes < FMath::Min(Base.Bytes.Num(), FullBytes.Num()) - PrefixBytes
		&& Base.Bytes[Base.Bytes.Num() - SuffixBytes - 1] == FullBytes[FullBytes.Num() - SuffixBytes - 1])
	{
		++SuffixBytes;
	}

	if (FParse::Param(FCommandLine::Get(), TEXT("CCLTerrainNetworkSmoke")))
	{
		UE_LOG(LogTemp, Display, TEXT("CCL_TERRAIN_TRANSFER Serial=%llu Base=%llu Total=%d Prefix=%d Suffix=%d"), TargetSerial, Base.Serial, TotalBytes, PrefixBytes, SuffixBytes);
	}

	Payload.Append(FullBytes.GetData() + PrefixBytes, TotalBytes - PrefixBytes - SuffixBytes);
	LastProgress = FPlatformTime::Seconds();
	ClientBegin(Region, TransferId, TargetSerial, Base.Serial, TotalBytes, PrefixBytes, SuffixBytes,
		FCrc::MemCrc32(FullBytes.GetData(), FullBytes.Num()));
}

void UCCLTerrainReplication::ClientBegin_Implementation(ACCLTerrainRegion* Region, FGuid Transfer, uint64 Serial,
	uint64 BaseSerial, int32 Total, int32 Prefix, int32 Suffix, uint32 Checksum)
{
	ResetTransfer();
	const auto* Base = States.Find(Region);
	if (!Region || !Transfer.IsValid() || Total < 1 || Total > FCCLTerrainCodec::MaximumBytes || Prefix < 0 || Suffix < 0
		|| int64(Prefix) + Suffix > Total || ((Prefix || Suffix) && (!Base || Base->Serial != BaseSerial || int64(Prefix) + Suffix > Base->Bytes.Num())))
	{
        if (FParse::Param(FCommandLine::Get(), TEXT("CCLTerrainNetworkSmoke")))
        {
            UE_LOG(LogTemp,Display,TEXT("CCL_TERRAIN_RECEIVE begin rejected region=%s serial=%llu total=%d prefix=%d suffix=%d"),*GetNameSafe(Region),Serial,Total,Prefix,Suffix);
        }
		ServerAck(Transfer, -1, false);
		return;
	}

	ActiveRegion = Region;
	TransferId = Transfer;
	TargetSerial = Serial;
	TotalBytes = Total;
	PrefixBytes = Prefix;
	SuffixBytes = Suffix;
	ExpectedCRC = Checksum;
	LastProgress = FPlatformTime::Seconds();
}

void UCCLTerrainReplication::ClientChunk_Implementation(FGuid Transfer, int32 Offset, const TArray<uint8>& Bytes)
{
	if (Transfer != TransferId || !ActiveRegion.IsValid())
	{
		return;
	}

	const int32 ExpectedPayload = TotalBytes - PrefixBytes - SuffixBytes;
	if (bApplying || Offset != Payload.Num() || Bytes.Num() > 8192 || Bytes.Num() > ExpectedPayload - Offset)
	{
		ServerAck(Transfer, -1, false);
		ResetTransfer();
		return;
	}

	Payload.Append(Bytes);
	NextOffset = Payload.Num();
	LastProgress = FPlatformTime::Seconds();
	if (NextOffset < ExpectedPayload)
	{
		ServerAck(Transfer, NextOffset, false);
		return;
	}

	const auto* Base = States.Find(ActiveRegion);
	if (PrefixBytes)
	{
		FullBytes.Append(Base->Bytes.GetData(), PrefixBytes);
	}

	FullBytes.Append(Payload);
	if (SuffixBytes)
	{
		FullBytes.Append(Base->Bytes.GetData() + Base->Bytes.Num() - SuffixBytes, SuffixBytes);
	}

	FString Error;
	if (FullBytes.Num() != TotalBytes || FCrc::MemCrc32(FullBytes.GetData(), FullBytes.Num()) != ExpectedCRC
		|| !ActiveRegion->ApplyReplica(FullBytes, TargetSerial, Error))
	{
        if (FParse::Param(FCommandLine::Get(), TEXT("CCLTerrainNetworkSmoke")))
        {
            UE_LOG(LogTemp,Display,TEXT("CCL_TERRAIN_RECEIVE apply rejected serial=%llu published=%llu replica=%llu bytes=%d expected=%d error=%s"),TargetSerial,ActiveRegion->GetPublicationSerial(),ActiveRegion->GetReplicaSerial(),FullBytes.Num(),TotalBytes,*Error);
        }
		ServerAck(Transfer, -1, false);
		ResetTransfer();
		return;
	}

	bApplying = 1;
}

void UCCLTerrainReplication::ServerAck_Implementation(FGuid Transfer, int32 Offset, bool bReady)
{
	if (Transfer != TransferId || !ActiveRegion.IsValid())
	{
		return;
	}

	if (Offset == -1)
	{
        // An RPC reference can arrive before the region actor. Leave replication bandwidth
        // for actor creation instead of immediately resending the entire snapshot.
        NextTransferAttempt = FPlatformTime::Seconds() + 0.5;
		States.Remove(ActiveRegion);
		ResetTransfer();
		return;
	}

	if (!bWaiting || Offset != SentOffset || (bReady && Offset != Payload.Num()))
	{
		return;
	}

#if WITH_DEV_AUTOMATION_TESTS
	if (bReady && bDropNextReadyAckForTesting)
	{
		bDropNextReadyAckForTesting = 0;
		// Inject an ACK that arrives after the transfer deadline without a minute-long test stall.
		LastProgress = FPlatformTime::Seconds() - 61.;
		UE_LOG(LogTemp, Display, TEXT("CCL_TERRAIN_NETWORK DROPPED_READY_ACK Serial=%llu"), TargetSerial);
		return;
	}
#endif

	LastProgress = FPlatformTime::Seconds();
	NextOffset = Offset;
	bWaiting = 0;
	if (bReady)
	{
		auto& State = States.FindOrAdd(ActiveRegion);
		State.Serial = TargetSerial;
		State.Bytes = MoveTemp(FullBytes);
		ResetTransfer();
	}
}

bool UCCLTerrainReplication::IsClientReady(ACCLTerrainRegion* Region) const
{
	if (!Region)
	{
		return false;
	}

	const auto* PC = Cast<APlayerController>(GetOwner());
	if (PC && PC->HasAuthority() && PC->IsLocalController())
	{
		return Region->IsTerrainReady();
	}

	const auto* State = States.Find(Region);
	return State && State->Serial == Region->GetPublicationSerial();
}

void UCCLTerrainReplication::ResetTransfer()
{
	ActiveRegion.Reset();
	TransferId.Invalidate();
	TargetSerial = 0;
	FullBytes.Reset();
	Payload.Reset();
	NextOffset = 0;
	SentOffset = 0;
	PrefixBytes = 0;
	SuffixBytes = 0;
	bWaiting = 0;
	bApplying = 0;
}

void UCCLTerrainReplication::EndPlay(EEndPlayReason::Type Reason)
{
    ReleaseMovementHold();
    Super::EndPlay(Reason);
}

void UCCLTerrainReplication::UpdateMovementHold()
{
    auto* PC = Cast<APlayerController>(GetOwner());
    auto* Character = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
    auto* Movement = Character ? Character->GetCharacterMovement() : nullptr;
    bool bHold = false;
    if (Movement)
    {
        const FVector Position = Character->GetActorLocation();
        for (TActorIterator<ACCLTerrainRegion> It(GetWorld()); It; ++It)
        {
            const bool bReady = PC->HasAuthority() ? IsClientReady(*It) : It->IsReplicaReady();
            if (!bReady && It->GetWorldTerrainBounds().IsValid
                && It->GetWorldTerrainBounds().ExpandBy(200.).IsInsideOrOn(Position))
            {
                bHold = true;
                break;
            }
        }
    }
    if (bMovementHeld && (!bHold || HeldMovement.Get() != Movement)) { ReleaseMovementHold(); }
    if (bHold && !bMovementHeld)
    {
        HeldMovement = Movement;
        bMovementHeld = 1;
        PreviousMovementMode = uint8(Movement->MovementMode);
        PreviousCustomMode = Movement->CustomMovementMode;
        Movement->StopMovementImmediately();
        Movement->DisableMovement();
        PC->SetIgnoreMoveInput(true);
    }
}

void UCCLTerrainReplication::ReleaseMovementHold()
{
    if (HeldMovement.IsValid() && HeldMovement->MovementMode == MOVE_None)
    {
        HeldMovement->SetMovementMode(EMovementMode(PreviousMovementMode), PreviousCustomMode);
    }
    if (bMovementHeld)
    {
        if (auto* PC = Cast<APlayerController>(GetOwner())) { PC->SetIgnoreMoveInput(false); }
    }
    HeldMovement.Reset();
    bMovementHeld = 0;
}
