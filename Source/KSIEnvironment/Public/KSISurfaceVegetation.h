#pragma once

#include "CoreMinimal.h"

struct FKSISurfaceGrid;
struct FKSISurfaceThermalContext;

enum class EKSIVegetationKind : uint8
{
	Tree, Grass, Flower, Shrub, Moss, Fern, Algae, AquaticPlant, Fungus
};

enum class EKSIVegetationState : uint8
{
	Empty, Growing, Mature, Dormant, Dead
};

struct KSIENVIRONMENT_API FKSIVegetationSpecies
{
	FGuid Id;
	EKSIVegetationKind Kind = EKSIVegetationKind::Grass;
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

struct KSIENVIRONMENT_API FKSIVegetationSeed
{
	FGuid Id;
	int32 SpeciesIndex = INDEX_NONE;
	double BiomassKg = 0.;
	double SoilNutrientsKg = 0.;
	double SoilOrganicKg = 0.;
};

struct KSIENVIRONMENT_API FKSIVegetationCell
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
	EKSIVegetationState State = EKSIVegetationState::Empty;
	uint8 bRootsViable = 0;
};

// Optional layer: one persistent species patch per surface cell, not per visual stem.
struct KSIENVIRONMENT_API FKSISurfaceVegetationLayer
{
	TArray<FKSIVegetationSpecies> Species;
	TArray<FKSIVegetationCell> Cells;

	bool Validate(const FKSISurfaceGrid& Grid, FString& Error) const;
	bool ValidateAt(double WorldSeconds, FString& Error) const;
	bool Serialize(FArchive& Archive, int32 SurfaceCells);
};

namespace KSISurfaceVegetation
{
	KSIENVIRONMENT_API bool Initialize(FKSISurfaceGrid& Grid, const TArray<FKSIVegetationSpecies>& Species,
		const TArray<FKSIVegetationSeed>& Seeds, double WorldSeconds, FString& Error);
	KSIENVIRONMENT_API bool AdvanceCandidate(FKSISurfaceGrid& Grid, double WorldFrom, double WorldDelta,
		double IrradianceWattsPerM2, double DaylightHours, FString& Error, FKSISurfaceThermalContext* Context = nullptr);
	KSIENVIRONMENT_API void RemoveSupport(FKSISurfaceGrid& Grid, int32 CellIndex, double NewBedMeters, double WorldSeconds);
}
