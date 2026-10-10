#pragma once

#include "CoreMinimal.h"
#include "CCLSessionRecord.h"
#include "Items/CCLFoodEnvironment.h"

struct FCCLPlayerSnapshot
{
    FString IdentityKey;
    FGuid AccountId;
    int32 Capacity = 16;
    TArray<FCCLSavedItem> Items;
    TArray<FCCLEquippedSlot> EquipmentSlots;
    TArray<FName> Skills;
    int32 Points = 1;
    uint8 Quest = 0;
    float HealthFraction = 1.f;
    float StaminaFraction = 1.f;
    FCCLFoodEnvironmentBinding Environment;
};

namespace CCLPlayerSnapshot
{
constexpr int32 MaximumBytes = 8 * 1024 * 1024;
constexpr int32 MaximumPlayers = 64;
bool Validate(const TArray<FCCLPlayerSnapshot>& Profiles, FGuid WorldId, double WorldSeconds, FString& Error);
bool Encode(const TArray<FCCLPlayerSnapshot>& Profiles, FGuid WorldId, double WorldSeconds, TArray<uint8>& Bytes,
            FString& Error);
bool Decode(const TArray<uint8>& Bytes, FGuid WorldId, double WorldSeconds, TArray<FCCLPlayerSnapshot>& Profiles,
            FString& Error);
} // namespace CCLPlayerSnapshot
