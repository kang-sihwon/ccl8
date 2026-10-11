#include "CCLCombustionModel.h"

namespace
{
	constexpr double VaporizationJoulesPerKg = 2500000.;
	bool Range(double V, double Low, double High)
	{
		return FMath::IsFinite(V) && V >= Low && V <= High;
	}

	bool Fail(FString& Error)
	{
		Error = TEXT("Invalid combustion state, input or work budget.");
		return false;
	}
}

double FCCLCombustionCell::TemperatureCelsius(const FCCLCombustionMaterial& M) const
{
	return HeatJoules / M.HeatCapacityJoulesPerKelvin - 273.15;
}

double FCCLCombustionCell::HeatBalanceError() const
{
	return HeatJoules - (InitialHeatJoules + AddedHeatJoules + ReleasedHeatJoules - LostHeatJoules - TransferredHeatJoules);
}

bool FCCLCombustionModel::Validate(const FCCLCombustionMaterial& M, const FCCLCombustionCell& S, FString& Error)
{
	if (!Range(M.HeatCapacityJoulesPerKelvin, 1., 1.e9) || !Range(M.EnergyJoulesPerKg, 1., 1.e9)
		|| !Range(M.BurnKgPerGameSecond, 0., 1000.) || !Range(M.IgnitionCelsius, 0., 2000.)
		|| !Range(M.ExtinctionCelsius, 0., M.IgnitionCelsius) || !Range(M.MaximumWaterFuelRatio, 0.001, 100.)
		|| !Range(M.CoolingWattsPerKelvin, 0., 1.e6) || !Range(M.SurfaceWattsPerKelvin, 0., 1.e6)
		|| !Range(M.SmokeKgPerFuelKg, 0., 1.) || !Range(S.InitialFuelKg, 0., 1.e6)
		|| !Range(S.AddedFuelKg, 0., 1.e9) || !Range(S.RemovedFuelKg, 0., 1.e9)
		|| !Range(S.FuelKg, 0., S.InitialFuelKg + S.AddedFuelKg + 1.e-9) || !Range(S.BurnedKg, 0., S.InitialFuelKg + S.AddedFuelKg + 1.e-9)
		|| !Range(S.HeatJoules, 0., 1.e15) || !Range(S.InitialHeatJoules, 0., 1.e15)
		|| !Range(S.AddedHeatJoules, 0., 1.e15) || !Range(S.ReleasedHeatJoules, 0., 1.e15)
		|| !Range(S.LostHeatJoules, 0., 1.e15) || !Range(S.TransferredHeatJoules, 0., 1.e15) || S.bBurning > 1
		|| FMath::Abs(S.InitialFuelKg + S.AddedFuelKg - S.RemovedFuelKg - S.FuelKg - S.BurnedKg)
			> FMath::Max(1.e-10, (S.InitialFuelKg + S.AddedFuelKg) * 1.e-10)
		|| FMath::Abs(S.HeatBalanceError()) > FMath::Max(1.e-6, (S.InitialHeatJoules + S.AddedHeatJoules + S.ReleasedHeatJoules) * 1.e-10))
	{
		return Fail(Error);
	}
	return true;
}

bool FCCLCombustionModel::Initialize(const FCCLCombustionMaterial& M, double Fuel, double Temperature,
	FCCLCombustionCell& State, FString& Error)
{
	if (!Range(Fuel, 0., 1.e6) || !Range(Temperature, -100., 2000.))
	{
		return Fail(Error);
	}

	FCCLCombustionCell S;
	S.InitialFuelKg = S.FuelKg = Fuel;
	S.InitialHeatJoules = S.HeatJoules = M.HeatCapacityJoulesPerKelvin * (Temperature + 273.15);
	if (!Validate(M, S, Error))
	{
		return false;
	}

	State = S;
	return true;
}

bool FCCLCombustionModel::TransferFuel(const FCCLCombustionMaterial& M, double DeltaKg, FCCLCombustionCell& State, FString& Error)
{
	if (!FMath::IsFinite(DeltaKg) || FMath::Abs(DeltaKg) > 1.e6 || !Validate(M, State, Error)
		|| State.FuelKg + DeltaKg < 0.)
	{
		return Fail(Error);
	}

	auto Candidate = State;
	Candidate.FuelKg += DeltaKg;
	if (DeltaKg >= 0.)
	{
		Candidate.AddedFuelKg += DeltaKg;
	}
	else
	{
		Candidate.RemovedFuelKg -= DeltaKg;
	}

	if (Candidate.FuelKg == 0.)
	{
		Candidate.bBurning = 0;
	}

	if (!Validate(M, Candidate, Error))
	{
		return false;
	}

	State = Candidate;
	return true;
}

bool FCCLCombustionModel::AddHeat(const FCCLCombustionMaterial& M, double Joules, FCCLCombustionCell& State, FString& Error)
{
	if (!Range(Joules, 0., 1.e12) || !Validate(M, State, Error))
	{
		return Fail(Error);
	}

	auto S = State;
	S.HeatJoules += Joules;
	S.AddedHeatJoules += Joules;
	if (!Validate(M, S, Error))
	{
		return false;
	}

	State = S;
	return true;
}

