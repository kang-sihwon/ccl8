#include "CCLSurfacePublication.h"

#include "CCLWorldSimulationSubsystem.h"
#include "CCLTerrainRegion.h"
#include "EngineUtils.h"
#include "Misc/Crc.h"

namespace CCLSurfacePublication
{
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

TSharedPtr<const FCCLSurfacePublication> FCCLSurfacePublicationCache::Capture(const FCCLSurfaceSimulation& Surface,
	FGuid Epoch, const TMap<FGuid, uint64>& TerrainSerials, FString& Error)
{
	TArray<uint8> Bytes;
	if (!Epoch.IsValid() || !Surface.Capture(Bytes, Error))
	{
		return nullptr;
	}
	if (Latest && Latest->Epoch == Epoch && Latest->Bytes == Bytes
		&& Latest->TerrainSerials.OrderIndependentCompareEqual(TerrainSerials))
	{
		return Latest;
	}
	auto Candidate = MakeShared<FCCLSurfacePublication>();
	Candidate->State = Surface;
	Candidate->Bytes = MoveTemp(Bytes);
	Candidate->TerrainSerials = TerrainSerials;
	Candidate->Epoch = Epoch;
	Candidate->CRC = FCrc::MemCrc32(Candidate->Bytes.GetData(), Candidate->Bytes.Num() - 4);
	Latest = Candidate;
	return Latest;
}

TSharedPtr<const FCCLSurfacePublication> UCCLSurfacePublicationSubsystem::Acquire()
{
	check(IsInGameThread());
	const auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	if (GetWorld()->GetNetMode() == NM_Client || !Runtime || !Runtime->IsRunning())
	{
		return nullptr;
	}
	if (FrameNumber != GFrameCounter || FrameEpoch != Runtime->GetEpoch())
	{
		FrameNumber = GFrameCounter;
		FrameEpoch = Runtime->GetEpoch();
		FramePublication.Reset();
		TMap<FGuid, uint64> TerrainSerials;
		FString Error;
		if (CCLSurfacePublication::CollectTerrainSerials(GetWorld(), Runtime->GetSurfaceSimulation(), TerrainSerials))
		{
			FramePublication = Cache.Capture(Runtime->GetSurfaceSimulation(), FrameEpoch, TerrainSerials, Error);
		}
	}
	return FramePublication;
}
