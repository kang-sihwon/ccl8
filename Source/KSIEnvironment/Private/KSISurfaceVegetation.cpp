#include "KSISurfaceVegetation.h"

#include "KSISurfaceSimulation.h"

namespace
{
	bool Range(double V, double Lo, double Hi)
	{
		return FMath::IsFinite(V) && V >= Lo && V <= Hi;
	}

	bool Near(double A, double B)
	{
		return FMath::Abs(A - B) <= FMath::Max(1.e-8, FMath::Max(FMath::Abs(A), FMath::Abs(B)) * 1.e-8);
	}

	bool Fail(FString& Error, const TCHAR* Reason)
	{
		Error = Reason;
		return false;
	}

	bool ValidSpecies(const FKSIVegetationSpecies& S)
	{
		return S.Id.IsValid() && uint8(S.Kind) <= uint8(EKSIVegetationKind::Fungus) && S.bRegrows <= 1
			&& Range(S.MaximumBiomassKg, 0.000001, 100000.) && Range(S.GrowthKgPerWorldSecond, 0., 1000.)
			&& Range(S.WaterCubicMetersPerKg, 0.000000001, 100.) && Range(S.NutrientKgPerBiomassKg, 0.000000001, 1.)
			&& Range(S.MinimumWetness, 0., 1.) && Range(S.OptimalWetness, S.MinimumWetness + 0.000001, 1.)
			&& Range(S.FullGrowthIrradiance, 1., 100000.) && Range(S.MinimumDaylightHours, 0., 24.)
			&& Range(S.MinimumTemperatureCelsius, -100., 100.) && Range(S.MaximumTemperatureCelsius, S.MinimumTemperatureCelsius, 100.)
			&& Range(S.LethalTemperatureCelsius, S.MaximumTemperatureCelsius + 0.000001, 2000.)
			&& Range(S.RootKillTemperatureCelsius, S.LethalTemperatureCelsius, 2000.)
			&& Range(S.StressDeathSeconds, 1., 1.e12) && Range(S.LifespanSeconds, 1., 1.e12)
			&& Range(S.RegrowthDelaySeconds, 1., 1.e12) && Range(S.DecayPerWorldSecond, 0., 1.)
			&& Range(S.MaximumSupportChangeMeters, 0., 100.);
	}

	void Die(FKSIVegetationCell& C, const FKSIVegetationSpecies& S, double Time, bool bKillRoots)
	{
		if (C.State != EKSIVegetationState::Dead)
		{
			C.DeadBiomassKg += C.LivingBiomassKg;
			C.LivingBiomassKg = 0.;
			C.State = EKSIVegetationState::Dead;
			C.DeathWorldSeconds = Time;
			C.StressWorldSeconds = 0.;
		}

		if (bKillRoots || !S.bRegrows)
		{
			C.bRootsViable = 0;
		}
	}

	bool Decay(FKSISurfaceGrid& G, int32 I, const FKSIVegetationSpecies& S, double Dt, FString& Error)
	{
		auto& C = G.Vegetation.Cells[I];
		const double Amount = FMath::Min(C.DeadBiomassKg * (1. - FMath::Exp(-S.DecayPerWorldSecond * Dt)), G.Fire.Cells[I].FuelKg);
		if (!FKSICombustionModel::TransferFuel(G.Fire.Material, -Amount, G.Fire.Cells[I], Error))
		{
			return false;
		}

		C.DeadBiomassKg -= Amount;
		C.DecayedBiomassKg += Amount;
		C.SoilOrganicKg += Amount;
		C.SoilNutrientsKg += Amount * S.NutrientKgPerBiomassKg;
		return true;
	}
}

