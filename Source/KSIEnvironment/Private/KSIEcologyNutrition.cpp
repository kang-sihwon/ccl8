#include "KSIEcologySimulation.h"

#include "Misc/SecureHash.h"
#include "Serialization/MemoryWriter.h"

void FKSIEcologyNutrition::Charge(double At, double Metabolism)
{
	const double Delta = FMath::Max(0., At - LastStepSeconds);
	const double EmptyTime = Metabolism > 0. ? FMath::Max(0., Delta - Energy / Metabolism) : (Energy <= 0. ? Delta : 0.);
	Energy = FMath::Max(0., Energy - Metabolism * Delta);
	StarvedSeconds = Energy <= 0. ? StarvedSeconds + EmptyTime : 0.;
	LastStepSeconds = At;
}

void FKSIEcologyNutrition::Merge(int64 Count, FKSIEcologyNutrition Other, int64 OtherCount, double At, double Metabolism)
{
	Charge(At, Metabolism);
	Other.Charge(At, Metabolism);
	Energy = (Energy * double(Count) + Other.Energy * double(OtherCount)) / double(Count + OtherCount);
	StarvedSeconds = Count > 0 ? FMath::Max(StarvedSeconds, Other.StarvedSeconds) : Other.StarvedSeconds;
}

namespace
{
	FGuid AggregateDeathId(FGuid World, FGuid Population, FGuid Cohort, FGuid Individual, double At)
	{
		TArray<uint8> Bytes;
		FMemoryWriter Writer(Bytes, true);
		uint32 Domain = 0x4147474E;
		Writer << Domain << World << Population << Cohort << Individual << At;
		FMD5 Hash;
		Hash.Update(Bytes.GetData(), Bytes.Num());
		uint32 Digest[4];
		Hash.Final(reinterpret_cast<uint8*>(Digest));
		return FGuid(Digest[0], Digest[1], Digest[2], Digest[3]);
	}
}

bool FKSIEcologySimulation::AdvanceAggregate(double At, FString& Error)
{
	TArray<FGuid> PopulationIds, FoodIds, IndividualIds;
	Populations.GetKeys(PopulationIds);
	ForageSources.GetKeys(FoodIds);
	Individuals.GetKeys(IndividualIds);
	PopulationIds.Sort();
	FoodIds.Sort();
	IndividualIds.Sort();
	TArray<FKSIEcologyEvent> Deaths;
	for (FGuid PopulationId : PopulationIds)
	{
		auto& P = Populations[PopulationId];
		const auto& Policy = Species[P.SpeciesId];
		if (!Policy.AggregateDietMask)
		{
			continue;
		}

		auto Feed = [&](FKSIEcologyNutrition& N, int64 Count, FGuid CohortId, FGuid IndividualId, double Birth)
		{
			if (Count <= 0)
			{
				return;
			}

			N.Charge(At, Policy.AggregateMetabolism);
			for (FGuid FoodId : FoodIds)
			{
				auto& F = ForageSources[FoodId];
				if (F.HabitatId != P.HabitatId || !(F.FoodMask & Policy.AggregateDietMask))
				{
					continue;
				}

				const double Needed = FMath::Max(0., 1. - N.Energy) * double(Count) / Policy.AggregateEnergyPerUnit;
				const double Amount = FMath::Min(F.AvailableUnits, Needed);
				F.AvailableUnits -= Amount;
				F.ConsumedUnits += Amount;
				P.AggregateConsumedUnits += Amount;
				N.Energy = FMath::Min(1., N.Energy + Amount * Policy.AggregateEnergyPerUnit / double(Count));
			}

			if (N.Energy > 0.)
			{
				N.StarvedSeconds = 0.;
			}
			if (N.StarvedSeconds >= Policy.AggregateStarvationSeconds)
			{
				FKSIEcologyEvent E;
				E.Id = AggregateDeathId(WorldId, PopulationId, CohortId, IndividualId, At);
				E.Kind = EKSIEcologyEventKind::Death;
				E.Source = PopulationId;
				E.CohortId = CohortId;
				E.IndividualId = IndividualId;
				E.Count = Count;
				E.BirthWorldSeconds = Birth;
				Deaths.Add(E);
			}
		};

		TArray<FGuid> CohortIds;
		P.Cohorts.GetKeys(CohortIds);
		CohortIds.Sort();
		for (FGuid Id : CohortIds)
		{
			auto& C = P.Cohorts[Id];
			Feed(C.Nutrition, C.Count, C.Id, {}, C.BirthWorldSeconds);
		}
		for (FGuid Id : IndividualIds)
		{
			auto& I = Individuals[Id];
			if (I.PopulationId == PopulationId && I.State == EKSIEcologyLifeState::Alive && !Behaviors.Contains(Id))
			{
				Feed(I.Nutrition, 1, I.CohortId, Id, I.BirthWorldSeconds);
			}
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
