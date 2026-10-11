#include "CCLEcologySimulation.h"

#include "Misc/SecureHash.h"
#include "Serialization/MemoryWriter.h"

namespace
{
	FGuid LifeEventId(FGuid World, FGuid Population, FGuid Cohort, FGuid Individual, double Time, uint8 Kind)
	{
		TArray<uint8> Bytes;
		FMemoryWriter Writer(Bytes, true);
		Writer << World << Population << Cohort << Individual << Time << Kind;
		FMD5 Hash;
		Hash.Update(Bytes.GetData(), Bytes.Num());
		uint32 Digest[4];
		Hash.Final(reinterpret_cast<uint8*>(Digest));
		return FGuid(Digest[0], Digest[1], Digest[2], Digest[3]);
	}
}

bool FCCLEcologySimulation::AdvanceTo(double Target, FString& Error, int32 MaxBoundaries, const ICCLEcologySpace* Space, const FCCLSurfaceSimulation* Surface)
{
	if (!Validate(Error))
	{
		return false;
	}

	if (!FMath::IsFinite(Target) || Target < WorldSeconds || Target > 1.e12 || MaxBoundaries < 1 || MaxBoundaries > 4096)
	{
		Error = TEXT("Invalid ecology target time or boundary budget.");
		return false;
	}

	FCCLEcologySimulation Candidate = *this;
	if (!Candidate.AdvanceCandidate(Target, Error, MaxBoundaries, Space, Surface) || !Candidate.Validate(Error))
	{
		return false;
	}

	*this = MoveTemp(Candidate);
	return true;
}

