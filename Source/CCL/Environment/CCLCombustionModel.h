#pragma once

#include "CoreMinimal.h"

struct FCCLCombustionMaterial
{
	double HeatCapacityJoulesPerKelvin = 2000.;
	double EnergyJoulesPerKg = 18000000.;
	double BurnKgPerGameSecond = 0.008;
	double IgnitionCelsius = 220.;
	double ExtinctionCelsius = 100.;
	double MaximumWaterFuelRatio = 0.3;
	double CoolingWattsPerKelvin = 12.;
	double SurfaceWattsPerKelvin = 30.;
	double SmokeKgPerFuelKg = 0.02;
};

struct FCCLCombustionCell
{
	double InitialFuelKg = 0.;
	double AddedFuelKg = 0.;
	double RemovedFuelKg = 0.;
	double FuelKg = 0.;
	double BurnedKg = 0.;
	double HeatJoules = 0.;
	double InitialHeatJoules = 0.;
	double AddedHeatJoules = 0.;
	double ReleasedHeatJoules = 0.;
	double LostHeatJoules = 0.;
	double TransferredHeatJoules = 0.;
	uint8 bBurning = 0;

	double TemperatureCelsius(const FCCLCombustionMaterial& Material) const;
	double HeatBalanceError() const;
};

// Transfers only: the caller owns the source water, receiving surface and air space.
struct FCCLCombustionFlux
{
	double VaporizedWaterKg = 0.;
	double SurfaceHeatJoules = 0.;
	double SmokeKg = 0.;
	double AirHeatJoules = 0.;
};

class CCL_API FCCLCombustionModel
{
public:
	static bool Initialize(const FCCLCombustionMaterial& Material, double FuelKg, double TemperatureCelsius,
		FCCLCombustionCell& State, FString& Error);
	static bool TransferFuel(const FCCLCombustionMaterial& Material, double DeltaKg, FCCLCombustionCell& State, FString& Error);
	static bool AddHeat(const FCCLCombustionMaterial& Material, double Joules, FCCLCombustionCell& State, FString& Error);
	static bool Advance(const FCCLCombustionMaterial& Material, double GameSeconds, double AmbientCelsius,
		double WindMetersPerSecond, double AvailableWaterKg, FCCLCombustionCell& State, FCCLCombustionFlux& Flux, FString& Error, double AvailableAirHeatJoules = 1.e15);
	static bool Validate(const FCCLCombustionMaterial& Material, const FCCLCombustionCell& State, FString& Error);
	static void SerializeMaterial(FArchive& Archive, FCCLCombustionMaterial& Material);
	static void SerializeCell(FArchive& Archive, FCCLCombustionCell& State, bool bFuelTransfers = true);
	static void Serialize(FArchive& Archive, FCCLCombustionMaterial& Material, FCCLCombustionCell& State);
};
