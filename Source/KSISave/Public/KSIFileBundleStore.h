#pragma once

#include "CoreMinimal.h"

struct KSISAVE_API FKSIFileBundle
{
    TArray<uint8> Root;
    TMap<FGuid, TArray<uint8>> Parts;
    uint64 Generation = 0;
    FString Directory;
};

struct KSISAVE_API FKSIFileBundleFormat
{
    uint32 MarkerMagic = 0x4b534942;
    FString RootFileName = TEXT("root.bin");
};

// Synchronous, side-effect-free semantic validation. Generation comes from validated payloads.
using FKSIFileBundleValidator = TFunctionRef<bool(const TArray<uint8>&, const TMap<FGuid, TArray<uint8>>&, uint64&, FString&)>;

// Immutable directories become visible only after their completion marker is published.
class KSISAVE_API FKSIFileBundleStore
{
public:
    static bool Write(const FString& Root, const TArray<uint8>& Payload, const TMap<FGuid, TArray<uint8>>& Parts,
        const FKSIFileBundleFormat& Format, FKSIFileBundleValidator Validate, FString& OutDirectory, FString& Error);
    static bool Recover(const FString& Root, const FKSIFileBundleFormat& Format, FKSIFileBundleValidator Validate,
        FKSIFileBundle& OutBundle, FString& Error);

private:
    static bool ReadDirectory(const FString& Directory, const FKSIFileBundleFormat& Format,
        FKSIFileBundleValidator Validate, FKSIFileBundle& OutBundle, FString& Error);
};
