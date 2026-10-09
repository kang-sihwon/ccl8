#include "CCLSurfaceReplication.h"

#include "CCLWorldSimulationSubsystem.h"
#include "CCLTerrainRegion.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Misc/Crc.h"

namespace
{
	bool SameSerials(const TMap<FGuid, uint64>& A, const TMap<FGuid, uint64>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}
		for (const auto& Pair : A)
		{
			const auto* Value = B.Find(Pair.Key);
			if (!Value || *Value != Pair.Value)
			{
				return false;
			}
		}
		return true;
	}

	bool CollectTerrainSerials(const UWorld* World, const FCCLSurfaceSimulation& Surface, TMap<FGuid, uint64>& Result)
	{
		for (const auto& Pair : Surface.GetRegions())
		{
			const auto& G = Pair.Value;
			if (!G.TerrainId.IsValid())
			{
				continue;
			}
			for (TActorIterator<ACCLTerrainRegion> It(World); It; ++It)
			{
				if (It->IsTerrainReady() && It->GetTerrainStore().GetSnapshot().Definition.RegionId == G.TerrainId
					&& It->GetTerrainStore().GetSnapshot().Definition.WorldId == Surface.GetWorldId()
					&& It->GetTerrainStore().GetRevision() == G.TerrainRevision)
				{
					Result.Add(G.TerrainId, It->GetPublicationSerial());
					break;
				}
			}
			if (!Result.Contains(G.TerrainId))
			{
				return false;
			}
		}
		return true;
	}
}

UCCLSurfaceReplication::UCCLSurfaceReplication()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.02f;
}

void UCCLSurfaceReplication::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function)
{
	Super::TickComponent(DeltaTime, TickType, Function);
	const auto* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->HasAuthority() || PC->IsLocalController())
	{
		return;
	}
	const double Now = FPlatformTime::Seconds();
	if (TransferId.IsValid() && Now - StartedAt > 15.)
	{
		ClearTransfer();
		bHasAcknowledgedSnapshot = 0;
	}
	if (!TransferId.IsValid() && Now >= NextSendAt)
	{
		const auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
		FString Error;
		TArray<uint8> Bytes;
		NextSendAt = Now + 0.25;
		if (!Runtime || !Runtime->IsRunning() || !Runtime->GetSurfaceSimulation().Capture(Bytes, Error))
		{
			return;
		}
		TMap<FGuid, uint64> TerrainSerials;
		if (!CollectTerrainSerials(GetWorld(), Runtime->GetSurfaceSimulation(), TerrainSerials))
		{
			return;
		}
		const uint32 CRC = FCrc::MemCrc32(Bytes.GetData(), Bytes.Num() - 4);
		if (bHasAcknowledgedSnapshot && CRC == LastCRC && SameSerials(TerrainSerials, AcknowledgedTerrainSerials))
		{
			return;
		}
		if (NextSerial == MAX_uint64)
		{
			return;
		}
		Payload = MoveTemp(Bytes);
		ExpectedCRC = CRC;
		TotalBytes = Payload.Num();
		TransferSerial = ++NextSerial;
		TransferId = FGuid::NewGuid();
		StartedAt = Now;
		TransferTerrainSerials = MoveTemp(TerrainSerials);
		TArray<FGuid> Ids;
		TArray<uint64> Serials;
		for (const auto& Pair : TransferTerrainSerials)
		{
			Ids.Add(Pair.Key);
			Serials.Add(Pair.Value);
		}
		FCCLSurfaceHistoryFrame Frame;
		Frame.Serial = TransferSerial;
		Frame.Epoch = Runtime->GetEpoch();
		Frame.SentAt = Now;
		Frame.State = Runtime->GetSurfaceSimulation();
		History.Add(MoveTemp(Frame));
		if (History.Num() > 4)
		{
			History.RemoveAt(0);
		}
		ClientBegin(TransferId, TransferSerial, TotalBytes, CRC, Ids, Serials);
	}
	if (TransferId.IsValid() && !bWaiting)
	{
		const int32 Count = FMath::Min(8192, TotalBytes - AcknowledgedOffset);
		TArray<uint8> Chunk;
		Chunk.Append(Payload.GetData() + AcknowledgedOffset, Count);
		SentOffset = AcknowledgedOffset + Count;
		bWaiting = 1;
		ClientChunk(TransferId, AcknowledgedOffset, Chunk);
	}
}

