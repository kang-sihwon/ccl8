#include "KSISurfaceSimulation.h"

#include "KSICelestialSystem.h"
#include "KSISurfaceQuery.h"
#include "Misc/Crc.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace
{
	constexpr uint32 Magic = 0x534C4343;
	constexpr double MaximumQuantity = 1.e15;

	bool Quantity(double Value)
	{
		return FMath::IsFinite(Value) && Value >= 0. && Value <= MaximumQuantity;
	}

	bool Fail(FString& Error, const TCHAR* Message)
	{
		Error = Message;
		return false;
	}

	bool ValidForcing(const FKSISurfaceForcing& F)
	{
		return Quantity(F.RainMetersPerWorldSecond) && F.RainMetersPerWorldSecond <= 1.
			&& Quantity(F.SnowMetersPerWorldSecond) && F.SnowMetersPerWorldSecond <= 1.
			&& Quantity(F.EvaporationMetersPerWorldSecond) && F.EvaporationMetersPerWorldSecond <= 1.
			&& Quantity(F.InfiltrationMetersPerWorldSecond) && F.InfiltrationMetersPerWorldSecond <= 1.
			&& Quantity(F.DrainageMetersPerWorldSecond) && F.DrainageMetersPerWorldSecond <= 1.
			&& Quantity(F.SoilCapacityMeters) && F.SoilCapacityMeters <= 10.
			&& FMath::IsFinite(F.TemperatureCelsius) && FMath::Abs(F.TemperatureCelsius) <= 1000.
			&& Quantity(F.PhaseMetersPerDegreeWorldSecond) && F.PhaseMetersPerDegreeWorldSecond <= 1.
			&& Quantity(F.FlowConductance) && F.FlowConductance <= 1000.
			&& FMath::IsFinite(F.BoundaryLevelMeters) && FMath::Abs(F.BoundaryLevelMeters) <= 1.e7
			&& Quantity(F.BoundaryConductance) && F.BoundaryConductance <= 1000.;
	}

	double Capacity(const FKSISurfaceGrid& G, const FKSISurfaceCell& Cell)
	{
		return (Cell.CeilingMeters - Cell.BedMeters) * FMath::Square(G.SpacingMeters);
	}

	bool GuidLess(const FGuid& A, const FGuid& B)
	{
		if (A.A != B.A)
		{
			return A.A < B.A;
		}
		if (A.B != B.B)
		{
			return A.B < B.B;
		}
		if (A.C != B.C)
		{
			return A.C < B.C;
		}
		return A.D < B.D;
	}

	struct FTransfer
	{
		int32 From;
		int32 To;
		double Amount;
	};

	bool AdvanceGrid(FKSISurfaceGrid& G, double GameDelta, double WorldFrom, double WorldDelta, int32 Steps,
		double Irradiance, double DaylightHours, FString& Error, FKSISurfaceThermalContext& Context)
	{
		const double Area = FMath::Square(G.SpacingMeters);
		const double Dt = GameDelta / Steps;
		const double WorldDt = WorldDelta / Steps;
		const auto& F = G.Forcing;
		TArray<FTransfer> Transfers;
		TArray<double> Outgoing, Incoming, SourceScales, TargetScales, Remaining, RoomLeft, Added;
		Outgoing.SetNumZeroed(G.Cells.Num());
		Incoming.SetNumZeroed(G.Cells.Num());
		SourceScales.SetNumUninitialized(G.Cells.Num());
		TargetScales.SetNumUninitialized(G.Cells.Num());
		Remaining.SetNumUninitialized(G.Cells.Num());
		RoomLeft.SetNumUninitialized(G.Cells.Num());
		Added.SetNumUninitialized(G.Cells.Num());
		Transfers.Reserve(G.Cells.Num() * 2);
		for (int32 Substep = 0; Substep < Steps; ++Substep)
		{
			for (int32 CellIndex = 0; CellIndex < G.Cells.Num(); ++CellIndex)
			{
				auto& Cell = G.Cells[CellIndex];
				const auto* Room = Context.PreviousAir->FindSpaceAt(FKSISurfaceThermalContext::CellPosition(G, CellIndex));
				const double Rain = F.RainMetersPerWorldSecond * WorldDt * Area * Cell.RainExposure;
				Cell.WaterCubicMeters += Rain;
				G.Ledger.RainCubicMeters += Rain;
				const double Snow = F.SnowMetersPerWorldSecond * WorldDt * Area * Cell.RainExposure;
				Cell.SnowCubicMeters += Snow;
				Cell.SnowVolumeCubicMeters += Snow / 0.1;
				G.Ledger.SnowCubicMeters += Snow;
				const double Infiltration = FMath::Min3(Cell.WaterCubicMeters, F.InfiltrationMetersPerWorldSecond * WorldDt * Area,
					FMath::Max(0., F.SoilCapacityMeters * Area - Cell.SoilCubicMeters));
				Cell.WaterCubicMeters -= Infiltration;
				Cell.SoilCubicMeters += Infiltration;
				const double Drainage = FMath::Min(Cell.SoilCubicMeters, F.DrainageMetersPerWorldSecond * WorldDt * Area);
				Cell.SoilCubicMeters -= Drainage;
				Cell.WaterCubicMeters += Drainage;
				const double Phase = F.PhaseMetersPerDegreeWorldSecond * FMath::Abs(F.TemperatureCelsius) * WorldDt * Area;
				if (Room)
				{
					const double Temperature = Room->TemperatureCelsius();
					const double CapacityPerCell = Room->VolumeCubicMeters * FKSIAirSimulation::HeatCapacityPerCubicMeter
						/ FMath::Max(1, Context.ContactCounts.FindRef(Room->SpaceId));
					const double Heat = FMath::Min(FMath::Abs(Temperature) * 30. * Area * Dt,
						FMath::Abs(Temperature) * CapacityPerCell * 0.5);
					if (Temperature > 0.)
					{
						const double IceMelt = FMath::Min(Cell.IceCubicMeters, Heat / 334000000.);
						const double SnowMelt = FMath::Min(Cell.SnowCubicMeters, FMath::Max(0., Heat / 334000000. - IceMelt));
						const double Fraction = Cell.SnowCubicMeters > 0. ? SnowMelt / Cell.SnowCubicMeters : 0.;
						Cell.SnowVolumeCubicMeters *= 1. - Fraction;
						Cell.SnowCubicMeters -= SnowMelt;
						Cell.IceCubicMeters -= IceMelt;
						Cell.WaterCubicMeters += IceMelt + SnowMelt;
						const double MeltHeat = (IceMelt + SnowMelt) * 334000000.;
						const double Vapor = FMath::Min(Cell.WaterCubicMeters + Cell.SoilCubicMeters,
							FMath::Max(0., Heat - MeltHeat) / 2500000000.);
						const double LiquidLoss = FMath::Min(Cell.WaterCubicMeters, Vapor);
						const double SoilLoss = FMath::Min(Cell.SoilCubicMeters, FMath::Max(0., Vapor - LiquidLoss));
						Cell.WaterCubicMeters -= LiquidLoss;
						Cell.SoilCubicMeters -= SoilLoss;
						G.Ledger.EvaporatedCubicMeters += LiquidLoss + SoilLoss;
						const double UsedHeat = MeltHeat + (LiquidLoss + SoilLoss) * 2500000000.;
						G.AirHeatToSurfaceJoules += UsedHeat;
						Context.AddSource(Room->SpaceId, -UsedHeat, (LiquidLoss + SoilLoss) * 1000., 0.);
					}
					else
					{
						const double Frozen = FMath::Min(Cell.WaterCubicMeters, Heat / 334000000.);
						Cell.WaterCubicMeters -= Frozen;
						Cell.IceCubicMeters += Frozen;
						const double Released = Frozen * 334000000.;
						G.SurfaceHeatToAirJoules += Released;
						Context.AddSource(Room->SpaceId, Released, 0., 0.);
					}
				}
				else if (F.TemperatureCelsius < 0.)
				{
					const double Frozen = FMath::Min(Cell.WaterCubicMeters, Phase);
					Cell.WaterCubicMeters -= Frozen;
					Cell.IceCubicMeters += Frozen;
				}
				else
				{
					const double Melted = FMath::Min(Cell.IceCubicMeters, Phase);
					Cell.IceCubicMeters -= Melted;
					Cell.WaterCubicMeters += Melted;
					const double SnowMelt = FMath::Min(Cell.SnowCubicMeters, Phase - Melted);
					const double Fraction = Cell.SnowCubicMeters > 0. ? SnowMelt / Cell.SnowCubicMeters : 0.;
					Cell.SnowVolumeCubicMeters *= 1. - Fraction;
					Cell.SnowCubicMeters -= SnowMelt;
					Cell.WaterCubicMeters += SnowMelt;
				}

				const double Demand = Room ? 0. : F.EvaporationMetersPerWorldSecond * WorldDt * Area;
				const double LiquidLoss = FMath::Min(Cell.WaterCubicMeters, Demand);
				const double SoilLoss = FMath::Min(Cell.SoilCubicMeters, Demand - LiquidLoss);
				Cell.WaterCubicMeters -= LiquidLoss;
				Cell.SoilCubicMeters -= SoilLoss;
				G.Ledger.EvaporatedCubicMeters += LiquidLoss + SoilLoss;
				if (Cell.WaterCubicMeters + Cell.IceCubicMeters + Cell.SnowVolumeCubicMeters > Capacity(G, Cell) + 1.e-9)
				{
					return Fail(Error, TEXT("Surface input exceeds the cell's explicit storage capacity."));
				}
			}

			if (!KSISurfaceFire::AdvanceCandidate(G, Dt, Error, &Context)
				|| !KSISurfaceVegetation::AdvanceCandidate(G, WorldFrom + Substep * WorldDt, WorldDt, Irradiance, DaylightHours, Error, &Context))
			{
				return false;
			}

			Transfers.Reset();
			for (double& Value : Outgoing)
			{
				Value = 0.;
			}
			for (double& Value : Incoming)
			{
				Value = 0.;
			}

			auto Propose = [&](int32 A, int32 B)
			{
				const double Difference = G.WaterLevelMeters(A) - G.WaterLevelMeters(B);
				const int32 From = Difference > 0. ? A : B;
				const int32 To = Difference > 0. ? B : A;
				const double Sill = FMath::Max(G.Cells[A].BedMeters, G.Cells[B].BedMeters);
				if (FMath::Min(G.Cells[A].CeilingMeters, G.Cells[B].CeilingMeters) <= Sill)
				{
					return;
				}
				const double AboveSill = FMath::Max(0., G.WaterLevelMeters(From) - FMath::Max(G.Cells[A].BedMeters, G.Cells[B].BedMeters)) * Area;
				const double Amount = FMath::Min3(FMath::Abs(Difference) * Area * 0.5,
					F.FlowConductance * FMath::Abs(Difference) * Dt, FMath::Min(AboveSill, G.Cells[From].WaterCubicMeters));
				if (Amount > 0.)
				{
					Transfers.Add({From, To, Amount});
					Outgoing[From] += Amount;
					Incoming[To] += Amount;
				}
			};
			for (int32 Y = 0; Y < G.Size.Y; ++Y)
			{
				for (int32 X = 0; X < G.Size.X; ++X)
				{
					const int32 I = X + Y * G.Size.X;
					if (X + 1 < G.Size.X)
					{
						Propose(I, I + 1);
					}
					if (Y + 1 < G.Size.Y)
					{
						Propose(I, I + G.Size.X);
					}
				}
			}
			// Freeze both limits before applying any edge so iteration order cannot change flux.
			for (int32 I = 0; I < G.Cells.Num(); ++I)
			{
				const auto& Cell = G.Cells[I];
				SourceScales[I] = Outgoing[I] > 0. ? FMath::Min(1., Cell.WaterCubicMeters / Outgoing[I]) : 1.;
				const double Room = FMath::Max(0., Capacity(G, Cell) - Cell.WaterCubicMeters - Cell.IceCubicMeters - Cell.SnowVolumeCubicMeters);
				TargetScales[I] = Incoming[I] > 0. ? FMath::Min(1., Room / Incoming[I]) : 1.;
				Remaining[I] = Cell.WaterCubicMeters;
				RoomLeft[I] = Room;
				Added[I] = 0.;
			}
			for (const auto& Transfer : Transfers)
			{
				const double Requested = Transfer.Amount * FMath::Min(SourceScales[Transfer.From], TargetScales[Transfer.To]);
				// Remaining frozen budgets absorb final-ulp rounding without spending newly received water.
				const double Amount = FMath::Min3(Requested, Remaining[Transfer.From], RoomLeft[Transfer.To]);
				Remaining[Transfer.From] -= Amount;
				RoomLeft[Transfer.To] -= Amount;
				Added[Transfer.To] += Amount;
			}

			for (int32 I = 0; I < G.Cells.Num(); ++I)
			{
				auto& Cell = G.Cells[I];
				Cell.WaterCubicMeters = Remaining[I] + Added[I];
				if (!Cell.bOpenBoundary || F.BoundaryConductance == 0.)
				{
					continue;
				}

				const double Difference = F.BoundaryLevelMeters - G.WaterLevelMeters(I);
				const double Requested = FMath::Min(FMath::Abs(Difference) * Area, F.BoundaryConductance * FMath::Abs(Difference) * Dt);
				if (Difference > 0.)
				{
					const double Amount = FMath::Min(Requested, FMath::Max(0., Capacity(G, Cell) - Cell.WaterCubicMeters - Cell.IceCubicMeters - Cell.SnowVolumeCubicMeters));
					Cell.WaterCubicMeters += Amount;
					G.Ledger.BoundaryInCubicMeters += Amount;
				}
				else
				{
					const double Amount = FMath::Min(Requested, Cell.WaterCubicMeters);
					Cell.WaterCubicMeters -= Amount;
					G.Ledger.BoundaryOutCubicMeters += Amount;
				}
			}
		}

		++G.Revision;
		return FKSISurfaceSimulation::ValidateRegion(G, Error);
	}
}

