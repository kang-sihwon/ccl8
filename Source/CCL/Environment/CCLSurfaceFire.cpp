#include "CCLSurfaceFire.h"

#include "CCLSurfaceSimulation.h"
#include "CCLCelestialSystem.h"
#include "CCLSurfaceQuery.h"

namespace
{
	constexpr double WaterKgPerCubicMeter = 1000.;
	constexpr double MeltJoulesPerKg = 334000.;
	constexpr double VaporJoulesPerKg = 2500000.;

	bool Quantity(double Value)
	{
		return FMath::IsFinite(Value) && Value >= 0. && Value <= 1.e15;
	}

	bool Near(double A, double B)
	{
		return FMath::Abs(A - B) <= FMath::Max(1.e-7, FMath::Max(A, B) * 1.e-9);
	}
}

bool FCCLSurfaceFireLayer::Validate(int32 SurfaceCells, FString& Error) const
{
	if ((!Cells.IsEmpty() && Cells.Num() != SurfaceCells) || !Quantity(SpreadWattsPerKelvin)
		|| SpreadWattsPerKelvin > 1.e6 || !Quantity(SpreadHeatJoules) || !Quantity(MeltHeatJoules)
		|| !Quantity(DissipatedSurfaceHeatJoules) || !Quantity(VaporizedWaterKg) || !Quantity(EmittedSmokeKg))
	{
		Error = TEXT("Invalid surface combustion layer.");
		return false;
	}

	double Transferred = 0.;
	double Burned = 0.;
	for (const auto& Cell : Cells)
	{
		if (!FCCLCombustionModel::Validate(Material, Cell, Error))
		{
			return false;
		}
		Transferred += Cell.TransferredHeatJoules;
		Burned += Cell.BurnedKg;
	}

	if (!Near(Transferred, SpreadHeatJoules + MeltHeatJoules + DissipatedSurfaceHeatJoules + VaporizedWaterKg * VaporJoulesPerKg)
		|| !Near(EmittedSmokeKg, Burned * Material.SmokeKgPerFuelKg))
	{
		Error = TEXT("Surface combustion transfers or smoke do not balance.");
		return false;
	}
	return true;
}

bool FCCLSurfaceFireLayer::Serialize(FArchive& Ar, int32 SurfaceCells, bool bFuelTransfers)
{
	uint8 Enabled = !Cells.IsEmpty();
	Ar << Enabled;
	if (Ar.IsError() || Enabled > 1)
	{
		return false;
	}
	if (!Enabled)
	{
		return true;
	}
	if (Ar.IsLoading() && int64(SurfaceCells) * (bFuelTransfers ? 89 : 73) + 120 > Ar.TotalSize() - Ar.Tell())
	{
		return false;
	}
	FCCLCombustionModel::SerializeMaterial(Ar, Material);
	Ar << SpreadWattsPerKelvin << SpreadHeatJoules << MeltHeatJoules << DissipatedSurfaceHeatJoules
		<< VaporizedWaterKg << EmittedSmokeKg;
	Cells.SetNum(SurfaceCells);
	for (auto& Cell : Cells)
	{
		FCCLCombustionModel::SerializeCell(Ar, Cell, bFuelTransfers);
	}
	return !Ar.IsError();
}

void FCCLSurfaceThermalContext::AddSource(FGuid Id, double Heat, double Vapor, double Smoke)
{
	Sources.FindOrAdd(Id) = Sources.FindRef(Id) + FCCLAirContents{Heat, Vapor, Smoke};
}

FVector FCCLSurfaceThermalContext::CellPosition(const FCCLSurfaceGrid& G, int32 I)
{
	return FVector(G.OriginMeters.X + (I % G.Size.X + 0.5) * G.SpacingMeters,
		G.OriginMeters.Y + (I / G.Size.X + 0.5) * G.SpacingMeters, G.Cells[I].BedMeters + 0.01);
}

