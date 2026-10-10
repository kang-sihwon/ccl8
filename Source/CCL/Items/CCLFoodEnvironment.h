#pragma once

#include "CoreMinimal.h"
#include "CCLFoodState.h"

class FCCLSurfaceSimulation;
struct FCCLInventorySnapshot;

enum class ECCLFoodEnvironment : uint8
{
    Fixed,
    WeatherRegion,
    AirSpace
};

struct FCCLFoodEnvironmentBinding
{
    ECCLFoodEnvironment Source = ECCLFoodEnvironment::Fixed;
    FGuid SourceId;
    FCCLFoodStorage Conditions;
    double SampleWorldSeconds = 0.;

    bool IsDynamic() const { return Source != ECCLFoodEnvironment::Fixed; }
};

namespace CCLFoodEnvironment
{
bool ResolveAt(const FVector& PositionMeters, const FCCLSurfaceSimulation& Surface, FCCLFoodEnvironmentBinding& Binding,
               FString& Error);
bool RelocatePickup(FCCLInventorySnapshot& Record, const FCCLSurfaceSimulation& Surface, FString& Error);
bool Validate(const FCCLFoodEnvironmentBinding& Binding, double WorldSeconds, FString& Error);
bool Sample(FCCLFoodEnvironmentBinding& Binding, const FCCLSurfaceSimulation& Surface, FString& Error);
bool Advance(FCCLInventorySnapshot& Record, const FCCLSurfaceSimulation& SurfaceCandidate, double WorldFromSeconds,
             FString& Error);
} // namespace CCLFoodEnvironment