double FKSISurfaceGrid::TotalCubicMeters() const
{
	double Total = 0.;
	for (const auto& Cell : Cells)
	{
		Total += Cell.WaterCubicMeters + Cell.IceCubicMeters + Cell.SoilCubicMeters + Cell.SnowCubicMeters;
	}
	return Total;
}

double FKSISurfaceGrid::BalanceErrorCubicMeters() const
{
	return TotalCubicMeters() - (Ledger.InitialCubicMeters + Ledger.RainCubicMeters + Ledger.SnowCubicMeters + Ledger.BoundaryInCubicMeters
		- Ledger.BoundaryOutCubicMeters - Ledger.EvaporatedCubicMeters);
}

double FKSISurfaceGrid::WaterLevelMeters(int32 Index) const
{
	const auto& Cell = Cells[Index];
	return Cell.BedMeters + (Cell.WaterCubicMeters + Cell.IceCubicMeters) / FMath::Square(SpacingMeters);
}

double FKSISurfaceGrid::Wetness(int32 Index) const
{
	const auto& Cell = Cells[Index];
	const double Capacity = FMath::Max(1.e-9, Forcing.SoilCapacityMeters * FMath::Square(SpacingMeters));
	return FMath::Clamp(Cell.SoilCubicMeters / Capacity + Cell.WaterCubicMeters / FMath::Square(SpacingMeters) * 10., 0., 1.);
}

