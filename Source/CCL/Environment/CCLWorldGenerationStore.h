#pragma once

#include "CoreMinimal.h"
#include "CCLWorldSnapshot.h"
#include "KSITerrainStore.h"

struct CCL_API FCCLWorldGenerationBundle
{
	TArray<uint8> World;
	TMap<FGuid, TArray<uint8>> Terrain;
	FKSITerrainSaveContext Context;
	FString Directory;
};

// Each immutable directory is published by a final completion marker. Older complete directories survive failures.
class CCL_API FCCLWorldGenerationStore
{
public:
	static bool Write(const FString& Root, const TArray<uint8>& World, const TMap<FGuid, TArray<uint8>>& Terrain,
		FString& OutDirectory, FString& Error);
	static bool Recover(const FString& Root, ECCLWorldDomain Domain, FCCLWorldGenerationBundle& OutBundle, FString& Error);
	static FKSITerrainSaveContext ContextFor(const FCCLWorldSnapshot& World);

};
