#pragma once

#include "CoreMinimal.h"
#include "CCLFoodEnvironment.h"

enum class ECCLInventoryOwner : uint8
{
    Container,
    FoodPickup
};

struct FCCLStoredItem
{
    FGuid Id;
    FString Definition;
    int32 Quantity = 0;
    int32 Slot = INDEX_NONE;
    int32 LoadedAmmo = 0;
    FString Food;
};

struct FCCLInventorySnapshot
{
    FGuid ContainerId;
    int32 Capacity = 16;
    TArray<FCCLStoredItem> Items;
    FCCLFoodEnvironmentBinding Environment;
    ECCLInventoryOwner Owner = ECCLInventoryOwner::Container;
    FTransform Transform = FTransform::Identity;
};

namespace CCLInventorySnapshot
{
constexpr int32 MaximumBytes = 8 * 1024 * 1024;
bool Validate(const TArray<FCCLInventorySnapshot>& Records, FGuid WorldId, double WorldSeconds, FString& Error);
bool Encode(const TArray<FCCLInventorySnapshot>& Records, FGuid WorldId, double WorldSeconds, TArray<uint8>& Bytes,
            FString& Error);
bool Decode(const TArray<uint8>& Bytes, FGuid WorldId, double WorldSeconds, TArray<FCCLInventorySnapshot>& Records,
            FString& Error);
} // namespace CCLInventorySnapshot