double FKSISurfaceGrid::Mud(int32 Index) const
{
	return FMath::Clamp((Wetness(Index) - 0.5) * 2., 0., 1.) * (Cells[Index].IceCubicMeters > 0.001 ? 0. : 1.);
}

bool FKSISurfaceSimulation::Initialize(FGuid InWorldId, double InGameSeconds, double InWorldSeconds, uint64 InStepId, FString& Error)
{
	if (!InWorldId.IsValid() || !Quantity(InGameSeconds) || !Quantity(InWorldSeconds))
	{
		return Fail(Error, TEXT("Surface simulation requires valid world and completed times."));
	}

	Air = {};
	Regions.Reset();
	SnowContacts.Reset();
	WorldId = InWorldId;
	GameSeconds = InGameSeconds;
	WorldSeconds = InWorldSeconds;
	StepId = InStepId;
	return true;
}

bool FKSISurfaceSimulation::AddRegion(FKSISurfaceGrid Region, FString& Error)
{
	if (!IsInitialized() || Regions.Contains(Region.RegionId) || Regions.Num() >= MaximumRegions || (!ValidateRegion(Region, Error) || !Region.Vegetation.ValidateAt(WorldSeconds, Error)))
	{
		return Fail(Error, TEXT("Surface region is invalid, duplicated or over budget."));
	}

	int32 Total = Region.Cells.Num();
	for (const auto& Pair : Regions)
	{
		Total += Pair.Value.Cells.Num();
	}
	if (Total > MaximumCells)
	{
		return Fail(Error, TEXT("Surface cell budget exceeded."));
	}

	Regions.Add(Region.RegionId, MoveTemp(Region));
	return true;
}

