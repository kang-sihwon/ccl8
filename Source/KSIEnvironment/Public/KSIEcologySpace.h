#pragma once

#include "CoreMinimal.h"
#include "KSIEcologySimulation.h"

class FKSISurfaceSimulation;

struct KSIENVIRONMENT_API FKSIEcologyRouteRequest
{
	EKSIEcologyMovement Medium = EKSIEcologyMovement::Ground;
	FVector FromMeters = FVector::ZeroVector;
	FVector TargetMeters = FVector::ZeroVector;
	FBox HabitatMeters = FBox(EForceInit::ForceInit);
	double RadiusMeters = .2;
	double WorldSeconds = 0.;
};

class KSIENVIRONMENT_API IKSIEcologySpace
{
public:
	virtual ~IKSIEcologySpace() = default;
	// True + empty route means no traversable route. False means the query is not ready/valid.
	virtual bool FindRoute(const FKSIEcologyRouteRequest& Request, const FKSISurfaceSimulation* Surface,
		TArray<FVector>& OutRoute, FString& Error) const = 0;
};
