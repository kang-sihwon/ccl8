#pragma once

#include "KSIEcologySpace.h"

class UWorld;

class CCL_API FCCLEcologyWorldSpace final : public IKSIEcologySpace
{
public:
	explicit FCCLEcologyWorldSpace(UWorld* InWorld) : World(InWorld) {}
	virtual bool FindRoute(const FKSIEcologyRouteRequest& Request, const FKSISurfaceSimulation* Surface,
		TArray<FVector>& OutRoute, FString& Error) const override;
	static bool IsInLiquid(const FKSISurfaceSimulation& Surface, const FVector& PositionMeters, double RadiusMeters);

private:
	UWorld* World = nullptr;
};