bool FKSISurfaceSimulation::ChangeForcing(FGuid RegionId, const FKSISurfaceForcing& Forcing, FString& Error)
{
	auto* Region = Regions.Find(RegionId);
	if (!Region || !ValidForcing(Forcing) || Region->Revision == MAX_uint64)
	{
		return Fail(Error, TEXT("Invalid surface forcing or exhausted revision."));
	}

	Region->Forcing = Forcing;
	Region->Climate.bEnabled = 0;
	++Region->Revision;
	return true;
}

bool FKSISurfaceSimulation::AddAirSpace(const FKSIAirSpace& Space, FString& Error)
{
	if (!IsInitialized() || (Space.ClimateRegionId.IsValid() && !Regions.Contains(Space.ClimateRegionId)))
	{
		return Fail(Error, TEXT("Air space requires an initialized world and existing climate region."));
	}
	return Air.AddSpace(Space, Error);
}

bool FKSISurfaceSimulation::Ignite(FGuid RegionId, int32 CellIndex, double HeatJoules, FString& Error)
{
	auto* Region = Regions.Find(RegionId);
	if (!Region || Region->Revision == MAX_uint64 || !Region->Fire.Cells.IsValidIndex(CellIndex))
	{
		return Fail(Error, TEXT("Ignition requires an existing fuel cell and available revision."));
	}
	if (!FKSICombustionModel::AddHeat(Region->Fire.Material, HeatJoules, Region->Fire.Cells[CellIndex], Error))
	{
		return false;
	}
	++Region->Revision;
	return true;
}

bool FKSISurfaceSimulation::ChangeClimate(FGuid RegionId, const FKSIClimate& Climate, FString& Error)
{
	auto* Region = Regions.Find(RegionId);
	if (!Region || Region->Revision == MAX_uint64 || !FKSIWeatherModel::Validate(Climate, Region->Weather, Error))
	{
		Error = TEXT("Invalid weather region, climate or exhausted revision.");
		return false;
	}

	Region->Climate = Climate;
	++Region->Revision;
	return true;
}

bool FKSISurfaceSimulation::ChangeWind(FGuid RegionId, double MetersPerSecond, FString& Error)
{
	auto* Region = Regions.Find(RegionId);
	if (!Region || Region->Revision == MAX_uint64 || !FMath::IsFinite(MetersPerSecond) || MetersPerSecond < 0. || MetersPerSecond > 100.)
	{
		return Fail(Error, TEXT("Wind requires an existing region and a speed between 0 and 100 m/s."));
	}

	Region->Weather.WindMetersPerSecond = MetersPerSecond;
	Region->Climate.PrevailingWindMetersPerSecond = MetersPerSecond;
	++Region->Revision;
	return true;
}

bool FKSISurfaceSimulation::Advance(const FKSIWorldStep& Step, FString& Error,
	const FKSICelestialSystem* Celestial, const FKSICelestialObserver* Observer, const IKSISurfaceProvider* Shelter,
	const TArray<FKSISurfaceOpening>* Openings)
{
	Error.Reset();
	if (!Validate(Error) || Step.StepId != StepId + 1 || StepId == MAX_uint64
		|| Step.GameFromSeconds != GameSeconds || Step.WorldFromSeconds != WorldSeconds
		|| !Quantity(Step.GameToSeconds) || !Quantity(Step.WorldToSeconds)
		|| Step.GameToSeconds <= GameSeconds || Step.WorldToSeconds < WorldSeconds)
	{
		return Fail(Error, TEXT("Surface step does not match its committed world clock."));
	}
	const double Delta = Step.GameToSeconds - GameSeconds;
	const double WorldDelta = Step.WorldToSeconds - WorldSeconds;
	const bool bAir = !Air.GetSpaces().IsEmpty();
	const double StepCount = FMath::CeilToDouble(Delta / 0.25);
	if ((!Regions.IsEmpty() || bAir) && StepCount > MaximumSubsteps)
	{
		return Fail(Error, TEXT("Surface substep budget exceeded; world time remains pending."));
	}
	bool bNatural = false;
	for (const auto& Pair : Regions)
	{
		bNatural |= Pair.Value.Climate.bEnabled != 0 || !Pair.Value.Vegetation.Cells.IsEmpty();
	}
	const double Slices = bAir ? FMath::Max(StepCount, bNatural ? FMath::CeilToDouble(WorldDelta / 60.) : 1.) : 1.;
	if (Slices > MaximumSubsteps || (bAir && !Openings))
	{
		return Fail(Error, TEXT("Air coupling requires bounded common steps and explicit opening context."));
	}
	FKSISurfaceSimulation Candidate = *this;
	for (int32 I = 0; I < int32(Slices); ++I)
	{
		if (!Candidate.AdvanceSlice(Delta / Slices, WorldSeconds + I * WorldDelta / Slices,
			WorldDelta / Slices, Error, Celestial, Observer, Shelter, Openings))
		{
			return false;
		}
	}
	Candidate.GameSeconds = Step.GameToSeconds;
	Candidate.WorldSeconds = Step.WorldToSeconds;
	Candidate.StepId = Step.StepId;
	if (!Candidate.Validate(Error))
	{
		return false;
	}
	*this = MoveTemp(Candidate);
	return true;
}

