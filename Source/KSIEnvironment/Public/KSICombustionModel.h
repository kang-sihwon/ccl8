#pragma once

#include "CoreMinimal.h"

struct KSIENVIRONMENT_API FKSICombustionMaterial
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

struct KSIENVIRONMENT_API FKSICombustionCell
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

	double TemperatureCelsius(const FKSICombustionMaterial& Material) const;
	double HeatBalanceError() const;
};

// Transfers only: the caller owns the source water, receiving surface and air space.
struct KSIENVIRONMENT_API FKSICombustionFlux
{
	double VaporizedWaterKg = 0.;
	double SurfaceHeatJoules = 0.;
	double SmokeKg = 0.;
	double AirHeatJoules = 0.;
};

class KSIENVIRONMENT_API FKSICombustionModel
{
public:
	static bool Initialize(const FKSICombustionMaterial& Material, double FuelKg, double TemperatureCelsius,
		FKSICombustionCell& State, FString& Error);
	static bool TransferFuel(const FKSICombustionMaterial& Material, double DeltaKg, FKSICombustionCell& State, FString& Error);
	static bool AddHeat(const FKSICombustionMaterial& Material, double Joules, FKSICombustionCell& State, FString& Error);
	static bool Advance(const FKSICombustionMaterial& Material, double GameSeconds, double AmbientCelsius,
		double WindMetersPerSecond, double AvailableWaterKg, FKSICombustionCell& State, FKSICombustionFlux& Flux, FString& Error, double AvailableAirHeatJoules = 1.e15);
	static bool Validate(const FKSICombustionMaterial& Material, const FKSICombustionCell& State, FString& Error);
	static void SerializeMaterial(FArchive& Archive, FKSICombustionMaterial& Material);
	static void SerializeCell(FArchive& Archive, FKSICombustionCell& State, bool bFuelTransfers = true);
	static void Serialize(FArchive& Archive, FKSICombustionMaterial& Material, FKSICombustionCell& State);
};
