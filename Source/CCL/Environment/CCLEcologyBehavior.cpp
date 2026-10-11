#include "CCLEcologySimulation.h"
#include "CCLEcologySpace.h"

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

bool FCCLEcologySimulation::AddGroup(const FCCLEcologyGroup& Group, FString& Error)
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

bool FCCLEcologySimulation::AddForage(FCCLEcologyForage Source, FString& Error)
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

bool FCCLEcologySimulation::AddBehavior(FCCLEcologyBehavior Behavior, FString& Error)
{
	const auto* Individual = Individuals.Find(Behavior.Id);
	if (!Individual || Individual->State != ECCLEcologyLifeState::Alive || !Individual->bPersistent
		|| Behaviors.Contains(Behavior.Id) || Behavior.ConsumedUnits != 0. || Behavior.StarvedSeconds != 0.
		|| Behavior.TargetFoodId.IsValid())
	{
		return Reject(Error, TEXT("Behavior requires a live persistent reservation and a new history."));
	}

	Behavior.LastStepSeconds = WorldSeconds;
	Behavior.Activity = ECCLEcologyActivity::Roaming;
	auto Candidate = *this;
	Candidate.Behaviors.Add(Behavior.Id, Behavior);
	if (!Candidate.Validate(Error))
	{
		return false;
	}

	*this = MoveTemp(Candidate);
	return true;
}

bool FCCLEcologySimulation::ValidateBehavior(FString& Error) const
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

	double TotalConsumed = 0.;
	double TotalEaten = 0.;
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
			|| uint8(B.Activity) > uint8(ECCLEcologyActivity::Blocked)
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

bool FCCLEcologySimulation::AdvanceBehavior(double At, FString& Error, const ICCLEcologySpace* Space, const FCCLSurfaceSimulation* Surface)
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

	TArray<FGuid> Ids, FoodIds;
	Behaviors.GetKeys(Ids);
	ForageSources.GetKeys(FoodIds);
	Ids.Sort();
	FoodIds.Sort();
	TArray<FCCLEcologyEvent> Deaths;
	for (FGuid Id : Ids)
	{
		auto& B = Behaviors[Id];
		auto& I = Individuals[Id];
		if (I.State != ECCLEcologyLifeState::Alive)
		{
			B.Activity = ECCLEcologyActivity::Inactive;
			continue;
		}

		const auto& P = Populations[I.PopulationId];
		const auto& H = Habitats[P.HabitatId];
		const auto Medium = Species[P.SpeciesId].Movement;
		const bool bPlanar = Medium == ECCLEcologyMovement::Ground || Medium == ECCLEcologyMovement::Insect;
		const double Delta = At - B.LastStepSeconds;
		B.LastStepSeconds = At;
		B.Energy = FMath::Max(0., B.Energy - B.MetabolismPerSecond * Delta);
		B.TargetFoodId.Invalidate();
		B.Activity = ECCLEcologyActivity::Roaming;
		FVector Target = (H.MinimumMeters + H.MaximumMeters) * .5;
		double Radius = FMath::Min(H.MaximumMeters.X - H.MinimumMeters.X, H.MaximumMeters.Y - H.MinimumMeters.Y) * .2;
		FGuid MotionId = I.PopulationId;
		if (const auto* G = Groups.Find(B.GroupId))
		{
			Target = G->AnchorMeters;
			Radius = G->RadiusMeters;
			MotionId = G->Id;
		}

		// Shared route plus stable per-individual offsets gives a bounded moving formation.
		const double Phase = At * .12 + double(GetTypeHash(MotionId) % 360) * PI / 180.;
		const double Offset = double(GetTypeHash(Id) % 360) * PI / 180.;
		Target += FVector(FMath::Cos(Phase), FMath::Sin(Phase), bPlanar ? 0. : FMath::Sin(Phase * .5) * .4) * Radius * .6;
		Target += FVector(FMath::Cos(Offset), FMath::Sin(Offset), 0.) * Radius * .25;
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

			FCCLEcologyRouteRequest Q;
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
		if (B.Energy < B.HungerThreshold)
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
				B.Activity = ECCLEcologyActivity::SeekingFood;
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
			B.Activity = ECCLEcologyActivity::Blocked;
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
				B.Activity = ECCLEcologyActivity::Feeding;
			}
		}

		B.StarvedSeconds = B.Energy <= 0. ? B.StarvedSeconds + Delta : 0.;
		if (B.Energy <= 0.)
		{
			B.Activity = ECCLEcologyActivity::Starving;
		}

		if (B.StarvedSeconds >= B.StarvationLimitSeconds)
		{
			FCCLEcologyEvent E;
			E.Id = StarvationId(WorldId, Id, At);
			E.Kind = ECCLEcologyEventKind::Death;
			E.Source = I.PopulationId;
			E.CohortId = I.CohortId;
			E.IndividualId = Id;
			E.BirthWorldSeconds = I.BirthWorldSeconds;
			Deaths.Add(E);
			B.Activity = ECCLEcologyActivity::Inactive;
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


bool FCCLEcologySimulation::HasSpatialBehavior() const
{
	for (const auto& Pair : Behaviors)
	{
		const auto* I = Individuals.Find(Pair.Key);
		if (Pair.Value.bUseWorldSpace && I && I->State == ECCLEcologyLifeState::Alive)
		{
			return true;
		}
	}
	return false;
}