bool FKSISurfaceSimulation::AdvanceSlice(double GameDelta, double WorldFrom, double WorldDelta, FString& Error,
	const FKSICelestialSystem* Celestial, const FKSICelestialObserver* Observer,
	const IKSISurfaceProvider* Shelter, const TArray<FKSISurfaceOpening>* Openings)
{
	const FKSIAirSimulation PreviousAir = Air;
	FKSISurfaceThermalContext Context;
	Context.PreviousAir = &PreviousAir;
	Context.Shelter = Shelter;
	Context.Observer = Observer;
	for (const auto& Pair : Regions)
	{
		for (int32 I = 0; I < Pair.Value.Cells.Num(); ++I)
		{
			if (const auto* Room = PreviousAir.FindSpaceAt(FKSISurfaceThermalContext::CellPosition(Pair.Value, I)))
			{
				++Context.ContactCounts.FindOrAdd(Room->SpaceId);
			}
		}
	}
	const double WeatherCount = FMath::Max(1., FMath::CeilToDouble(WorldDelta / 60.));
	TArray<double> Irradiance, Daylight;
	for (const auto& Pair : Regions)
	{
		if ((!Pair.Value.Climate.bEnabled && Pair.Value.Vegetation.Cells.IsEmpty()) || WorldDelta == 0.)
		{
			continue;
		}
		if (!Celestial || !Observer || WeatherCount > MaximumSubsteps)
		{
			return Fail(Error, TEXT("Natural weather requires celestial context and a bounded world-time budget."));
		}

		Irradiance.Reserve(int32(WeatherCount));
		for (int32 I = 0; I < int32(WeatherCount); ++I)
		{
			FKSICelestialObservation Observation;
			if (!Celestial->Observe(WorldFrom + (I + 0.5) * WorldDelta / WeatherCount, *Observer, Observation, Error))
			{
				return false;
			}
			Irradiance.Add(Observation.TotalHorizontalIrradianceWattsPerM2);
			const auto* Star = Observation.Stars.FindByPredicate([&](const FKSIStellarObservation& S)
			{
				return S.BodyId == Observation.DominantStarId;
			});
			double Hours = 0.;
			if (Star)
			{
				const double Latitude = FMath::DegreesToRadians(FMath::Clamp(Observer->LatitudeDegrees, -89.999, 89.999));
				const double Declination = FMath::DegreesToRadians(Star->DeclinationDegrees);
				const double CosHourAngle = -FMath::Tan(Latitude) * FMath::Tan(Declination);
				Hours = 24. * FMath::Acos(FMath::Clamp(CosHourAngle, -1., 1.)) / UE_DOUBLE_PI;
			}

			Daylight.Add(Hours);
		}
		break;
	}

	TArray<FGuid> RegionIds;
	Regions.GetKeys(RegionIds);
	RegionIds.Sort(GuidLess);
	for (FGuid RegionId : RegionIds)
	{
		auto& G = Regions[RegionId];
		if (G.bDynamicShelter)
		{
			if (!Shelter || !Observer || !Celestial)
			{
				return Fail(Error, TEXT("Dynamic shelter requires surface and celestial context."));
			}
			FKSICelestialObservation Observation;
			if (!Celestial->Observe(WorldFrom + WorldDelta * 0.5, *Observer, Observation, Error))
			{
				return false;
			}
			FKSIShelterQuery Query;
			Query.BodyId = Observer->BodyId;
			Query.RequiredRevision = Shelter->GetRevision();
			Query.RequiredEpoch = Shelter->GetEpoch();
			Query.ToWindSource = FVector::ForwardVector;
			if (!Observation.Stars.IsEmpty())
			{
				Query.ToSun = Observation.Stars[0].LocalDirection;
			}
			for (int32 Index = 0; Index < G.Cells.Num(); ++Index)
			{
				auto& Cell = G.Cells[Index];
				Query.PositionMeters = FVector(G.OriginMeters.X + (Index % G.Size.X + 0.5) * G.SpacingMeters,
					G.OriginMeters.Y + (Index / G.Size.X + 0.5) * G.SpacingMeters,
					Cell.BedMeters + G.SnowDepthMeters(Index) + 0.01);
				FKSIShelterSample Sample;
				if (!FKSIShelterEvaluator::Evaluate(*Shelter, Query, Sample, Error))
				{
					return false;
				}
				Cell.RainExposure = Sample.Transmission.Precipitation;
				Cell.SunExposure = Sample.Transmission.Sun;
				Cell.WindExposure = Sample.Transmission.Wind;
			}
		}
		const bool bNatural = G.Climate.bEnabled && WorldDelta > 0.;
		const bool bVegetation = !G.Vegetation.Cells.IsEmpty() && WorldDelta > 0.;
		const int32 Count = bNatural || bVegetation ? Irradiance.Num() : 1;
		const double GameDt = GameDelta / Count;
		const double WorldDt = WorldDelta / Count;
		const int32 FlowSteps = FMath::CeilToInt32(GameDt / 0.25);
		for (int32 I = 0; I < Count; ++I)
		{
			if (bNatural)
			{
				if (!FKSIWeatherModel::Advance(G.Climate, WorldFrom + I * WorldDt, WorldDt,
					Irradiance[I], G.Weather, Error))
				{
					return false;
				}
				G.Forcing.TemperatureCelsius = G.Weather.TemperatureCelsius;
				G.Forcing.RainMetersPerWorldSecond = G.Weather.RainMetersPerSecond;
				G.Forcing.SnowMetersPerWorldSecond = G.Weather.SnowMetersPerSecond;
				G.Forcing.EvaporationMetersPerWorldSecond = G.Weather.EvaporationMetersPerSecond;
			}

			const double Sun = bNatural ? G.Weather.IrradianceWattsPerM2 : bVegetation ? Irradiance[I] : 0.;
			const double Hours = bVegetation ? Daylight[I] : 0.;
			if (G.Revision == MAX_uint64 || !AdvanceGrid(G, GameDt, WorldFrom + I * WorldDt, WorldDt, FlowSteps, Sun, Hours, Error, Context))
			{
				return false;
			}
		}
	}

	if (!Air.GetSpaces().IsEmpty())
	{
		TArray<FGuid> SpaceIds;
		Air.GetSpaces().GetKeys(SpaceIds);
		SpaceIds.Sort(GuidLess);
		TMap<FGuid, FKSIAirExterior> Exteriors;
		BuildAirExteriors(Exteriors);
		for (FGuid Id : SpaceIds)
		{
			const auto Source = Context.Sources.FindRef(Id);
			if (!Air.AddSource(Id, Source.HeatJoules, Source.VaporKg, Source.SmokeKg, Error))
			{
				return false;
			}
		}
		TArray<FKSISurfaceOpening> EffectiveOpenings = *Openings;
		if (Shelter)
		{
			for (auto& Opening : EffectiveOpenings)
			{
				if (Air.FindSpace(Opening.SpaceA) || Air.FindSpace(Opening.SpaceB))
				{
					if (!Shelter->QueryOpeningFraction(Opening, Opening.OpenFraction, Error))
					{
						return false;
					}
				}
			}
		}

		if (!Air.Advance(GameDelta, EffectiveOpenings, Exteriors, Error))
		{
			return false;
		}
	}
	return true;
}

