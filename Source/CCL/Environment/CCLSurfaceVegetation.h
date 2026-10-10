#pragma once

#include "CoreMinimal.h"

struct FCCLSurfaceGrid;
struct FCCLSurfaceThermalContext;

enum class ECCLVegetationKind : uint8
{
	Tree, Grass, Flower, Shrub, Moss, Fern, Algae, AquaticPlant, Fungus
};

enum class ECCLVegetationState : uint8
{
	Empty, Growing, Mature, Dormant, Dead
};

struct FCCLVegetationSpecies
{
	FGuid Id;
	ECCLVegetationKind Kind = ECCLVegetationKind::Grass;
	double MaximumBiomassKg = 2.;
	double GrowthKgPerWorldSecond = 0.00001;
	double WaterCubicMetersPerKg = 0.0005;
	double NutrientKgPerBiomassKg = 0.02;
	double MinimumWetness = 0.05;
	double OptimalWetness = 0.6;
	double FullGrowthIrradiance = 500.;
	double MinimumDaylightHours = 6.;
	double MinimumTemperatureCelsius = 2.;
	double MaximumTemperatureCelsius = 40.;
	double LethalTemperatureCelsius = 65.;
	double RootKillTemperatureCelsius = 400.;
	double StressDeathSeconds = 864000.;
	double LifespanSeconds = 31536000.;
	double RegrowthDelaySeconds = 86400.;
	double DecayPerWorldSecond = 0.000001;
	double MaximumSupportChangeMeters = 0.25;
	uint8 bRegrows = 1;
};

struct FCCLVegetationSeed
{
	FGuid Id;
	int32 SpeciesIndex = INDEX_NONE;
	double BiomassKg = 0.;
	double SoilNutrientsKg = 0.;
	double SoilOrganicKg = 0.;
};

struct FCCLVegetationCell
{
	FGuid Id;
	int32 SpeciesIndex = INDEX_NONE;
	double InitialBiomassKg = 0.;
	double LivingBiomassKg = 0.;
	double DeadBiomassKg = 0.;
	double GrownBiomassKg = 0.;
	double BurnedBiomassKg = 0.;
	double DecayedBiomassKg = 0.;
	double SoilNutrientsKg = 0.;
	double InitialNutrientsKg = 0.;
	double SoilOrganicKg = 0.;
	double InitialOrganicKg = 0.;
	double FungalConsumptionKg = 0.;
	double TranspiredCubicMeters = 0.;
	double CycleStartedWorldSeconds = 0.;
	double DeathWorldSeconds = -1.;
	double StressWorldSeconds = 0.;
	uint32 RegrowthCount = 0;
	ECCLVegetationState State = ECCLVegetationState::Empty;
	uint8 bRootsViable = 0;
};

// Optional layer: one persistent species patch per surface cell, not per visual stem.
struct FCCLSurfaceVegetationLayer
{
	TArray<FCCLVegetationSpecies> Species;
	TArray<FCCLVegetationCell> Cells;

	bool Validate(const FCCLSurfaceGrid& Grid, FString& Error) const;
	bool ValidateAt(double WorldSeconds, FString& Error) const;
	bool Serialize(FArchive& Archive, int32 SurfaceCells);
};

namespace CCLSurfaceVegetation
{
	bool Initialize(FCCLSurfaceGrid& Grid, const TArray<FCCLVegetationSpecies>& Species,
		const TArray<FCCLVegetationSeed>& Seeds, double WorldSeconds, FString& Error);
	bool AdvanceCandidate(FCCLSurfaceGrid& Grid, double WorldFrom, double WorldDelta,
		double IrradianceWattsPerM2, double DaylightHours, FString& Error, FCCLSurfaceThermalContext* Context = nullptr);
	void RemoveSupport(FCCLSurfaceGrid& Grid, int32 CellIndex, double NewBedMeters, double WorldSeconds);
}