bool FKSISurfaceVegetationLayer::Validate(const FKSISurfaceGrid& G, FString& Error) const
{
	if (Cells.IsEmpty())
	{
		return Species.IsEmpty() || Fail(Error, TEXT("Empty vegetation layer cannot retain unbound species."));
	}

	if (Cells.Num() != G.Cells.Num() || G.Fire.Cells.Num() != Cells.Num() || Species.Num() > 32)
	{
		return Fail(Error, TEXT("Vegetation requires matching surface/fuel cells and bounded species."));
	}

	TSet<FGuid> SpeciesIds, PlantIds;
	for (const auto& S : Species)
	{
		if (!ValidSpecies(S) || SpeciesIds.Contains(S.Id))
		{
			return Fail(Error, TEXT("Invalid or duplicate vegetation species."));
		}

		SpeciesIds.Add(S.Id);
	}

	for (int32 I = 0; I < Cells.Num(); ++I)
	{
		const auto& C = Cells[I];
		const double Quantities[] = {C.InitialBiomassKg, C.LivingBiomassKg, C.DeadBiomassKg, C.GrownBiomassKg,
			C.BurnedBiomassKg, C.DecayedBiomassKg, C.SoilNutrientsKg, C.InitialNutrientsKg, C.SoilOrganicKg,
			C.InitialOrganicKg, C.FungalConsumptionKg, C.TranspiredCubicMeters};
		for (double Q : Quantities)
		{
			if (!Range(Q, 0., 1.e9))
			{
				return Fail(Error, TEXT("Vegetation quantity exceeds its finite nonnegative budget."));
			}
		}

		if (uint8(C.State) > uint8(EKSIVegetationState::Dead) || C.bRootsViable > 1
			|| !Range(C.CycleStartedWorldSeconds, 0., 1.e12) || !Range(C.StressWorldSeconds, 0., 1.e12)
			|| !Range(C.DeathWorldSeconds, -1., 1.e12))
		{
			return Fail(Error, TEXT("Invalid vegetation time or lifecycle state."));
		}

		const double Biomass = C.LivingBiomassKg + C.DeadBiomassKg;
		const auto& Fire = G.Fire.Cells[I];
		if (!Near(C.InitialBiomassKg + C.GrownBiomassKg, Biomass + C.BurnedBiomassKg + C.DecayedBiomassKg)
			|| !Near(C.InitialOrganicKg + C.DecayedBiomassKg, C.SoilOrganicKg + C.FungalConsumptionKg)
			|| !Near(Fire.InitialFuelKg, C.InitialBiomassKg) || !Near(Fire.AddedFuelKg, C.GrownBiomassKg)
			|| !Near(Fire.RemovedFuelKg, C.DecayedBiomassKg) || !Near(Fire.BurnedKg, C.BurnedBiomassKg)
			|| !Near(Fire.FuelKg, Biomass))
		{
			return Fail(Error, TEXT("Vegetation biomass, soil organic matter or fuel does not balance."));
		}

		if (C.SpeciesIndex == INDEX_NONE)
		{
			if (C.Id.IsValid() || C.State != EKSIVegetationState::Empty || C.bRootsViable
				|| C.InitialBiomassKg != 0. || C.GrownBiomassKg != 0. || !Near(C.InitialNutrientsKg, C.SoilNutrientsKg))
			{
				return Fail(Error, TEXT("Empty vegetation cell has an identity or unexplained biomass."));
			}

			continue;
		}

		if (!Species.IsValidIndex(C.SpeciesIndex) || !C.Id.IsValid() || PlantIds.Contains(C.Id))
		{
			return Fail(Error, TEXT("Invalid vegetation identity or species index."));
		}

		PlantIds.Add(C.Id);
		const auto& S = Species[C.SpeciesIndex];
		if (!Near(C.InitialNutrientsKg, C.SoilNutrientsKg + Biomass * S.NutrientKgPerBiomassKg)
			|| C.LivingBiomassKg > S.MaximumBiomassKg + 1.e-9 || C.State == EKSIVegetationState::Empty
			|| (C.State == EKSIVegetationState::Dead && (C.LivingBiomassKg != 0. || C.DeathWorldSeconds < C.CycleStartedWorldSeconds))
			|| (C.State != EKSIVegetationState::Dead && C.DeathWorldSeconds != -1.)
			|| (C.bRootsViable && !S.bRegrows) || (S.Kind != EKSIVegetationKind::Fungus && C.FungalConsumptionKg != 0.))
		{
			return Fail(Error, TEXT("Vegetation nutrients or life phase are inconsistent."));
		}
	}

	return true;
}

bool FKSISurfaceVegetationLayer::ValidateAt(double WorldSeconds, FString& Error) const
{
	for (const auto& C : Cells)
	{
		if (C.SpeciesIndex == INDEX_NONE)
		{
			continue;
		}

		if (!Species.IsValidIndex(C.SpeciesIndex) || C.CycleStartedWorldSeconds > WorldSeconds
			|| C.DeathWorldSeconds > WorldSeconds
			|| (C.State != EKSIVegetationState::Dead && C.CycleStartedWorldSeconds + Species[C.SpeciesIndex].LifespanSeconds <= WorldSeconds))
		{
			return Fail(Error, TEXT("Vegetation life state disagrees with the committed world time."));
		}
	}

	return true;
}