void UCCLSurfaceReplication::ClientBegin_Implementation(FGuid Transfer, uint64 Serial, int32 Total, uint32 Checksum, const TArray<FGuid>& TerrainIds, const TArray<uint64>& TerrainSerials)
{
	ClearTransfer();
	if (!Transfer.IsValid() || !Serial || Serial <= PublishedSerial || Total < 56 || Total > FCCLSurfaceSimulation::MaximumBytes
		|| TerrainIds.Num() != TerrainSerials.Num() || TerrainIds.Num() > FCCLSurfaceSimulation::MaximumRegions)
	{
		ServerAck(Transfer, -1);
		return;
	}
	for (int32 I = 0; I < TerrainIds.Num(); ++I)
	{
		if (!TerrainIds[I].IsValid() || !TerrainSerials[I] || TransferTerrainSerials.Contains(TerrainIds[I]))
		{
			ServerAck(Transfer, -1);
			ClearTransfer();
			return;
		}
		TransferTerrainSerials.Add(TerrainIds[I], TerrainSerials[I]);
	}
	TransferId = Transfer;
	TransferSerial = Serial;
	ExpectedCRC = Checksum;
	TotalBytes = Total;
	Payload.Reserve(Total);
}

void UCCLSurfaceReplication::ClientChunk_Implementation(FGuid Transfer, int32 Offset, const TArray<uint8>& Bytes)
{
	if (Transfer != TransferId)
	{
		return;
	}
	if (Offset != Payload.Num() || Bytes.IsEmpty() || Bytes.Num() > 8192 || Bytes.Num() > TotalBytes - Offset)
	{
		ServerAck(Transfer, -1);
		ClearTransfer();
		return;
	}
	Payload.Append(Bytes);
	if (Payload.Num() == TotalBytes)
	{
		FString Error;
		FCCLSurfaceSimulation Candidate;
		if (FCrc::MemCrc32(Payload.GetData(), Payload.Num() - 4) != ExpectedCRC || !Candidate.Restore(Payload, Error))
		{
			ServerAck(Transfer, -1);
			ClearTransfer();
			return;
		}
		TSet<FGuid> Required;
		for (const auto& Pair : Candidate.GetRegions())
		{
			if (Pair.Value.TerrainId.IsValid())
			{
				Required.Add(Pair.Value.TerrainId);
				if (!TransferTerrainSerials.Contains(Pair.Value.TerrainId))
				{
					ServerAck(Transfer, -1);
					ClearTransfer();
					return;
				}
			}
		}
		if (Required.Num() != TransferTerrainSerials.Num())
		{
			ServerAck(Transfer, -1);
			ClearTransfer();
			return;
		}
		Replica = MoveTemp(Candidate);
		ReplicaTerrainSerials = TransferTerrainSerials;
		PublishedSerial = TransferSerial;
		ServerAck(Transfer, Payload.Num());
		ClearTransfer();
		return;
	}
	ServerAck(Transfer, Payload.Num());
}

void UCCLSurfaceReplication::ServerAck_Implementation(FGuid Transfer, int32 Offset)
{
	if (Transfer != TransferId)
	{
		return;
	}
	if (!bWaiting || Offset != SentOffset)
	{
		ClearTransfer();
		bHasAcknowledgedSnapshot = 0;
		return;
	}
	AcknowledgedOffset = Offset;
	StartedAt = FPlatformTime::Seconds();
	bWaiting = 0;
	if (AcknowledgedOffset == TotalBytes)
	{
		LastCRC = ExpectedCRC;
		AcknowledgedTerrainSerials = TransferTerrainSerials;
		bHasAcknowledgedSnapshot = 1;
		PublishedSerial = TransferSerial;
		ClearTransfer();
	}
}

void UCCLSurfaceReplication::ClearTransfer()
{
	TransferId.Invalidate();
	Payload.Reset();
	TransferTerrainSerials.Reset();
	TotalBytes = 0;
	AcknowledgedOffset = 0;
	SentOffset = 0;
	bWaiting = 0;
}