bool FCCLEcologySimulation::AdvanceCandidate(double Target, FString& Error, int32 MaxBoundaries, const ICCLEcologySpace* Space, const FCCLSurfaceSimulation* Surface)
{
	TArray<FGuid> PopulationIds;
	Populations.GetKeys(PopulationIds);
	PopulationIds.Sort();
	int32 Boundaries = 0;
	while (WorldSeconds < Target)
	{
		double Next = Target;
		const double BehaviorBoundary = (!Behaviors.IsEmpty() || !ForageSources.IsEmpty())
			? FMath::FloorToDouble(WorldSeconds) + 1. : TNumericLimits<double>::Max();
		Next = FMath::Min(Next, BehaviorBoundary);
		TMap<FGuid, double> BirthTimes;
		for (FGuid Id : PopulationIds)
		{
			const auto& P = Populations[Id];
			const auto& S = Species[P.SpeciesId];
			if (LivingCount(Id) && S.BirthIntervalSeconds > 0.)
			{
				const double BirthAt = (FMath::FloorToDouble(WorldSeconds / S.BirthIntervalSeconds) + 1.) * S.BirthIntervalSeconds;
				BirthTimes.Add(Id, BirthAt);
				Next = FMath::Min(Next, BirthAt);
			}

			for (const auto& Pair : P.Cohorts)
			{
				if (Pair.Value.Count > 0)
				{
					Next = FMath::Min(Next, FMath::Max(WorldSeconds, Pair.Value.BirthWorldSeconds + S.LifespanSeconds));
				}
			}
		}

		for (const auto& Pair : Individuals)
		{
			const auto& I = Pair.Value;
			if (I.State == ECCLEcologyLifeState::Alive)
			{
				const auto& S = Species[Populations[I.PopulationId].SpeciesId];
				Next = FMath::Min(Next, FMath::Max(WorldSeconds, I.BirthWorldSeconds + S.LifespanSeconds));
			}
		}

		if (++Boundaries > MaxBoundaries)
		{
			Error = TEXT("Ecology chronological boundary budget exceeded; retry a shorter world interval.");
			return false;
		}

		WorldSeconds = Next;
		TArray<FCCLEcologyEvent> Deaths;
		for (FGuid Id : PopulationIds)
		{
			const auto& P = Populations[Id];
			const auto& S = Species[P.SpeciesId];
			TArray<FGuid> CohortIds;
			P.Cohorts.GetKeys(CohortIds);
			CohortIds.Sort();
			for (FGuid CohortId : CohortIds)
			{
				const auto& C = P.Cohorts[CohortId];
				if (C.Count > 0 && C.BirthWorldSeconds + S.LifespanSeconds <= Next)
				{
					FCCLEcologyEvent E;
					E.Kind = ECCLEcologyEventKind::Death;
					E.Id = LifeEventId(WorldId, Id, C.Id, {}, C.BirthWorldSeconds + S.LifespanSeconds, uint8(E.Kind));
					E.Source = Id;
					E.CohortId = C.Id;
					E.BirthWorldSeconds = C.BirthWorldSeconds;
					E.Count = C.Count;
					Deaths.Add(E);
				}
			}
		}

		TArray<FGuid> IndividualIds;
		Individuals.GetKeys(IndividualIds);
		IndividualIds.Sort();
		for (FGuid Id : IndividualIds)
		{
			const auto& I = Individuals[Id];
			const auto& S = Species[Populations[I.PopulationId].SpeciesId];
			if (I.State == ECCLEcologyLifeState::Alive && I.BirthWorldSeconds + S.LifespanSeconds <= Next)
			{
				FCCLEcologyEvent E;
				E.Kind = ECCLEcologyEventKind::Death;
				E.Id = LifeEventId(WorldId, I.PopulationId, I.CohortId, Id, I.BirthWorldSeconds + S.LifespanSeconds, uint8(E.Kind));
				E.Source = I.PopulationId;
				E.CohortId = I.CohortId;
				E.BirthWorldSeconds = I.BirthWorldSeconds;
				E.IndividualId = Id;
				Deaths.Add(E);
			}
		}

		for (const auto& E : Deaths)
		{
			// The authoritative lifespan deadline revokes execution before recording death.
			Leases.Remove(E.IndividualId);
			if (!Apply(E, Error))
			{
				return false;
			}
		}

		if (Next == BehaviorBoundary && !AdvanceBehavior(Next, Error, Space, Surface))
		{
			return false;
		}

		// Shared habitat capacity is awarded in stable population-ID order after all deaths.
		for (FGuid Id : PopulationIds)
		{
			const double* BirthAt = BirthTimes.Find(Id);
			if (!BirthAt || *BirthAt != Next)
			{
				continue;
			}

			const auto& P = Populations[Id];
			const auto& S = Species[P.SpeciesId];
			int64 Adults = 0;
			for (const auto& Pair : P.Cohorts)
			{
				Adults += Next - Pair.Value.BirthWorldSeconds >= S.MaturitySeconds ? Pair.Value.Count : 0;
			}

			for (const auto& Pair : Individuals)
			{
				const auto& I = Pair.Value;
				Adults += I.PopulationId == Id && I.State == ECCLEcologyLifeState::Alive
					&& Next - I.BirthWorldSeconds >= S.MaturitySeconds ? 1 : 0;
			}

			int64 Available = Habitats[P.HabitatId].Capacity;
			for (FGuid OtherId : PopulationIds)
			{
				Available -= Populations[OtherId].HabitatId == P.HabitatId ? LivingCount(OtherId) : 0;
			}

			const int64 Count = FMath::Min(Available, Adults / 2);
			if (Count > 0)
			{
				FCCLEcologyEvent E;
				E.Kind = ECCLEcologyEventKind::Birth;
				E.CohortId = LifeEventId(WorldId, Id, {}, {}, Next, 255);
				E.Id = LifeEventId(WorldId, Id, E.CohortId, {}, Next, uint8(E.Kind));
				E.Source = Id;
				E.Target = Id;
				E.BirthWorldSeconds = Next;
				E.Count = Count;
				if (!Apply(E, Error))
				{
					return false;
				}
			}
		}
	}

	return true;
}