bool FCCLCombustionModel::Advance(const FCCLCombustionMaterial& M, double Seconds, double Ambient,
	double Wind, double Water, FCCLCombustionCell& State, FCCLCombustionFlux& Flux, FString& Error, double AirBudget)
{
	if (!Validate(M, State, Error) || !Range(Seconds, 0., 409.6) || !Range(Ambient, -100., 2000.)
		|| !Range(Wind, 0., 200.) || !Range(Water, 0., 1.e9) || !Range(AirBudget, 0., 1.e15))
	{
		return Fail(Error);
	}

	auto S = State;
	FCCLCombustionFlux F;
	const int32 Steps = FMath::Max(1, FMath::CeilToInt32(Seconds / 0.1));
	const double Dt = Seconds / Steps;
	const double AmbientHeat = (Ambient + 273.15) * M.HeatCapacityJoulesPerKelvin;
	for (int32 I = 0; I < Steps && Dt > 0.; ++I)
	{
		const double Temperature = S.TemperatureCelsius(M);
		const double WetRatio = Water / FMath::Max(S.FuelKg, 0.001);
		S.bBurning = S.FuelKg > 0. && WetRatio < M.MaximumWaterFuelRatio
			&& Temperature >= (S.bBurning ? M.ExtinctionCelsius : M.IgnitionCelsius);
		if (S.bBurning)
		{
			const double Burned = FMath::Min(S.FuelKg, M.BurnKgPerGameSecond * Dt
				* (1. + FMath::Min(Wind, 20.) * 0.05) * (1. - WetRatio / M.MaximumWaterFuelRatio));
			S.FuelKg -= Burned;
			S.BurnedKg += Burned;
			const double Heat = Burned * M.EnergyJoulesPerKg;
			S.ReleasedHeatJoules += Heat;
			S.HeatJoules += Heat;
			F.SmokeKg += Burned * M.SmokeKgPerFuelKg;
		}
		const double RequestedExchange = (S.HeatJoules - AmbientHeat)
			* (1. - FMath::Exp(-M.CoolingWattsPerKelvin * (1. + Wind * 0.1) * Dt / M.HeatCapacityJoulesPerKelvin));
		const double Exchange = FMath::Max(RequestedExchange, -AirBudget);
		AirBudget -= FMath::Max(0., -Exchange);
		F.AirHeatJoules += Exchange;
		S.HeatJoules -= Exchange;
		if (Exchange >= 0.)
		{
			S.LostHeatJoules += Exchange;
		}
		else
		{
			S.AddedHeatJoules -= Exchange;
		}

		const double AboveAmbient = FMath::Max(0., S.HeatJoules - AmbientHeat);
		const double ToSurface = AboveAmbient * (1. - FMath::Exp(-M.SurfaceWattsPerKelvin * Dt / M.HeatCapacityJoulesPerKelvin));
		const double VaporHeat = FMath::Min(ToSurface, Water * VaporizationJoulesPerKg);
		const double Vapor = VaporHeat / VaporizationJoulesPerKg;
		Water -= Vapor;
		F.VaporizedWaterKg += Vapor;
		F.SurfaceHeatJoules += ToSurface - VaporHeat;
		S.HeatJoules -= ToSurface;
		S.TransferredHeatJoules += ToSurface;
		if (S.FuelKg <= 0. || S.TemperatureCelsius(M) < M.ExtinctionCelsius)
		{
			S.bBurning = 0;
		}
	}

	if (!Validate(M, S, Error))
	{
		return false;
	}

	State = S;
	Flux = F;
	return true;
}

void FCCLCombustionModel::SerializeMaterial(FArchive& Ar, FCCLCombustionMaterial& M)
{
	Ar << M.HeatCapacityJoulesPerKelvin << M.EnergyJoulesPerKg << M.BurnKgPerGameSecond << M.IgnitionCelsius
		<< M.ExtinctionCelsius << M.MaximumWaterFuelRatio << M.CoolingWattsPerKelvin << M.SurfaceWattsPerKelvin << M.SmokeKgPerFuelKg;
}

void FCCLCombustionModel::SerializeCell(FArchive& Ar, FCCLCombustionCell& S, bool bFuelTransfers)
{
	Ar << S.InitialFuelKg << S.FuelKg << S.BurnedKg << S.HeatJoules << S.InitialHeatJoules << S.AddedHeatJoules
		<< S.ReleasedHeatJoules << S.LostHeatJoules << S.TransferredHeatJoules << S.bBurning;
	if (bFuelTransfers)
	{
		Ar << S.AddedFuelKg << S.RemovedFuelKg;
	}
}

void FCCLCombustionModel::Serialize(FArchive& Ar, FCCLCombustionMaterial& M, FCCLCombustionCell& S)
{
	SerializeMaterial(Ar, M);
	SerializeCell(Ar, S);
}
