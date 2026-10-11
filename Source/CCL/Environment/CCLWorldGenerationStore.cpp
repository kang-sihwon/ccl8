#include "CCLWorldGenerationStore.h"

#include "KSIFileBundleStore.h"

namespace
{
    FKSIFileBundleFormat Format()
    {
        return {0x43434c47, TEXT("world.bin")};
    }

    bool ValidateWorld(const TArray<uint8>& World, const TMap<FGuid, TArray<uint8>>& Terrain,
        const ECCLWorldDomain* Domain, uint64& Generation, FKSITerrainSaveContext& Context, FString& Error)
    {
        FCCLWorldSnapshot State;
        if (!FCCLWorldSnapshotCodec::Decode(World, State, Error) || (Domain && State.Identity.Domain != *Domain))
        {
            Error = TEXT("Invalid world generation or domain.");
            return false;
        }
        Context = FCCLWorldGenerationStore::ContextFor(State);
        for (const auto& Pair : Terrain)
        {
            FKSITerrainSnapshot Region;
            FKSITerrainSaveContext RegionContext;
            if (!FKSITerrainCodec::Decode(Pair.Value, Region, RegionContext, Error)
                || Region.Definition.RegionId != Pair.Key || !(RegionContext == Context))
            {
                Error = TEXT("Terrain and world generation contexts differ.");
                return false;
            }
        }
        Generation = Context.WorldGeneration;
        return true;
    }
}

FKSITerrainSaveContext FCCLWorldGenerationStore::ContextFor(const FCCLWorldSnapshot& World)
{
	FKSITerrainSaveContext Context;
	Context.WorldId = World.Identity.WorldId;
	Context.BaseWorldVersion = World.Identity.BaseWorldVersion;
	Context.WorldGeneration = World.Identity.Generation;
	Context.GameSeconds = World.Clock.GameSeconds;
	Context.WorldSeconds = World.Clock.WorldSeconds;
	return Context;
}

bool FCCLWorldGenerationStore::Write(const FString& Root, const TArray<uint8>& World,
    const TMap<FGuid, TArray<uint8>>& Terrain, FString& OutDirectory, FString& Error)
{
    return FKSIFileBundleStore::Write(Root, World, Terrain, Format(),
        [](const auto& Payload, const auto& Parts, uint64& Generation, FString& Failure)
        {
            FKSITerrainSaveContext Context;
            return ValidateWorld(Payload, Parts, nullptr, Generation, Context, Failure);
        }, OutDirectory, Error);
}

bool FCCLWorldGenerationStore::Recover(const FString& Root, ECCLWorldDomain Domain,
    FCCLWorldGenerationBundle& OutBundle, FString& Error)
{
    FKSIFileBundle Bundle;
    if (!FKSIFileBundleStore::Recover(Root, Format(),
        [Domain](const auto& Payload, const auto& Parts, uint64& Generation, FString& Failure)
        {
            FKSITerrainSaveContext Context;
            return ValidateWorld(Payload, Parts, &Domain, Generation, Context, Failure);
        }, Bundle, Error))
    {
        return false;
    }
    FCCLWorldGenerationBundle Result;
    uint64 Generation = 0;
    if (!ValidateWorld(Bundle.Root, Bundle.Parts, &Domain, Generation, Result.Context, Error))
    {
        return false;
    }
    Result.World = MoveTemp(Bundle.Root);
    Result.Terrain = MoveTemp(Bundle.Parts);
    Result.Directory = MoveTemp(Bundle.Directory);
    OutBundle = MoveTemp(Result);
    return true;
}