void FKSISurfaceSimulation::BuildAirExteriors(TMap<FGuid, FKSIAirExterior>& Exteriors) const
{
	for (const auto& Pair : Air.GetSpaces())
	{
		FKSIAirExterior Outside;
		if (const auto* Region = Regions.Find(Pair.Value.ClimateRegionId))
		{
			Outside.TemperatureCelsius = Region->Forcing.TemperatureCelsius;
			// Liquid-equivalent weather column / 1000 m mixing height, with 1000 kg/m3 water.
			Outside.VaporKgPerCubicMeter = Region->Weather.VaporMeters;
			Outside.WindMetersPerSecond = Region->Weather.WindMetersPerSecond;
		}
		Exteriors.Add(Pair.Key, Outside);
	}
}

bool FKSISurfaceSimulation::ValidateAirContext(const TArray<FKSISurfaceOpening>& Openings, FString& Error) const
{
	if (Air.GetSpaces().IsEmpty())
	{
		return true;
	}
	FKSIAirSimulation Candidate = Air;
	TMap<FGuid, FKSIAirExterior> Exteriors;
	BuildAirExteriors(Exteriors);
	return Candidate.Advance(0., Openings, Exteriors, Error);
}

bool FKSISurfaceSimulation::RebaseTerrain(FGuid RegionId, const TArray<double>& BedsMeters, uint64 TerrainRevision, FString& Error, bool bRestoring)
{
	const auto* Existing = Regions.Find(RegionId);
	if (!Existing || BedsMeters.Num() != Existing->Cells.Num() || (!bRestoring && TerrainRevision <= Existing->TerrainRevision) || TerrainRevision == 0 || Existing->Revision == MAX_uint64)
	{
		return Fail(Error, TEXT("Terrain rebase requires the complete newer surface geometry."));
	}

	FKSISurfaceGrid Candidate = *Existing;
	for (int32 I = 0; I < BedsMeters.Num(); ++I)
	{
		if (!FMath::IsFinite(BedsMeters[I]) || BedsMeters[I] > Candidate.Cells[I].CeilingMeters || FMath::Abs(BedsMeters[I]) > 1.e7)
		{
			return Fail(Error, TEXT("New terrain bed is outside its explicit surface storage."));
		}
		KSISurfaceVegetation::RemoveSupport(Candidate, I, BedsMeters[I], WorldSeconds);
		Candidate.Cells[I].BedMeters = BedsMeters[I];
	}

	for (int32 I = 0; I < Candidate.Cells.Num(); ++I)
	{
		auto& Source = Candidate.Cells[I];
		const double Room = Capacity(Candidate, Source) - Source.IceCubicMeters - Source.SnowVolumeCubicMeters;
		if (Room < 0.)
		{
			return Fail(Error, TEXT("Terrain would remove the support volume of persistent ice."));
		}

		double Excess = FMath::Max(0., Source.WaterCubicMeters - Room);
		Source.WaterCubicMeters -= Excess;
		TArray<int32> Queue = {I};
		TSet<int32> Seen = {I};
		for (int32 Head = 0; Head < Queue.Num() && Excess > 0.; ++Head)
		{
			const int32 At = Queue[Head];
			auto& Target = Candidate.Cells[At];
			if (At != I)
			{
				const double Amount = FMath::Min(Excess, FMath::Max(0., Capacity(Candidate, Target) - Target.WaterCubicMeters - Target.IceCubicMeters - Target.SnowVolumeCubicMeters));
				Target.WaterCubicMeters += Amount;
				Excess -= Amount;
			}
			if (Target.bOpenBoundary && Excess > 0.)
			{
				Candidate.Ledger.BoundaryOutCubicMeters += Excess;
				Excess = 0.;
			}

			const int32 X = At % Candidate.Size.X;
			const int32 Y = At / Candidate.Size.X;
			for (const FIntPoint Offset : {FIntPoint(-1, 0), FIntPoint(1, 0), FIntPoint(0, -1), FIntPoint(0, 1)})
			{
				const int32 NX = X + Offset.X, NY = Y + Offset.Y;
				if (NX >= 0 && NY >= 0 && NX < Candidate.Size.X && NY < Candidate.Size.Y)
				{
					const int32 Next = NX + NY * Candidate.Size.X;
					if (!Seen.Contains(Next) && FMath::Min(Target.CeilingMeters, Candidate.Cells[Next].CeilingMeters) > FMath::Max(Target.BedMeters, Candidate.Cells[Next].BedMeters))
					{
						Seen.Add(Next);
						Queue.Add(Next);
					}
				}
			}
		}
		if (Excess > 0.)
		{
			return Fail(Error, TEXT("Terrain displacement has no connected capacity or external outlet."));
		}
	}

	Candidate.TerrainRevision = TerrainRevision;
	++Candidate.Revision;
	if (!ValidateRegion(Candidate, Error))
	{
		return false;
	}
	Regions[RegionId] = MoveTemp(Candidate);
	return true;
}

