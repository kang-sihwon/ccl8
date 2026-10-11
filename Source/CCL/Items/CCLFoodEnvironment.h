#pragma once

#include "CoreMinimal.h"
#include "CCLFoodState.h"

class FKSISurfaceSimulation;
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
bool ResolveAt(const FVector& PositionMeters, const FKSISurfaceSimulation& Surface, FCCLFoodEnvironmentBinding& Binding,
			   FString& Error);
bool RelocatePickup(FCCLInventorySnapshot& Record, const FKSISurfaceSimulation& Surface, FString& Error);
bool Validate(const FCCLFoodEnvironmentBinding& Binding, double WorldSeconds, FString& Error);
bool Sample(FCCLFoodEnvironmentBinding& Binding, const FKSISurfaceSimulation& Surface, FString& Error);
bool Advance(FCCLInventorySnapshot& Record, const FKSISurfaceSimulation& SurfaceCandidate, double WorldFromSeconds,
			 FString& Error);
} // namespace CCLFoodEnvironment