bool FKSISurfaceVegetationLayer::Serialize(FArchive& Ar, int32 SurfaceCells)
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

	int32 Count = Species.Num();
	Ar << Count;
	if (Ar.IsError() || Count < 0 || Count > 32 || (Ar.IsLoading() && int64(Count) * 154 > Ar.TotalSize() - Ar.Tell()))
	{
		return false;
	}

	Species.SetNum(Count);
	for (auto& S : Species)
	{
		Ar << S.Id << S.Kind << S.MaximumBiomassKg << S.GrowthKgPerWorldSecond << S.WaterCubicMetersPerKg
			<< S.NutrientKgPerBiomassKg << S.MinimumWetness << S.OptimalWetness << S.FullGrowthIrradiance
			<< S.MinimumDaylightHours << S.MinimumTemperatureCelsius << S.MaximumTemperatureCelsius
			<< S.LethalTemperatureCelsius << S.RootKillTemperatureCelsius << S.StressDeathSeconds
			<< S.LifespanSeconds << S.RegrowthDelaySeconds << S.DecayPerWorldSecond << S.MaximumSupportChangeMeters << S.bRegrows;
	}

	Count = Cells.Num();
	Ar << Count;
	if (Ar.IsError() || Count != SurfaceCells || Count < 1 || Count > 16384
		|| (Ar.IsLoading() && int64(Count) * 146 > Ar.TotalSize() - Ar.Tell()))
	{
		return false;
	}

	Cells.SetNum(Count);
	for (auto& C : Cells)
	{
		Ar << C.Id << C.SpeciesIndex << C.InitialBiomassKg << C.LivingBiomassKg << C.DeadBiomassKg
			<< C.GrownBiomassKg << C.BurnedBiomassKg << C.DecayedBiomassKg << C.SoilNutrientsKg << C.InitialNutrientsKg
			<< C.SoilOrganicKg << C.InitialOrganicKg << C.FungalConsumptionKg << C.TranspiredCubicMeters
			<< C.CycleStartedWorldSeconds << C.DeathWorldSeconds << C.StressWorldSeconds << C.RegrowthCount << C.State << C.bRootsViable;
	}

	return !Ar.IsError();
}

bool KSISurfaceVegetation::Initialize(FKSISurfaceGrid& Grid, const TArray<FKSIVegetationSpecies>& Species,
	const TArray<FKSIVegetationSeed>& Seeds, double WorldSeconds, FString& Error)
{
	if (!Grid.Vegetation.Cells.IsEmpty() || !Grid.Fire.Cells.IsEmpty() || Seeds.Num() != Grid.Cells.Num()
		|| !Range(WorldSeconds, 0., 1.e12))
	{
		return Fail(Error, TEXT("Vegetation initialization requires an unbound grid and complete seed cells."));
	}

	auto G = Grid;
	G.Vegetation.Species = Species;
	G.Vegetation.Cells.SetNum(Seeds.Num());
	G.Fire.Cells.SetNum(Seeds.Num());
	for (int32 I = 0; I < Seeds.Num(); ++I)
	{
		const auto& Seed = Seeds[I];
		auto& C = G.Vegetation.Cells[I];
		C.Id = Seed.Id;
		C.SpeciesIndex = Seed.SpeciesIndex;
		C.InitialBiomassKg = C.LivingBiomassKg = Seed.BiomassKg;
		C.SoilNutrientsKg = C.InitialNutrientsKg = Seed.SoilNutrientsKg;
		C.SoilOrganicKg = C.InitialOrganicKg = Seed.SoilOrganicKg;
		C.CycleStartedWorldSeconds = WorldSeconds;
		if (Species.IsValidIndex(C.SpeciesIndex))
		{
			const auto& S = Species[C.SpeciesIndex];
			C.InitialNutrientsKg += Seed.BiomassKg * S.NutrientKgPerBiomassKg;
			C.bRootsViable = S.bRegrows;
			C.State = Seed.BiomassKg >= S.MaximumBiomassKg ? EKSIVegetationState::Mature : EKSIVegetationState::Growing;
		}

		if (!FKSICombustionModel::Initialize(G.Fire.Material, Seed.BiomassKg, G.Forcing.TemperatureCelsius, G.Fire.Cells[I], Error))
		{
			return false;
		}
	}

	if (!FKSISurfaceSimulation::ValidateRegion(G, Error))
	{
		return false;
	}

	Grid = MoveTemp(G);
	return true;
}

