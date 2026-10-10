#pragma once

#include "CoreMinimal.h"

// Open regional atmosphere. Rates use world seconds and liquid-equivalent water depths.
struct FCCLClimate
{
	double BaseTemperatureCelsius = -8.;
	double SolarWarmingCelsius = 30.;
	double ThermalResponseSeconds = 1800.;
	double MoistureSupplyMetersPerSecond = 0.000001;
	double CondensationSeconds = 300.;
	double PrevailingWindMetersPerSecond = 4.;
	double FrontPeriodSeconds = 21600.;
	int32 Seed = 42;
	uint8 bEnabled = 0;
};

struct FCCLWeatherState
{
	double TemperatureCelsius = -8.;
	double VaporMeters = 0.004;
	double CloudMeters = 0.;
	double WindMetersPerSecond = 4.;
	double RainMetersPerSecond = 0.;
	double SnowMetersPerSecond = 0.;
	double EvaporationMetersPerSecond = 0.;
	double IrradianceWattsPerM2 = 0.;
};

class CCL_API FCCLWeatherModel
{
public:
	static bool Advance(const FCCLClimate& Climate, double WorldFromSeconds, double WorldDeltaSeconds,
		double IrradianceWattsPerM2, FCCLWeatherState& State, FString& Error);
	static bool Validate(const FCCLClimate& Climate, const FCCLWeatherState& State, FString& Error);
	static void Serialize(FArchive& Archive, FCCLClimate& Climate, FCCLWeatherState& State);
};