bool FKSISurfaceSimulation::Capture(TArray<uint8>& Bytes, FString& Error) const
{
	if (!Validate(Error))
	{
		return false;
	}

	FKSISurfaceSimulation Copy = *this;
	TArray<uint8> Candidate;
	FMemoryWriter Writer(Candidate, true);
	if (!Copy.Serialize(Writer) || Candidate.Num() > MaximumBytes - 4)
	{
		return Fail(Error, TEXT("Surface snapshot exceeds its byte budget."));
	}
	uint32 CRC = FCrc::MemCrc32(Candidate.GetData(), Candidate.Num());
	Writer << CRC;
	Bytes = MoveTemp(Candidate);
	return true;
}

bool FKSISurfaceSimulation::Restore(const TArray<uint8>& Bytes, FString& Error)
{
	if (Bytes.Num() < 56 || Bytes.Num() > MaximumBytes)
	{
		return Fail(Error, TEXT("Invalid surface snapshot length."));
	}

	uint32 CRC;
	FMemory::Memcpy(&CRC, Bytes.GetData() + Bytes.Num() - 4, 4);
	if (CRC != FCrc::MemCrc32(Bytes.GetData(), Bytes.Num() - 4))
	{
		return Fail(Error, TEXT("Surface snapshot checksum mismatch."));
	}

	TArray<uint8> Payload;
	Payload.Append(Bytes.GetData(), Bytes.Num() - 4);
	FMemoryReader Reader(Payload, true);
	FKSISurfaceSimulation Candidate;
	if (!Candidate.Serialize(Reader) || Reader.Tell() != Payload.Num() || !Candidate.Validate(Error))
	{
		return Fail(Error, TEXT("Malformed surface snapshot."));
	}
	*this = MoveTemp(Candidate);
	return true;
}

bool FKSISurfaceSimulation::Validate(FString& Error) const
{
	if (!WorldId.IsValid() || !Quantity(GameSeconds) || !Quantity(WorldSeconds) || Regions.Num() > MaximumRegions || !Air.Validate(Error))
	{
		return Fail(Error, TEXT("Invalid surface world identity or time."));
	}

	for (const auto& Pair : Air.GetSpaces())
	{
		if (Pair.Value.ClimateRegionId.IsValid() && !Regions.Contains(Pair.Value.ClimateRegionId))
		{
			return Fail(Error, TEXT("Air space climate region is missing."));
		}
	}
	int32 Count = 0;
	for (const auto& Pair : Regions)
	{
		Count += Pair.Value.Cells.Num();
		if (Pair.Key != Pair.Value.RegionId || !ValidateRegion(Pair.Value, Error) || !Pair.Value.Vegetation.ValidateAt(WorldSeconds, Error) || Count > MaximumCells)
		{
			return false;
		}
	}
	return true;
}

bool FKSISurfaceSimulation::ValidateRegion(const FKSISurfaceGrid& G, FString& Error)
{
	if (!G.RegionId.IsValid() || G.Size.X < 1 || G.Size.Y < 1 || G.Size.X > 128 || G.Size.Y > 128
		|| G.Cells.Num() != G.Size.X * G.Size.Y || G.OriginMeters.ContainsNaN() || G.OriginMeters.GetAbsMax() > 1.e7
		|| !FMath::IsFinite(G.SpacingMeters) || G.SpacingMeters < 0.05 || G.SpacingMeters > 16.
		|| G.Revision == 0 || G.bDynamicShelter > 1 || (G.TerrainId.IsValid() && G.TerrainRevision == 0) || !Quantity(G.AirHeatToSurfaceJoules) || !Quantity(G.SurfaceHeatToAirJoules)
		|| !G.Fire.Validate(G.Cells.Num(), Error) || !G.Vegetation.Validate(G, Error) || !ValidForcing(G.Forcing) || !FKSIWeatherModel::Validate(G.Climate, G.Weather, Error))
	{
		return Fail(Error, TEXT("Invalid surface region definition."));
	}

	for (const auto& Cell : G.Cells)
	{
		if (!FMath::IsFinite(Cell.BedMeters) || !FMath::IsFinite(Cell.CeilingMeters)
			|| FMath::Abs(Cell.BedMeters) > 1.e7 || FMath::Abs(Cell.CeilingMeters) > 1.e7 || Cell.CeilingMeters < Cell.BedMeters
			|| !Quantity(Cell.SnowCubicMeters) || !Quantity(Cell.SnowVolumeCubicMeters)
			|| Cell.SnowVolumeCubicMeters < Cell.SnowCubicMeters / 0.65 - 1.e-12
			|| Cell.SnowVolumeCubicMeters > Cell.SnowCubicMeters / 0.05 + 1.e-12
			|| !Quantity(Cell.WaterCubicMeters) || !Quantity(Cell.IceCubicMeters) || !Quantity(Cell.SoilCubicMeters)
			|| Cell.WaterCubicMeters + Cell.IceCubicMeters + Cell.SnowVolumeCubicMeters > Capacity(G, Cell) + 1.e-8
			|| !FMath::IsFinite(Cell.RainExposure) || Cell.RainExposure < 0. || Cell.RainExposure > 1. || Cell.bOpenBoundary > 1
			|| !FMath::IsFinite(Cell.SunExposure) || Cell.SunExposure < 0. || Cell.SunExposure > 1.
			|| !FMath::IsFinite(Cell.WindExposure) || Cell.WindExposure < 0. || Cell.WindExposure > 1.)
		{
			Error = FString::Printf(TEXT("Invalid surface cell: water=%.17g ice=%.17g soil=%.17g capacity=%.17g bed=%.17g ceiling=%.17g"),
				Cell.WaterCubicMeters, Cell.IceCubicMeters, Cell.SoilCubicMeters, Capacity(G, Cell), Cell.BedMeters, Cell.CeilingMeters);
			return false;
		}
	}

	const auto& L = G.Ledger;
	if (!Quantity(L.InitialCubicMeters) || !Quantity(L.RainCubicMeters) || !Quantity(L.SnowCubicMeters) || !Quantity(L.BoundaryInCubicMeters)
		|| !Quantity(L.BoundaryOutCubicMeters) || !Quantity(L.EvaporatedCubicMeters)
		|| FMath::Abs(G.BalanceErrorCubicMeters()) > FMath::Max(1.e-8, G.TotalCubicMeters() * 1.e-8))
	{
		return Fail(Error, TEXT("Surface water ledger does not balance."));
	}
	return true;
}

