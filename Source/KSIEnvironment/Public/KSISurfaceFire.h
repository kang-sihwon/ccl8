#pragma once

#include "CoreMinimal.h"
#include "KSICombustionModel.h"
#include "KSIAirSimulation.h"

struct FKSISurfaceGrid;
struct FKSICelestialObserver;
class IKSISurfaceProvider;

struct KSIENVIRONMENT_API FKSISurfaceThermalContext
{
	const FKSIAirSimulation* PreviousAir = nullptr;
	const IKSISurfaceProvider* Shelter = nullptr;
	const FKSICelestialObserver* Observer = nullptr;
	TMap<FGuid, int32> ContactCounts;
	TMap<FGuid, FKSIAirContents> Sources;

	void AddSource(FGuid SpaceId, double HeatJoules, double VaporKg, double SmokeKg);
	static FVector CellPosition(const FKSISurfaceGrid& Grid, int32 Index);
};

// Empty Cells disables combustion without allocating per-cell fuel state.
struct KSIENVIRONMENT_API FKSISurfaceFireLayer
{
	FKSICombustionMaterial Material;
	double SpreadWattsPerKelvin = 60.;
	double SpreadHeatJoules = 0.;
	double MeltHeatJoules = 0.;
	double DissipatedSurfaceHeatJoules = 0.;
	double VaporizedWaterKg = 0.;
	double EmittedSmokeKg = 0.;
	TArray<FKSICombustionCell> Cells;

	bool Validate(int32 SurfaceCells, FString& Error) const;
	bool Serialize(FArchive& Archive, int32 SurfaceCells, bool bFuelTransfers = true);
};

namespace KSISurfaceFire
{
	// Mutates only the caller's unpublished surface candidate.
	KSIENVIRONMENT_API bool AdvanceCandidate(FKSISurfaceGrid& Grid, double GameSeconds, FString& Error, FKSISurfaceThermalContext* Context = nullptr);
}