bool CCLSurfaceFire::AdvanceCandidate(FCCLSurfaceGrid& G, double Seconds, FString& Error, FCCLSurfaceThermalContext* Context)
{
	auto& Layer = G.Fire;
	if (Layer.Cells.IsEmpty() || Seconds == 0.)
	{
		return true;
	}

	for (int32 I = 0; I < G.Cells.Num(); ++I)
	{
		auto& Cell = G.Cells[I];
		FCCLCombustionFlux Flux;
		const double WaterKg = (Cell.WaterCubicMeters + Cell.SoilCubicMeters) * WaterKgPerCubicMeter;
		const double Wind = G.Weather.WindMetersPerSecond * Cell.WindExposure;
		const auto* Room = Context && Context->PreviousAir ? Context->PreviousAir->FindSpaceAt(FCCLSurfaceThermalContext::CellPosition(G, I)) : nullptr;
		const double Ambient = Room ? Room->TemperatureCelsius() : G.Forcing.TemperatureCelsius;
		const double AirBudget = Room ? FMath::Max(0., (Ambient + 100.) * Room->VolumeCubicMeters
			* FCCLAirSimulation::HeatCapacityPerCubicMeter) * 0.5 / FMath::Max(1, Context->ContactCounts.FindRef(Room->SpaceId)) : 1.e15;
		if (!FCCLCombustionModel::Advance(Layer.Material, Seconds, Ambient,
			Wind, WaterKg, Layer.Cells[I], Flux, Error, AirBudget))
		{
			return false;
		}

		const double Vapor = Flux.VaporizedWaterKg / WaterKgPerCubicMeter;
		const double LiquidLoss = FMath::Min(Cell.WaterCubicMeters, Vapor);
		const double SoilLoss = FMath::Min(Cell.SoilCubicMeters, FMath::Max(0., Vapor - LiquidLoss));
		Cell.WaterCubicMeters -= LiquidLoss;
		Cell.SoilCubicMeters -= SoilLoss;
		G.Ledger.EvaporatedCubicMeters += LiquidLoss + SoilLoss;
		Layer.VaporizedWaterKg += Flux.VaporizedWaterKg;
		Layer.EmittedSmokeKg += Flux.SmokeKg;

		const double MeltBudget = Flux.SurfaceHeatJoules / (WaterKgPerCubicMeter * MeltJoulesPerKg);
		const double IceMelt = FMath::Min(Cell.IceCubicMeters, MeltBudget);
		const double SnowMelt = FMath::Min(Cell.SnowCubicMeters, FMath::Max(0., MeltBudget - IceMelt));
		const double Fraction = Cell.SnowCubicMeters > 0. ? SnowMelt / Cell.SnowCubicMeters : 0.;
		Cell.SnowVolumeCubicMeters *= 1. - Fraction;
		Cell.SnowCubicMeters -= SnowMelt;
		Cell.IceCubicMeters -= IceMelt;
		Cell.WaterCubicMeters += IceMelt + SnowMelt;
		const double MeltHeat = (IceMelt + SnowMelt) * WaterKgPerCubicMeter * MeltJoulesPerKg;
		Layer.MeltHeatJoules += MeltHeat;
		const double Dissipated = FMath::Max(0., Flux.SurfaceHeatJoules - MeltHeat);
		Layer.DissipatedSurfaceHeatJoules += Dissipated;
		if (Room)
		{
			Context->AddSource(Room->SpaceId, Flux.AirHeatJoules + Dissipated, Flux.VaporizedWaterKg, Flux.SmokeKg);
		}
	}

	// At most four neighbours. Each edge uses <= 1/8 of the frozen heat difference,
	// leaving >= half the donor's initial heat even when every neighbour is cold.
	TArray<double> Sent, Received;
	Sent.SetNumZeroed(G.Cells.Num());
	Received.SetNumZeroed(G.Cells.Num());
	bool bQueryFailed = false;
	auto Transfer = [&](int32 A, int32 B)
	{
		if (FMath::Min(G.Cells[A].CeilingMeters, G.Cells[B].CeilingMeters)
			<= FMath::Max(G.Cells[A].BedMeters, G.Cells[B].BedMeters))
		{
			return;
		}
		double Transmission = 1.;
		if (Context && Context->Shelter)
		{
			if (!Context->Observer)
			{
				Error = TEXT("Fire barriers require a celestial body context.");
				bQueryFailed = true;
				return;
			}
			FCCLSurfaceQuery Query;
			Query.BodyId = Context->Observer->BodyId;
			Query.OriginMeters = FCCLSurfaceThermalContext::CellPosition(G, A);
			const FVector Segment = FCCLSurfaceThermalContext::CellPosition(G, B) - Query.OriginMeters;
			Query.Direction = Segment.GetSafeNormal();
			Query.MaximumDistanceMeters = Segment.Length() - 1.e-5;
			Query.KindMask = uint8(ECCLSurfaceKind::Terrain) | uint8(ECCLSurfaceKind::Structure);
			Query.RequiredRevision = Context->Shelter->GetRevision();
			Query.RequiredEpoch = Context->Shelter->GetEpoch();
			TArray<FCCLSurfaceSample> Hits;
			if (!Context->Shelter->QuerySurfaces(Query, Hits, Error))
			{
				bQueryFailed = true;
				return;
			}
			for (const auto& Hit : Hits)
			{
				Transmission *= Hit.Transmission.Wind;
			}
		}
		const double Difference = Layer.Cells[A].HeatJoules - Layer.Cells[B].HeatJoules;
		const int32 From = Difference > 0. ? A : B;
		const int32 To = Difference > 0. ? B : A;
		const double Wind = G.Weather.WindMetersPerSecond * FMath::Min(G.Cells[A].WindExposure, G.Cells[B].WindExposure);
		const double Fraction = FMath::Min(0.125, Layer.SpreadWattsPerKelvin * Seconds
			* (1. + FMath::Min(Wind, 20.) * 0.05) / Layer.Material.HeatCapacityJoulesPerKelvin);
		const double Heat = FMath::Abs(Difference) * Fraction * Transmission;
		Sent[From] += Heat;
		Received[To] += Heat;
		Layer.SpreadHeatJoules += Heat;
	};
	for (int32 Y = 0; Y < G.Size.Y; ++Y)
	{
		for (int32 X = 0; X < G.Size.X; ++X)
		{
			const int32 I = X + Y * G.Size.X;
			if (X + 1 < G.Size.X)
			{
				Transfer(I, I + 1);
			}
			if (Y + 1 < G.Size.Y)
			{
				Transfer(I, I + G.Size.X);
			}
		}
	}
	if (bQueryFailed)
	{
		return false;
	}
	for (int32 I = 0; I < Layer.Cells.Num(); ++I)
	{
		auto& Cell = Layer.Cells[I];
		Cell.HeatJoules += Received[I] - Sent[I];
		Cell.AddedHeatJoules += Received[I];
		Cell.TransferredHeatJoules += Sent[I];
	}
	return true;
}
