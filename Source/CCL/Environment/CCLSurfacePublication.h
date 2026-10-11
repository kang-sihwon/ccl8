#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "KSISurfaceSimulation.h"
#include "CCLSurfacePublication.generated.h"

struct CCL_API FCCLSurfacePublication
{
	FKSISurfaceSimulation State;
	TArray<uint8> Bytes;
	TMap<FGuid, uint64> TerrainSerials;
	FGuid Epoch;
	uint32 CRC = 0;
};

namespace CCLSurfacePublication
{
	CCL_API bool CollectTerrainSerials(const UWorld* World, const FKSISurfaceSimulation& Surface, TMap<FGuid, uint64>& Result);
}

// Exact-byte deduplication preserves the identity and lifetime of unchanged publications.
class CCL_API FCCLSurfacePublicationCache
{
public:
	TSharedPtr<const FCCLSurfacePublication> Capture(const FKSISurfaceSimulation& Surface, FGuid Epoch,
		const TMap<FGuid, uint64>& TerrainSerials, FString& Error);

private:
	TSharedPtr<const FCCLSurfacePublication> Latest;
};

// First network request in a server frame freezes the publication for that frame.
// Later same-frame world mutations become eligible for the following frame's publication.
UCLASS()
class CCL_API UCCLSurfacePublicationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	TSharedPtr<const FCCLSurfacePublication> Acquire();

private:
	FCCLSurfacePublicationCache Cache;
	TSharedPtr<const FCCLSurfacePublication> FramePublication;
	FGuid FrameEpoch;
	uint64 FrameNumber = MAX_uint64;
};