bool KSISurfaceVegetation::AdvanceCandidate(FKSISurfaceGrid& G, double WorldFrom, double WorldDelta,
	double Irradiance, double DaylightHours, FString& Error, FKSISurfaceThermalContext* Context)
{
	if (G.Vegetation.Cells.IsEmpty())
	{
		return true;
	}

	if (!Range(WorldFrom, 0., 1.e12) || !Range(WorldDelta, 0., 1.e12 - WorldFrom)
		|| !Range(Irradiance, 0., 1.e9) || !Range(DaylightHours, 0., 24.))
	{
		return Fail(Error, TEXT("Invalid vegetation world-time or solar input."));
	}

	const double WorldTo = WorldFrom + WorldDelta;
	for (int32 I = 0; I < G.Vegetation.Cells.Num(); ++I)
	{
		auto& C = G.Vegetation.Cells[I];
		if (C.SpeciesIndex == INDEX_NONE)
		{
			continue;
		}

		const auto& S = G.Vegetation.Species[C.SpeciesIndex];
		auto& Fire = G.Fire.Cells[I];
		auto& Surface = G.Cells[I];
		const double Burned = FMath::Max(0., Fire.BurnedKg - C.BurnedBiomassKg);
		const double BurnedDead = FMath::Min(C.DeadBiomassKg, Burned);
		const double BurnedLiving = FMath::Min(C.LivingBiomassKg, Burned - BurnedDead);
		C.DeadBiomassKg -= BurnedDead;
		C.LivingBiomassKg -= BurnedLiving;
		C.BurnedBiomassKg += BurnedDead + BurnedLiving;
		C.SoilNutrientsKg += (BurnedDead + BurnedLiving) * S.NutrientKgPerBiomassKg;
		const double FireTemperature = Fire.TemperatureCelsius(G.Fire.Material);
		if (FireTemperature >= S.LethalTemperatureCelsius)
		{
			Die(C, S, WorldFrom, FireTemperature >= S.RootKillTemperatureCelsius);
		}

		const bool bAquatic = S.Kind == EKSIVegetationKind::Algae || S.Kind == EKSIVegetationKind::AquaticPlant;
		double& Water = bAquatic ? Surface.WaterCubicMeters : Surface.SoilCubicMeters;
		const double Capacity = G.Forcing.SoilCapacityMeters * FMath::Square(G.SpacingMeters);
		const double Wetness = FMath::Clamp(Water / FMath::Max(Capacity, 1.e-12), 0., 1.);
		const auto* Room = Context && Context->PreviousAir
			? Context->PreviousAir->FindSpaceAt(FKSISurfaceThermalContext::CellPosition(G, I)) : nullptr;
		const double Temperature = Room ? Room->TemperatureCelsius() : G.Forcing.TemperatureCelsius;
		const bool bSeason = DaylightHours >= S.MinimumDaylightHours
			&& Temperature >= S.MinimumTemperatureCelsius
			&& Temperature <= S.MaximumTemperatureCelsius;
		const bool bWater = Wetness >= S.MinimumWetness;
		double Current = WorldFrom;
		int32 Boundaries = 0;
		while (Current < WorldTo)
		{
			if (++Boundaries > 32)
			{
				return Fail(Error, TEXT("Vegetation lifecycle exceeded the candidate boundary budget."));
			}

			if (C.State == EKSIVegetationState::Dead)
			{
				const bool bCanRegrow = C.bRootsViable && bSeason && bWater && C.SoilNutrientsKg > 0.
					&& (S.Kind != EKSIVegetationKind::Fungus || C.SoilOrganicKg > 0.)
					&& FireTemperature < S.LethalTemperatureCelsius;
				const double ReadyAt = C.DeathWorldSeconds + S.RegrowthDelaySeconds;
				if (bCanRegrow && Current >= ReadyAt)
				{
					if (C.RegrowthCount == MAX_uint32)
					{
						return Fail(Error, TEXT("Vegetation regrowth counter exhausted."));
					}

					++C.RegrowthCount;
					C.CycleStartedWorldSeconds = Current;
					C.DeathWorldSeconds = -1.;
					C.StressWorldSeconds = 0.;
					C.State = EKSIVegetationState::Growing;
					continue;
				}

				const double Stop = bCanRegrow && ReadyAt > Current ? FMath::Min(WorldTo, ReadyAt) : WorldTo;
				if (!Decay(G, I, S, Stop - Current, Error))
				{
					return false;
				}

				Current = Stop;
				continue;
			}

			const double LifetimeEnd = C.CycleStartedWorldSeconds + S.LifespanSeconds;
			double Stop = FMath::Min(WorldTo, LifetimeEnd);
			if (!bWater)
			{
				Stop = FMath::Min(Stop, Current + FMath::Max(0., S.StressDeathSeconds - C.StressWorldSeconds));
			}

			if (Stop <= Current)
			{
				Die(C, S, Current, false);
				continue;
			}

			const double Dt = Stop - Current;
			if (!Decay(G, I, S, Dt, Error))
			{
				return false;
			}

			C.State = bSeason && bWater ? EKSIVegetationState::Growing : EKSIVegetationState::Dormant;
			if (bSeason && bWater)
			{
				const double Sun = FMath::Clamp(Irradiance * Surface.SunExposure / S.FullGrowthIrradiance, 0., 1.);
				const double Light = S.Kind == EKSIVegetationKind::Fungus ? 1. - Sun : Sun;
				double Growth = S.GrowthKgPerWorldSecond * Dt * Light * FMath::Min(1., Wetness / S.OptimalWetness);
				Growth = FMath::Min(Growth, FMath::Max(0., S.MaximumBiomassKg - C.LivingBiomassKg));
				Growth = FMath::Min3(Growth, Water / S.WaterCubicMetersPerKg, C.SoilNutrientsKg / S.NutrientKgPerBiomassKg);
				if (S.Kind == EKSIVegetationKind::Fungus)
				{
					Growth = FMath::Min(Growth, C.SoilOrganicKg);
				}

				if (Room)
				{
					// Spend only available sensible heat above freezing. Earlier surface and plant
					// transfers in this candidate have already reserved their part of the room.
					const double RoomCapacity = Room->VolumeCubicMeters * FKSIAirSimulation::HeatCapacityPerCubicMeter;
					const double Available = FMath::Max(0., Room->Contents.HeatJoules
						+ Context->Sources.FindRef(Room->SpaceId).HeatJoules - 273.15 * RoomCapacity);
					const double Share = FMath::Max(0., Temperature) * RoomCapacity * 0.5
						/ FMath::Max(1, Context->ContactCounts.FindRef(Room->SpaceId));
					Growth = FMath::Min(Growth, FMath::Min(Available, Share) / (S.WaterCubicMetersPerKg * 2500000000.));
				}

				if (!FKSICombustionModel::TransferFuel(G.Fire.Material, Growth, Fire, Error))
				{
					return false;
				}

				C.LivingBiomassKg += Growth;
				C.GrownBiomassKg += Growth;
				C.SoilNutrientsKg = FMath::Max(0., C.SoilNutrientsKg - Growth * S.NutrientKgPerBiomassKg);
				const double UsedWater = FMath::Min(Water, Growth * S.WaterCubicMetersPerKg);
				Water -= UsedWater;
				C.TranspiredCubicMeters += UsedWater;
				G.Ledger.EvaporatedCubicMeters += UsedWater;
				if (Room)
				{
					const double Heat = UsedWater * 2500000000.;
					G.AirHeatToSurfaceJoules += Heat;
					Context->AddSource(Room->SpaceId, -Heat, UsedWater * 1000., 0.);
				}
				if (S.Kind == EKSIVegetationKind::Fungus)
				{
					C.SoilOrganicKg -= Growth;
					C.FungalConsumptionKg += Growth;
				}

				if (C.LivingBiomassKg >= S.MaximumBiomassKg)
				{
					C.State = EKSIVegetationState::Mature;
				}
			}

			C.StressWorldSeconds = bWater ? 0. : C.StressWorldSeconds + Dt;
			Current = Stop;
			if (Current >= LifetimeEnd || C.StressWorldSeconds >= S.StressDeathSeconds)
			{
				Die(C, S, Current, false);
			}
		}
	}

	return G.Vegetation.Validate(G, Error);
}

void KSISurfaceVegetation::RemoveSupport(FKSISurfaceGrid& G, int32 I, double NewBed, double WorldSeconds)
{
	if (!G.Vegetation.Cells.IsValidIndex(I))
	{
		return;
	}

	auto& C = G.Vegetation.Cells[I];
	if (G.Vegetation.Species.IsValidIndex(C.SpeciesIndex))
	{
		const auto& S = G.Vegetation.Species[C.SpeciesIndex];
		if (FMath::Abs(NewBed - G.Cells[I].BedMeters) > S.MaximumSupportChangeMeters)
		{
			Die(C, S, WorldSeconds, true);
		}
	}
}