const FCCLSurfaceSimulation* UCCLSurfaceReplication::View(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	if (World->GetNetMode() != NM_Client)
	{
		const auto* Runtime = World->GetSubsystem<UCCLWorldSimulationSubsystem>();
		return Runtime && Runtime->IsRunning() ? &Runtime->GetSurfaceSimulation() : nullptr;
	}
	const auto* PC = World->GetFirstPlayerController();
	const auto* Receiver = PC ? PC->FindComponentByClass<UCCLSurfaceReplication>() : nullptr;
	return Receiver && Receiver->Replica.IsInitialized() ? &Receiver->Replica : nullptr;
}

bool UCCLSurfaceReplication::IsGeometryReady(const UWorld* World, const FCCLSurfaceSimulation& Surface, const FCCLSurfaceGrid& Grid)
{
	if (!Grid.TerrainId.IsValid())
	{
		return true;
	}
	for (TActorIterator<ACCLTerrainRegion> It(World); It; ++It)
	{
		if (!It->IsTerrainReady() || (World->GetNetMode() == NM_Client && !It->IsReplicaReady()))
		{
			continue;
		}
		const auto& Snapshot = It->GetTerrainStore().GetSnapshot();
		if (Snapshot.Definition.RegionId == Grid.TerrainId && Snapshot.Definition.WorldId == Surface.GetWorldId()
			&& Snapshot.Revision == Grid.TerrainRevision)
		{
			if (World->GetNetMode() == NM_Client)
			{
				const auto* PC = World->GetFirstPlayerController();
				const auto* Receiver = PC ? PC->FindComponentByClass<UCCLSurfaceReplication>() : nullptr;
				const auto* Serial = Receiver ? Receiver->ReplicaTerrainSerials.Find(Grid.TerrainId) : nullptr;
				return Serial && *Serial == It->GetReplicaSerial() && *Serial == It->GetPublicationSerial();
			}
			return true;
		}
	}
	return false;
}

bool UCCLSurfaceReplication::HasCurrentState(const FCCLSurfaceSimulation& Surface) const
{
	TArray<uint8> Bytes;
	FString Error;
	TMap<FGuid, uint64> Serials;
	return bHasAcknowledgedSnapshot && CollectTerrainSerials(GetWorld(), Surface, Serials)
		&& SameSerials(Serials, AcknowledgedTerrainSerials) && Surface.Capture(Bytes, Error)
		&& LastCRC == FCrc::MemCrc32(Bytes.GetData(), Bytes.Num() - 4);
}

bool UCCLSurfaceReplication::SampleHistoricalSnow(uint64 Serial, const FVector& Position, FCCLSnowSample& Sample) const
{
	const auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	if (!Runtime)
	{
		return false;
	}
	for (const auto& Frame : History)
	{
		if (Frame.Serial == Serial && Frame.Epoch == Runtime->GetEpoch() && FPlatformTime::Seconds() - Frame.SentAt <= 15.)
		{
			Frame.State.SampleSnow(Position, Sample);
			return true;
		}
	}
	return false;
}

void UCCLSurfaceReplication::PredictSnowContact(FGuid SourceId, uint64 Sequence, const FVector& Position)
{
	const double Now = FPlatformTime::Seconds();
	PredictedContacts.RemoveAll([&](const FCCLPredictedSnowContact& C)
	{
		return Now - C.CreatedAt > 2. || Replica.LastSnowContact(C.SourceId) >= C.Sequence;
	});
	if (!SourceId.IsValid() || Sequence == 0 || PredictedContacts.ContainsByPredicate([&](const FCCLPredictedSnowContact& C)
		{ return C.SourceId == SourceId && C.Sequence == Sequence; }))
	{
		return;
	}
	if (PredictedContacts.Num() >= 64)
	{
		PredictedContacts.RemoveAt(0);
	}
	PredictedContacts.Add({SourceId, Sequence, Position, Now});
}

double UCCLSurfaceReplication::PredictedSnowScale(const FVector& Position) const
{
	for (const auto& C : PredictedContacts)
	{
		if (FPlatformTime::Seconds() - C.CreatedAt <= 2. && Replica.LastSnowContact(C.SourceId) < C.Sequence
			&& FVector::DistSquared2D(C.PositionMeters, Position) < FMath::Square(0.27))
		{
			return 0.5;
		}
	}
	return 1.;
}
