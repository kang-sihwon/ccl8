#include "KSIEcologySimulation.h"
#include "KSIEcologySpace.h"

#include "Misc/SecureHash.h"
#include "Serialization/MemoryWriter.h"

namespace
{
	bool Bounded(double Value, double Maximum = 1.e12)
	{
		return FMath::IsFinite(Value) && Value >= 0. && Value <= Maximum;
	}

	bool Reject(FString& Error, const TCHAR* Message)
	{
		Error = Message;
		return false;
	}

	FGuid StarvationId(FGuid WorldId, FGuid IndividualId, double At)
	{
		TArray<uint8> Bytes;
		FMemoryWriter Writer(Bytes, true);
		uint32 Domain = 0x53544152;
		Writer << Domain << WorldId << IndividualId << At;
		FMD5 Hash;
		Hash.Update(Bytes.GetData(), Bytes.Num());
		uint32 Digest[4];
		Hash.Final(reinterpret_cast<uint8*>(Digest));
		return FGuid(Digest[0], Digest[1], Digest[2], Digest[3]);
	}
}

bool FKSIEcologySimulation::AddGroup(const FKSIEcologyGroup& Group, FString& Error)
{
	if (Groups.Contains(Group.Id))
	{
		return Reject(Error, TEXT("Ecology group identity is already registered."));
	}

	auto Candidate = *this;
	Candidate.Groups.Add(Group.Id, Group);
	if (!Candidate.Validate(Error))
	{
		return false;
	}

	*this = MoveTemp(Candidate);
	return true;
}

bool FKSIEcologySimulation::AddForage(FKSIEcologyForage Source, FString& Error)
{
	if (ForageSources.Contains(Source.Id) || Source.InitialUnits != Source.AvailableUnits
		|| Source.GrownUnits != 0. || Source.ConsumedUnits != 0.)
	{
		return Reject(Error, TEXT("New forage requires a unique identity and an initial balanced ledger."));
	}

	Source.LastStepSeconds = WorldSeconds;
	auto Candidate = *this;
	Candidate.ForageSources.Add(Source.Id, Source);
	if (!Candidate.Validate(Error))
	{
		return false;
	}

	*this = MoveTemp(Candidate);
	return true;
}

bool FKSIEcologySimulation::AddBehavior(FKSIEcologyBehavior Behavior, FString& Error)
{
	const auto* Individual = Individuals.Find(Behavior.Id);
	if (!Individual || Individual->State != EKSIEcologyLifeState::Alive || !Individual->bPersistent
		|| Behaviors.Contains(Behavior.Id) || Behavior.ConsumedUnits != 0. || Behavior.StarvedSeconds != 0.
		|| Behavior.TargetFoodId.IsValid())
	{
		return Reject(Error, TEXT("Behavior requires a live persistent reservation and a new history."));
	}

	Behavior.LastStepSeconds = WorldSeconds;
	const auto& Policy = Species[Populations[Individual->PopulationId].SpeciesId];
	if (Policy.AggregateDietMask)
	{
		auto Nutrition = Individual->Nutrition;
		Nutrition.Charge(WorldSeconds, Policy.AggregateMetabolism);
		Behavior.Energy = Nutrition.Energy;
		Behavior.StarvedSeconds = Nutrition.StarvedSeconds;
	}
	Behavior.Activity = EKSIEcologyActivity::Roaming;
	auto Candidate = *this;
	Candidate.Behaviors.Add(Behavior.Id, Behavior);
	if (!Candidate.Validate(Error))
	{
		return false;
	}

	*this = MoveTemp(Candidate);
	return true;
}