bool FKSISurfaceSimulation::Serialize(FArchive& Ar)
{
	uint32 Tag = Magic, Version = 7;
	Ar << Tag << Version << WorldId << GameSeconds << WorldSeconds << StepId;
	int32 Count = Regions.Num();
	Ar << Count;
	if (Ar.IsError() || Tag != Magic || (Version < 1 || Version > 7) || Count < 0 || Count > MaximumRegions)
	{
		return false;
	}

	TArray<FGuid> Ids;
	Regions.GetKeys(Ids);
	Ids.Sort(GuidLess);
	int32 TotalCells = 0;
	for (int32 I = 0; I < Count; ++I)
	{
		FKSISurfaceGrid G;
		if (Ar.IsSaving())
		{
			G = Regions[Ids[I]];
		}
		Ar << G.RegionId << G.TerrainId << G.OriginMeters << G.Size << G.SpacingMeters << G.Revision << G.TerrainRevision;
		auto& F = G.Forcing;
		Ar << F.RainMetersPerWorldSecond << F.EvaporationMetersPerWorldSecond << F.InfiltrationMetersPerWorldSecond
			<< F.DrainageMetersPerWorldSecond << F.SoilCapacityMeters << F.TemperatureCelsius << F.PhaseMetersPerDegreeWorldSecond
			<< F.FlowConductance << F.BoundaryLevelMeters << F.BoundaryConductance;
		if (Version >= 2)
		{
			Ar << F.SnowMetersPerWorldSecond;
		}
		if (Version >= 3)
		{
			FKSIWeatherModel::Serialize(Ar, G.Climate, G.Weather);
		}
		if (Version >= 4)
		{
			Ar << G.bDynamicShelter;
		}
		auto& L = G.Ledger;
		Ar << L.InitialCubicMeters << L.RainCubicMeters << L.BoundaryInCubicMeters << L.BoundaryOutCubicMeters << L.EvaporatedCubicMeters;
		if (Version >= 2)
		{
			Ar << L.SnowCubicMeters;
		}
		int32 Cells = G.Cells.Num();
		Ar << Cells;
		if (Ar.IsError() || Cells < 1 || Cells > 16384 || TotalCells > MaximumCells - Cells
			|| (Ar.IsLoading() && int64(Cells) * (Version >= 4 ? 81 : Version >= 2 ? 65 : 49) > Ar.TotalSize() - Ar.Tell()))
		{
			return false;
		}
		TotalCells += Cells;
		G.Cells.SetNum(Cells);
		for (auto& C : G.Cells)
		{
			Ar << C.BedMeters << C.CeilingMeters << C.WaterCubicMeters << C.IceCubicMeters << C.SoilCubicMeters
				<< C.RainExposure << C.bOpenBoundary;
			if (Version >= 2)
			{
				Ar << C.SnowCubicMeters << C.SnowVolumeCubicMeters;
			}
			if (Version >= 4)
			{
				Ar << C.SunExposure << C.WindExposure;
			}
		}
		if (Version >= 5 && !G.Fire.Serialize(Ar, Cells, Version >= 7))
		{
			return false;
		}
		if (Version >= 6)
		{
			Ar << G.AirHeatToSurfaceJoules << G.SurfaceHeatToAirJoules;
		}
		if (Version >= 7 && !G.Vegetation.Serialize(Ar, Cells))
		{
			return false;
		}

		if (Ar.IsLoading())
		{
			if (Regions.Contains(G.RegionId))
			{
				return false;
			}
			Regions.Add(G.RegionId, MoveTemp(G));
		}
	}
	if (Version >= 2)
	{
		int32 ContactCount = SnowContacts.Num();
		Ar << ContactCount;
		if (ContactCount < 0 || ContactCount > 128 || (Ar.IsLoading() && int64(ContactCount) * 24 > Ar.TotalSize() - Ar.Tell()))
		{
			return false;
		}
		TArray<FGuid> Sources;
		SnowContacts.GetKeys(Sources);
		Sources.Sort(GuidLess);
		for (int32 I = 0; I < ContactCount; ++I)
		{
			FGuid Id;
			uint64 Sequence = 0;
			if (Ar.IsSaving())
			{
				Id = Sources[I];
				Sequence = SnowContacts[Id];
			}
			Ar << Id << Sequence;
			if (!Id.IsValid() || Sequence == 0 || (Ar.IsLoading() && SnowContacts.Contains(Id)))
			{
				return false;
			}
			if (Ar.IsLoading())
			{
				SnowContacts.Add(Id, Sequence);
			}
		}
	}
	return !Ar.IsError() && (Version < 6 || Air.Serialize(Ar));
}
