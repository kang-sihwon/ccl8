#pragma once

#include "CoreMinimal.h"
#include "CCLEcologySimulation.h"

class UWorld;
class FCCLSurfaceSimulation;

struct FCCLEcologyRouteRequest
{
    ECCLEcologyMovement Medium = ECCLEcologyMovement::Ground;
    FVector FromMeters = FVector::ZeroVector;
    FVector TargetMeters = FVector::ZeroVector;
    FBox HabitatMeters = FBox(EForceInit::ForceInit);
    double RadiusMeters = .2;
    double WorldSeconds = 0.;
};

class CCL_API ICCLEcologySpace
{
public:
    virtual ~ICCLEcologySpace() = default;
    // True + empty route means no traversable route. False means the query is not ready/valid.
    virtual bool FindRoute(const FCCLEcologyRouteRequest& Request, const FCCLSurfaceSimulation* Surface,
        TArray<FVector>& OutRoute, FString& Error) const = 0;
};

class CCL_API FCCLEcologyWorldSpace final : public ICCLEcologySpace
{
public:
    explicit FCCLEcologyWorldSpace(UWorld* InWorld) : World(InWorld) {}
    virtual bool FindRoute(const FCCLEcologyRouteRequest& Request, const FCCLSurfaceSimulation* Surface,
        TArray<FVector>& OutRoute, FString& Error) const override;
    static bool IsInLiquid(const FCCLSurfaceSimulation& Surface, const FVector& PositionMeters, double RadiusMeters);

private:
    UWorld* World = nullptr;
};