bool FKSIEcologySimulation::ValidateBehavior(FString& Error) const
{
	if (Groups.Num() > 256 || ForageSources.Num() > 4096 || Behaviors.Num() > 4096)
	{
		return Reject(Error, TEXT("Ecology behavior state budget exceeded."));
	}

	for (const auto& Pair : Groups)
	{
		const auto& G = Pair.Value;
		const auto* H = Habitats.Find(G.HabitatId);
		if (!G.Id.IsValid() || G.Id != Pair.Key || !H || G.AnchorMeters.ContainsNaN()
			|| !FBox(H->MinimumMeters, H->MaximumMeters).IsInsideOrOn(G.AnchorMeters)
			|| !Bounded(G.RadiusMeters, 10000.) || G.RadiusMeters <= 0.)
		{
			return Reject(Error, TEXT("Invalid ecology group geometry or habitat."));
		}
	}

	auto NutritionValid = [&](const FKSIEcologyNutrition& N)
	{
		return Bounded(N.Energy, 1.) && Bounded(N.StarvedSeconds) && Bounded(N.LastStepSeconds, WorldSeconds);
	};
	for (const auto& Pair : Species)
	{
		const auto& S = Pair.Value;
		if (!Bounded(S.AggregateMetabolism, 1.) || !Bounded(S.AggregateEnergyPerUnit, 1.e6) || S.AggregateEnergyPerUnit <= 0. ||
			!Bounded(S.AggregateStarvationSeconds, 1.e9) || S.AggregateStarvationSeconds <= 0.)
		{
			return Reject(Error, TEXT("Invalid aggregate nutrition policy."));
		}
	}

	double AggregateEaten = 0.;
	for (const auto& Pair : Populations)
	{
		if (!Bounded(Pair.Value.AggregateConsumedUnits))
		{
			return Reject(Error, TEXT("Invalid aggregate food history."));
		}
		AggregateEaten += Pair.Value.AggregateConsumedUnits;
		for (const auto& Cohort : Pair.Value.Cohorts)
		{
			if (!NutritionValid(Cohort.Value.Nutrition))
			{
				return Reject(Error, TEXT("Invalid cohort energy or time."));
			}
		}
	}
	for (const auto& Pair : Individuals)
	{
		if (!NutritionValid(Pair.Value.Nutrition))
		{
			return Reject(Error, TEXT("Invalid reserved energy or time."));
		}
	}

	double TotalConsumed = 0.;
	double TotalEaten = AggregateEaten;
	for (const auto& Pair : ForageSources)
	{
		const auto& F = Pair.Value;
		TotalConsumed += F.ConsumedUnits;
		const auto* H = Habitats.Find(F.HabitatId);
		if (!F.Id.IsValid() || F.Id != Pair.Key || !H || F.PositionMeters.ContainsNaN()
			|| !FBox(H->MinimumMeters, H->MaximumMeters).IsInsideOrOn(F.PositionMeters) || !F.FoodMask
			|| !Bounded(F.InitialUnits) || !Bounded(F.AvailableUnits) || !Bounded(F.CapacityUnits)
			|| F.AvailableUnits > F.CapacityUnits || !Bounded(F.RegrowthPerSecond, 1.e6)
			|| !Bounded(F.GrownUnits) || !Bounded(F.ConsumedUnits)
			|| !Bounded(F.LastStepSeconds, WorldSeconds)
			|| !FMath::IsNearlyEqual(F.InitialUnits + F.GrownUnits, F.AvailableUnits + F.ConsumedUnits,
				1.e-8 * FMath::Max(1., F.InitialUnits + F.GrownUnits)))
		{
			return Reject(Error, TEXT("Invalid ecology forage geometry, time or resource ledger."));
		}
	}

	for (const auto& Pair : Behaviors)
	{
		const auto& B = Pair.Value;
		TotalEaten += B.ConsumedUnits;
		const auto* I = Individuals.Find(B.Id);
		const auto* P = I ? Populations.Find(I->PopulationId) : nullptr;
		const auto* G = Groups.Find(B.GroupId);
		const auto* F = ForageSources.Find(B.TargetFoodId);
		if (B.Id != Pair.Key || !I || !I->bPersistent || !P
			|| (B.GroupId.IsValid() && (!G || G->HabitatId != P->HabitatId))
			|| (B.TargetFoodId.IsValid() && (!F || F->HabitatId != P->HabitatId || !(F->FoodMask & B.DietMask)))
			|| !Bounded(B.SpeedMetersPerSecond, 100.) || B.SpeedMetersPerSecond <= 0. || !B.DietMask
			|| !Bounded(B.Energy, 1.) || !Bounded(B.MetabolismPerSecond, 1.)
			|| !Bounded(B.BiteUnitsPerSecond, 1.e6) || B.BiteUnitsPerSecond <= 0.
			|| !Bounded(B.EnergyPerUnit, 1.e6) || B.EnergyPerUnit <= 0.
			|| !Bounded(B.HungerThreshold, 1.) || !Bounded(B.StarvationLimitSeconds, 1.e9) || B.StarvationLimitSeconds <= 0.
			|| !Bounded(B.StarvedSeconds) || !Bounded(B.ConsumedUnits) || !Bounded(B.LastStepSeconds, WorldSeconds)
			|| uint8(B.Activity) > uint8(EKSIEcologyActivity::Blocked)
			|| B.bUseWorldSpace > 1 || !Bounded(B.RadiusMeters, 10.) || B.RadiusMeters < .02)
		{
			return Reject(Error, TEXT("Invalid ecology behavior ownership, policy or history."));
		}
	}

	if (!FMath::IsNearlyEqual(TotalConsumed, TotalEaten, 1.e-8 * FMath::Max(1., TotalConsumed)))
	{
		return Reject(Error, TEXT("Forage consumption and individual feeding histories disagree."));
	}

	return true;
}

