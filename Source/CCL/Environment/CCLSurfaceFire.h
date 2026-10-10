#pragma once

#include "CoreMinimal.h"
#include "CCLCombustionModel.h"
#include "CCLAirSimulation.h"

struct FCCLSurfaceGrid;
struct FCCLCelestialObserver;
class ICCLSurfaceProvider;

struct FCCLSurfaceThermalContext
{
	const FCCLAirSimulation* PreviousAir = nullptr;
	const ICCLSurfaceProvider* Shelter = nullptr;
	const FCCLCelestialObserver* Observer = nullptr;
	TMap<FGuid, int32> ContactCounts;
	TMap<FGuid, FCCLAirContents> Sources;

	void AddSource(FGuid SpaceId, double HeatJoules, double VaporKg, double SmokeKg);
	static FVector CellPosition(const FCCLSurfaceGrid& Grid, int32 Index);
};

// Empty Cells disables combustion without allocating per-cell fuel state.
struct FCCLSurfaceFireLayer
{
	FCCLCombustionMaterial Material;
	double SpreadWattsPerKelvin = 60.;
	double SpreadHeatJoules = 0.;
	double MeltHeatJoules = 0.;
	double DissipatedSurfaceHeatJoules = 0.;
	double VaporizedWaterKg = 0.;
	double EmittedSmokeKg = 0.;
	TArray<FCCLCombustionCell> Cells;

	bool Validate(int32 SurfaceCells, FString& Error) const;
	bool Serialize(FArchive& Archive, int32 SurfaceCells, bool bFuelTransfers = true);
};

namespace CCLSurfaceFire
{
	// Mutates only the caller's unpublished surface candidate.
	bool AdvanceCandidate(FCCLSurfaceGrid& Grid, double GameSeconds, FString& Error, FCCLSurfaceThermalContext* Context = nullptr);
}
