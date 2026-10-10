#include "CCLWeatherModel.h"

namespace
{
	bool Range(double Value, double Min, double Max)
	{
		return FMath::IsFinite(Value) && Value >= Min && Value <= Max;
	}
}

bool FCCLWeatherModel::Advance(const FCCLClimate& C, double From, double Delta, double Sun,
	FCCLWeatherState& State, FString& Error)
{
	if (!Validate(C, State, Error) || !Range(From, 0., 1.e15) || !Range(Delta, 0., 60.)
		|| !Range(Sun, 0., 100000.))
	{
		Error = TEXT("Weather requires finite inputs and at most 60 world seconds per step.");
		return false;
	}

	if (Delta == 0. || !C.bEnabled)
	{
		return true;
	}

	auto Next = State;
	const double Phase = 2. * PI * FMath::Fmod(From + Delta * 0.5, C.FrontPeriodSeconds) / C.FrontPeriodSeconds
		+ double(uint32(C.Seed) % 65536) * (2. * PI / 65536.);
	const double Front = 0.5 + 0.5 * FMath::Sin(Phase);
	Next.WindMetersPerSecond = C.PrevailingWindMetersPerSecond * (0.5 + Front);
	const double CloudFraction = FMath::Clamp(State.CloudMeters / 0.003, 0., 1.);
	Next.IrradianceWattsPerM2 = Sun * (1. - 0.75 * CloudFraction);
	const double Target = C.BaseTemperatureCelsius + C.SolarWarmingCelsius * FMath::Min(2., Next.IrradianceWattsPerM2 / 1000.);
	Next.TemperatureCelsius = Target + (State.TemperatureCelsius - Target) * FMath::Exp(-Delta / C.ThermalResponseSeconds);
	Next.VaporMeters += C.MoistureSupplyMetersPerSecond * (0.5 + Front) * Delta;
	const double Capacity = 0.005 * FMath::Exp(FMath::Clamp(Next.TemperatureCelsius * 0.06, -10., 10.));
	const double Condensed = FMath::Max(0., Next.VaporMeters - Capacity) * (1. - FMath::Exp(-Delta / C.CondensationSeconds));
	Next.VaporMeters -= Condensed;
	Next.CloudMeters += Condensed;
	const double Precipitation = Next.CloudMeters * (1. - FMath::Exp(-Delta / C.CondensationSeconds));
	Next.CloudMeters -= Precipitation;
	const double SnowFraction = FMath::Clamp((1. - Next.TemperatureCelsius) * 0.5, 0., 1.);
	Next.SnowMetersPerSecond = Precipitation * SnowFraction / Delta;
	Next.RainMetersPerSecond = Precipitation * (1. - SnowFraction) / Delta;
	const double Humidity = FMath::Clamp(Next.VaporMeters / Capacity, 0., 1.);
	Next.EvaporationMetersPerSecond = 0.00000002 * FMath::Max(0., Next.TemperatureCelsius + 5.)
		* (1. - Humidity) * (1. + Next.WindMetersPerSecond * 0.2);
	if (!Validate(C, Next, Error))
	{
		return false;
	}

	State = Next;
	return true;
}

bool FCCLWeatherModel::Validate(const FCCLClimate& C, const FCCLWeatherState& S, FString& Error)
{
	if (C.bEnabled > 1 || !Range(C.BaseTemperatureCelsius, -100., 100.) || !Range(C.SolarWarmingCelsius, 0., 100.)
		|| !Range(C.ThermalResponseSeconds, 1., 1.e7) || !Range(C.MoistureSupplyMetersPerSecond, 0., 0.0001)
		|| !Range(C.CondensationSeconds, 1., 1.e7) || !Range(C.PrevailingWindMetersPerSecond, 0., 100.)
		|| !Range(C.FrontPeriodSeconds, 60., 1.e9) || !Range(S.TemperatureCelsius, -100., 300.)
		|| !Range(S.VaporMeters, 0., 100.) || !Range(S.CloudMeters, 0., 100.)
		|| !Range(S.WindMetersPerSecond, 0., 150.) || !Range(S.RainMetersPerSecond, 0., 1.)
		|| !Range(S.SnowMetersPerSecond, 0., 1.) || !Range(S.EvaporationMetersPerSecond, 0., 1.)
		|| !Range(S.IrradianceWattsPerM2, 0., 100000.))
	{
		Error = TEXT("Invalid regional climate or weather state.");
		return false;
	}

	return true;
}

void FCCLWeatherModel::Serialize(FArchive& Ar, FCCLClimate& C, FCCLWeatherState& S)
{
	Ar << C.BaseTemperatureCelsius << C.SolarWarmingCelsius << C.ThermalResponseSeconds
		<< C.MoistureSupplyMetersPerSecond << C.CondensationSeconds << C.PrevailingWindMetersPerSecond
		<< C.FrontPeriodSeconds << C.Seed << C.bEnabled;
	Ar << S.TemperatureCelsius << S.VaporMeters << S.CloudMeters << S.WindMetersPerSecond
		<< S.RainMetersPerSecond << S.SnowMetersPerSecond << S.EvaporationMetersPerSecond << S.IrradianceWattsPerM2;
}