bool FKSIEcologySimulation::AdvanceBehavior(double At, FString& Error, const IKSIEcologySpace* Space, const FKSISurfaceSimulation* Surface)
{
	// Called only on integer world boundaries, inside the unpublished ecology candidate.
	for (auto& Pair : ForageSources)
	{
		auto& F = Pair.Value;
		const double Growth = FMath::Min(F.CapacityUnits - F.AvailableUnits, F.RegrowthPerSecond * (At - F.LastStepSeconds));
		F.AvailableUnits += Growth;
		F.GrownUnits += Growth;
		F.LastStepSeconds = At;
	}

	if (!AdvanceAggregate(At, Error))
	{
		return false;
	}

	TArray<FGuid> Ids, FoodIds;
	Behaviors.GetKeys(Ids);
	ForageSources.GetKeys(FoodIds);
	Ids.Sort();
	FoodIds.Sort();
	// Read neighbors from the same pre-step snapshot, independent of update order.
	double NeighborhoodSize = 1.;
	for (FGuid Id : Ids)
	{
		NeighborhoodSize = FMath::Max(NeighborhoodSize, Behaviors[Id].RadiusMeters * 4.);
	}
	TMap<FIntVector, TArray<FGuid>> Neighborhoods;
	TMap<FGuid, FVector> PreviousPositions;
	auto CellAt = [NeighborhoodSize](const FVector& Position)
	{
		return FIntVector(FMath::FloorToInt(Position.X / NeighborhoodSize), FMath::FloorToInt(Position.Y / NeighborhoodSize), FMath::FloorToInt(Position.Z / NeighborhoodSize));
	};
	for (FGuid Id : Ids)
	{
		if (Individuals[Id].State != EKSIEcologyLifeState::Alive)
		{
			continue;
		}
		PreviousPositions.Add(Id, Individuals[Id].PositionMeters);
		Neighborhoods.FindOrAdd(CellAt(Individuals[Id].PositionMeters)).Add(Id);
	}
	TArray<FKSIEcologyEvent> Deaths;
	for (FGuid Id : Ids)
	{
		auto& B = Behaviors[Id];
		auto& I = Individuals[Id];
		if (I.State != EKSIEcologyLifeState::Alive)
		{
			B.Activity = EKSIEcologyActivity::Inactive;
			continue;
		}

		const auto& P = Populations[I.PopulationId];
		const auto& H = Habitats[P.HabitatId];
		const auto Medium = Species[P.SpeciesId].Movement;
		const bool bPlanar = Medium == EKSIEcologyMovement::Ground || Medium == EKSIEcologyMovement::Insect;
		const double Delta = At - B.LastStepSeconds;
		FKSIEcologyNutrition Charged{B.Energy, B.StarvedSeconds, B.LastStepSeconds};
		Charged.Charge(At, B.MetabolismPerSecond);
		B.LastStepSeconds = At;
		B.Energy = Charged.Energy;
		const bool bContinueFeeding = B.Activity == EKSIEcologyActivity::Feeding && B.Energy < .95;
		B.TargetFoodId.Invalidate();
		B.Activity = EKSIEcologyActivity::Roaming;
		FVector Target = (H.MinimumMeters + H.MaximumMeters) * .5;
		double Radius = FMath::Min(H.MaximumMeters.X - H.MinimumMeters.X, H.MaximumMeters.Y - H.MinimumMeters.Y) * .2;
		if (const auto* G = Groups.Find(B.GroupId))
		{
			Target = G->AnchorMeters;
			Radius = G->RadiusMeters;
		}

		// Independent, continuous search paths are reproducible after save/restore.
		// The group anchor bounds the home range, rather than forcing a shared orbit.
		const FVector Home = Target;
		const double Offset = double(GetTypeHash(Id) % 360) * PI / 180.;
		const double Pace = .10 + double(GetTypeHash(Id) % 13) * .004;
		const double Phase = At * Pace + Offset;
		Target += FVector(FMath::Sin(Phase) * .65 + FMath::Sin(Phase * .43 + Offset) * .2,
			FMath::Sin(Phase * .71 + Offset) * .7, bPlanar ? 0. : FMath::Sin(Phase * .53 + Offset) * .25) * Radius;
		// Ground animals pause to inspect/forage. Hunger interrupts the pause below.
		const double RestPhase = FMath::Fmod(At + double(GetTypeHash(Id) % 23), 23.);
		const bool bResting = bPlanar && RestPhase < 5.;
		if (bResting)
		{
			Target = I.PositionMeters;
		}
		FVector Separation = FVector::ZeroVector;
		const FIntVector Cell = CellAt(I.PositionMeters);
		for (int32 X = -1; X <= 1; ++X)
		{
			for (int32 Y = -1; Y <= 1; ++Y)
			{
				for (int32 Z = -1; Z <= 1; ++Z)
				{
					const auto* Neighbors = Neighborhoods.Find(Cell + FIntVector(X, Y, Z));
					if (!Neighbors)
					{
						continue;
					}
					for (FGuid Other : *Neighbors)
					{
						if (Other == Id || Individuals[Other].PopulationId != I.PopulationId)
						{
							continue;
						}
						FVector Away = I.PositionMeters - PreviousPositions[Other];
						if (bPlanar)
						{
							Away.Z = 0.;
						}
						const double Distance = Away.Size();
						const double SpaceWanted = (B.RadiusMeters + Behaviors[Other].RadiusMeters) * 2.;
						if (Distance < SpaceWanted)
						{
							const FVector Direction = Distance > .001 ? Away / Distance : FVector(FMath::Cos(Offset), FMath::Sin(Offset), 0.);
							Separation += Direction * (SpaceWanted - Distance);
						}
					}
				}
			}
		}
		Target += Separation.GetClampedToMaxSize(Radius * .5);
		Target = Home + (Target - Home).GetClampedToMaxSize(Radius);
		Target = Target.ComponentMax(H.MinimumMeters).ComponentMin(H.MaximumMeters);
		if (bPlanar && !B.bUseWorldSpace)
		{
			Target.Z = I.PositionMeters.Z;
		}

		TArray<FVector> Route;
		auto FindRoute = [&](const FVector& Goal, TArray<FVector>& Points)
		{
			if (!B.bUseWorldSpace)
			{
				FVector End = Goal;
				if (bPlanar)
				{
					End.Z = I.PositionMeters.Z;
				}

				Points = {I.PositionMeters, End};
				return true;
			}

			if (!Space)
			{
				Error = TEXT("Spatial ecology behavior requires a route provider.");
				return false;
			}

			FKSIEcologyRouteRequest Q;
			Q.Medium = Medium;
			Q.FromMeters = I.PositionMeters;
			Q.TargetMeters = Goal;
			Q.HabitatMeters = FBox(H.MinimumMeters, H.MaximumMeters);
			Q.RadiusMeters = B.RadiusMeters;
			Q.WorldSeconds = At;
			if (!Space->FindRoute(Q, Surface, Points, Error))
			{
				return false;
			}

			if (Points.Num() > 1024 || (!Points.IsEmpty() && (!Points[0].Equals(I.PositionMeters, 1.e-5)
				|| FVector::Distance(Points.Last(), Goal) > .3)))
			{
				Error = TEXT("Ecology route has invalid endpoints or exceeds its point budget.");
				return false;
			}
			for (const FVector& Pnt : Points)
			{
				if (Pnt.ContainsNaN() || !Q.HabitatMeters.IsInsideOrOn(Pnt))
				{
					Error = TEXT("Ecology route left the habitat.");
					return false;
				}
			}
			return true;
		};
		// Keep eating until satiated instead of toggling search/feed around one threshold.
		if (B.Energy < B.HungerThreshold || bContinueFeeding)
		{
			double Nearest = TNumericLimits<double>::Max();
			for (FGuid FoodId : FoodIds)
			{
				const auto& F = ForageSources[FoodId];
				if (F.HabitatId != P.HabitatId || !(F.FoodMask & B.DietMask) || F.AvailableUnits <= 0.
					|| (bPlanar && !B.bUseWorldSpace && FMath::Abs(F.PositionMeters.Z - I.PositionMeters.Z) > .1))
				{
					continue;
				}

				const double Distance = FVector::DistSquared(F.PositionMeters, I.PositionMeters);
				if (Distance < Nearest)
				{
					TArray<FVector> FoodRoute;
					if (!FindRoute(F.PositionMeters, FoodRoute))
					{
						return false;
					}

					if (FoodRoute.IsEmpty())
					{
						continue;
					}

					Route = MoveTemp(FoodRoute);
					Nearest = Distance;
					B.TargetFoodId = FoodId;
					Target = F.PositionMeters;
					if (bPlanar && !B.bUseWorldSpace)
					{
						Target.Z = I.PositionMeters.Z;
					}
				}
			}

			if (B.TargetFoodId.IsValid())
			{
				B.Activity = EKSIEcologyActivity::SeekingFood;
			}
		}

		if (!B.TargetFoodId.IsValid() && !FindRoute(Target, Route))
		{
			return false;
		}

		double Remaining = B.SpeedMetersPerSecond * Delta;
		double Travelled = 0.;
		bool bReached = !Route.IsEmpty();
		for (int32 PointIndex = 1; PointIndex < Route.Num(); ++PointIndex)
		{
			const FVector Difference = Route[PointIndex] - I.PositionMeters;
			const double Distance = Difference.Size();
			const double Take = FMath::Min(Distance, Remaining);
			I.PositionMeters += Difference.GetSafeNormal() * Take;
			Remaining -= Take;
			Travelled += Take;
			if (Take < Distance)
			{
				bReached = false;
				break;
			}
		}

		if (Route.IsEmpty())
		{
			B.Activity = EKSIEcologyActivity::Blocked;
		}

		const double TravelSeconds = Travelled / B.SpeedMetersPerSecond;
		if (B.TargetFoodId.IsValid() && bReached && TravelSeconds < Delta)
		{
			auto& F = ForageSources[B.TargetFoodId];
			const double Amount = FMath::Min3(F.AvailableUnits, B.BiteUnitsPerSecond * (Delta - TravelSeconds), (1. - B.Energy) / B.EnergyPerUnit);
			F.AvailableUnits -= Amount;
			F.ConsumedUnits += Amount;
			B.ConsumedUnits += Amount;
			B.Energy = FMath::Min(1., B.Energy + Amount * B.EnergyPerUnit);
			if (Amount > 0.)
			{
				B.Activity = EKSIEcologyActivity::Feeding;
			}
		}

		// Nutrition-enabled populations keep the same empty-energy duration across representations.
		// Legacy species retain their original whole-step starvation rule.
		const double Starved = Species[P.SpeciesId].AggregateDietMask ? Charged.StarvedSeconds : B.StarvedSeconds + Delta;
		B.StarvedSeconds = B.Energy <= 0. ? Starved : 0.;
		if (B.Energy <= 0.)
		{
			B.Activity = EKSIEcologyActivity::Starving;
		}

		if (B.StarvedSeconds >= B.StarvationLimitSeconds)
		{
			FKSIEcologyEvent E;
			E.Id = StarvationId(WorldId, Id, At);
			E.Kind = EKSIEcologyEventKind::Death;
			E.Source = I.PopulationId;
			E.CohortId = I.CohortId;
			E.IndividualId = Id;
			E.BirthWorldSeconds = I.BirthWorldSeconds;
			Deaths.Add(E);
			B.Activity = EKSIEcologyActivity::Inactive;
		}
	}

	for (const auto& E : Deaths)
	{
		Leases.Remove(E.IndividualId);
		if (!Apply(E, Error))
		{
			return false;
		}
	}

	return true;
}

bool FKSIEcologySimulation::HasSpatialBehavior() const
{
	for (const auto& Pair : Behaviors)
	{
		const auto* I = Individuals.Find(Pair.Key);
		if (Pair.Value.bUseWorldSpace && I && I->State == EKSIEcologyLifeState::Alive)
		{
			return true;
		}
	}
	return false;
}
