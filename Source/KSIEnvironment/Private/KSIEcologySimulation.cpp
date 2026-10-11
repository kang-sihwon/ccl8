#include "KSIEcologySimulation.h"

#include "Misc/Crc.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace
{
	constexpr int64 MaximumCount = 1000000000;
	constexpr int64 MaximumLedger = 1000000000000;
	bool Fail(FString& Error, const TCHAR* Message)
	{
		Error = Message;
		return false;
	}

	bool Time(double Value)
	{
		return FMath::IsFinite(Value) && FMath::Abs(Value) <= 1.e12;
	}

	bool Quantity(int64 Value, int64 Limit = MaximumCount)
	{
		return Value >= 0 && Value <= Limit;
	}

	bool EventShape(const FKSIEcologyEvent& E, double WorldSeconds)
	{
		return E.Id.IsValid() && uint8(E.Kind) <= uint8(EKSIEcologyEventKind::Materialize)
			&& E.CohortId.IsValid() && E.Count > 0 && Quantity(E.Count) && E.bPersistent <= 1
			&& Time(E.BirthWorldSeconds) && E.BirthWorldSeconds <= WorldSeconds;
	}

	template<typename T> void Enum(FArchive& Ar, T& Value)
	{
		uint8 Byte = uint8(Value);
		Ar << Byte;
		Value = T(Byte);
	}

	void Vector(FArchive& Ar, FVector& V)
	{
		Ar << V.X << V.Y << V.Z;
	}

	bool Count(FArchive& Ar, int32& N, int32 Maximum, int32 MinimumBytes)
	{
		Ar << N;
		if (Ar.IsError() || N < 0 || N > Maximum
			|| (Ar.IsLoading() && int64(N) * MinimumBytes > Ar.TotalSize() - Ar.Tell()))
		{
			Ar.SetError();
			return false;
		}

		return true;
	}

	template<typename T, typename F> bool Map(FArchive& Ar, TMap<FGuid, T>& Values, int32 Maximum, int32 MinimumBytes, F Serialize)
	{
		int32 N = Values.Num();
		if (!Count(Ar, N, Maximum, MinimumBytes))
		{
			return false;
		}

		if (Ar.IsLoading())
		{
			Values.Reset();
			for (int32 I = 0; I < N; ++I)
			{
				T Value;
				Serialize(Value);
				if (Ar.IsError() || !Value.Id.IsValid() || Values.Contains(Value.Id))
				{
					Ar.SetError();
					return false;
				}

				const FGuid Id = Value.Id;
				Values.Add(Id, MoveTemp(Value));
			}
		}
		else
		{
			TArray<FGuid> Keys;
			Values.GetKeys(Keys);
			Keys.Sort();
			for (FGuid Id : Keys)
			{
				Serialize(Values[Id]);
			}
		}

		return !Ar.IsError();
	}
}

bool FKSIEcologyEvent::operator==(const FKSIEcologyEvent& O) const
{
	return Id == O.Id && Kind == O.Kind && Source == O.Source && Target == O.Target && CohortId == O.CohortId
		&& IndividualId == O.IndividualId && Count == O.Count && BirthWorldSeconds == O.BirthWorldSeconds && bPersistent == O.bPersistent;
}

bool FKSIEcologySimulation::Initialize(FGuid Id, double Seconds, FString& Error)
{
	if (!Id.IsValid() || !Time(Seconds) || Seconds < 0.)
	{
		return Fail(Error, TEXT("Invalid ecology world identity or time."));
	}

	FKSIEcologySimulation Candidate;
	Candidate.WorldId = Id;
	Candidate.WorldSeconds = Seconds;
	*this = MoveTemp(Candidate);
	Error.Reset();
	return true;
}

bool FKSIEcologySimulation::AddSpecies(const FKSIEcologySpecies& Value, FString& Error)
{
	if (Species.Contains(Value.Id))
	{
		return Fail(Error, TEXT("Species identity already exists."));
	}

	FKSIEcologySimulation Candidate = *this;
	Candidate.Species.Add(Value.Id, Value);
	if (!Candidate.Validate(Error))
	{
		return false;
	}

	*this = MoveTemp(Candidate);
	return true;
}

bool FKSIEcologySimulation::AddHabitat(const FKSIEcologyHabitat& Value, FString& Error)
{
	if (Habitats.Contains(Value.Id))
	{
		return Fail(Error, TEXT("Habitat identity already exists."));
	}

	FKSIEcologySimulation Candidate = *this;
	Candidate.Habitats.Add(Value.Id, Value);
	if (!Candidate.Validate(Error))
	{
		return false;
	}

	*this = MoveTemp(Candidate);
	return true;
}

bool FKSIEcologySimulation::AddPopulation(FKSIEcologyPopulation Value, FString& Error)
{
	if (Value.Cohorts.Num() > 16384 || Populations.Contains(Value.Id) || Value.Born || Value.Arrived || Value.Died || Value.Captured || Value.Departed)
	{
		return Fail(Error, TEXT("Population must be a fresh initial allocation."));
	}

	Value.Initial = 0;
	for (auto& Pair : Value.Cohorts)
	{
		const auto* Definition = Species.Find(Value.SpeciesId);
		if (!Quantity(Pair.Value.Count) || (Pair.Value.Count > 0 && Definition
			&& Pair.Value.BirthWorldSeconds + Definition->LifespanSeconds <= WorldSeconds))
		{
			return Fail(Error, TEXT("Initial cohort count or living age is invalid."));
		}

		Value.Initial += Pair.Value.Count;
		if (Definition && Definition->AggregateDietMask)
		{
			Pair.Value.Nutrition.LastStepSeconds = WorldSeconds;
		}
	}

	FKSIEcologySimulation Candidate = *this;
	const FGuid Id = Value.Id;
	Candidate.Populations.Add(Id, MoveTemp(Value));
	if (!Candidate.Validate(Error))
	{
		return false;
	}

	*this = MoveTemp(Candidate);
	return true;
}

bool FKSIEcologySimulation::Apply(const FKSIEcologyEvent& Event, FString& Error)
{
	if (const auto* Previous = Receipts.Find(Event.Id))
	{
		if (!(*Previous == Event))
		{
			return Fail(Error, TEXT("Ecology event ID was reused with a different payload."));
		}

		Error.Reset();
		return true;
	}

	if (!EventShape(Event, WorldSeconds) || Receipts.Num() >= 65536)
	{
		return Fail(Error, TEXT("Invalid ecology event or exhausted receipt budget."));
	}

	FKSIEcologySimulation Candidate = *this;
	if (!Candidate.ApplyCandidate(Event, Error))
	{
		return false;
	}

	Candidate.Receipts.Add(Event.Id, Event);
	if (!Candidate.Validate(Error))
	{
		return false;
	}

	*this = MoveTemp(Candidate);
	return true;
}

bool FKSIEcologySimulation::ApplyCandidate(const FKSIEcologyEvent& E, FString& Error)
{
	auto* Source = Populations.Find(E.Source);
	auto* Target = Populations.Find(E.Target);
	auto* Individual = Individuals.Find(E.IndividualId);
	if (E.Kind == EKSIEcologyEventKind::Birth || E.Kind == EKSIEcologyEventKind::Immigration)
	{
		if (!Target || E.IndividualId.IsValid() || E.bPersistent
			|| (E.Kind == EKSIEcologyEventKind::Immigration && E.Source.IsValid()))
		{
			return Fail(Error, TEXT("Invalid population inflow endpoints."));
		}

		if (E.Kind == EKSIEcologyEventKind::Immigration
			&& E.BirthWorldSeconds + Species[Target->SpeciesId].LifespanSeconds <= WorldSeconds)
		{
			return Fail(Error, TEXT("An incoming cohort must still be within its species lifespan."));
		}

		if (E.Kind == EKSIEcologyEventKind::Birth)
		{
			if (Source != Target || E.BirthWorldSeconds != WorldSeconds)
			{
				return Fail(Error, TEXT("Birth requires its parent population and current world time."));
			}

			int64 Adults = 0;
			const double Maturity = Species[Target->SpeciesId].MaturitySeconds;
			for (const auto& Pair : Target->Cohorts)
			{
				Adults += WorldSeconds - Pair.Value.BirthWorldSeconds >= Maturity ? Pair.Value.Count : 0;
			}

			for (const auto& Pair : Individuals)
			{
				const auto& I = Pair.Value;
				Adults += I.PopulationId == E.Target && I.State == EKSIEcologyLifeState::Alive
					&& WorldSeconds - I.BirthWorldSeconds >= Maturity ? 1 : 0;
			}

			if (E.Count > Adults / 2)
			{
				return Fail(Error, TEXT("Birth count exceeds available adult pairs."));
			}
		}

		auto* Cohort = Target->Cohorts.Find(E.CohortId);
		if (Cohort && Cohort->BirthWorldSeconds != E.BirthWorldSeconds)
		{
			return Fail(Error, TEXT("Cohort identity has a different birth time."));
		}

		if (!Cohort)
		{
			Cohort = &Target->Cohorts.Add(E.CohortId, {E.CohortId, E.BirthWorldSeconds, 0});
			Cohort->Nutrition.LastStepSeconds = WorldSeconds;
		}

		if (Species[Target->SpeciesId].AggregateDietMask)
		{
			FKSIEcologyNutrition Incoming;
			Incoming.LastStepSeconds = WorldSeconds;
			Cohort->Nutrition.Merge(Cohort->Count, Incoming, E.Count, WorldSeconds, Species[Target->SpeciesId].AggregateMetabolism);
		}
		Cohort->Count += E.Count;
		(E.Kind == EKSIEcologyEventKind::Birth ? Target->Born : Target->Arrived) += E.Count;
		return true;
	}

	if (!Source || (E.Kind == EKSIEcologyEventKind::Migration
		? !Target || Source == Target || Source->SpeciesId != Target->SpeciesId : E.Target.IsValid()))
	{
		return Fail(Error, TEXT("Invalid ecology transfer endpoints or species."));
	}

	auto* Cohort = Source->Cohorts.Find(E.CohortId);
	if (!Cohort || Cohort->BirthWorldSeconds != E.BirthWorldSeconds)
	{
		return Fail(Error, TEXT("Missing or mismatched source cohort."));
	}

	if (E.Kind == EKSIEcologyEventKind::Materialize)
	{
		if (E.Count != 1 || !E.IndividualId.IsValid() || UsedIndividualIds.Contains(E.IndividualId) || Cohort->Count < 1)
		{
			return Fail(Error, TEXT("Individual reservation is duplicated or has no available member."));
		}

		--Cohort->Count;
		FKSIEcologyIndividual IndividualRecord = {E.IndividualId, E.Source, E.CohortId, E.BirthWorldSeconds, EKSIEcologyLifeState::Alive, E.bPersistent};
		const auto& Habitat = Habitats[Source->HabitatId];
		IndividualRecord.PositionMeters = (Habitat.MinimumMeters + Habitat.MaximumMeters) * 0.5;
		const auto& Policy = Species[Source->SpeciesId];
		if (Policy.AggregateDietMask)
		{
			Cohort->Nutrition.Charge(WorldSeconds, Policy.AggregateMetabolism);
			IndividualRecord.Nutrition = Cohort->Nutrition;
		}
		Individuals.Add(E.IndividualId, IndividualRecord);
		UsedIndividualIds.Add(E.IndividualId);
		return true;
	}

	if (E.bPersistent)
	{
		return Fail(Error, TEXT("Persistence is only set when materializing an individual."));
	}

	if (E.IndividualId.IsValid())
	{
		if (!Individual || Individual->State != EKSIEcologyLifeState::Alive || Individual->PopulationId != E.Source
			|| Individual->CohortId != E.CohortId || E.Count != 1 || Leases.Contains(E.IndividualId))
		{
			return Fail(Error, TEXT("Individual transfer requires a live, matching, quiescent record."));
		}
	}
	else if (Cohort->Count < E.Count)
	{
		return Fail(Error, TEXT("Aggregate transfer exceeds its unreserved population."));
	}

	if (E.Kind == EKSIEcologyEventKind::Migration)
	{
		auto* Destination = Target->Cohorts.Find(E.CohortId);
		if (Destination && Destination->BirthWorldSeconds != E.BirthWorldSeconds)
		{
			return Fail(Error, TEXT("Destination cohort birth time does not match."));
		}

		if (!Destination)
		{
			Destination = &Target->Cohorts.Add(E.CohortId, {E.CohortId, E.BirthWorldSeconds, 0});
		}

		if (Individual)
		{
			if (auto* B = Behaviors.Find(Individual->Id))
			{
				B->GroupId.Invalidate();
				B->TargetFoodId.Invalidate();
			}
			Individual->PopulationId = E.Target;
			const auto& Habitat = Habitats[Target->HabitatId];
			Individual->PositionMeters = Individual->PositionMeters.ComponentMax(Habitat.MinimumMeters).ComponentMin(Habitat.MaximumMeters);
		}
		else
		{
			const auto& Policy = Species[Source->SpeciesId];
			if (Policy.AggregateDietMask)
			{
				Destination->Nutrition.Merge(Destination->Count, Cohort->Nutrition, E.Count, WorldSeconds, Policy.AggregateMetabolism);
			}
			Cohort->Count -= E.Count;
			Destination->Count += E.Count;
		}

		Source->Departed += E.Count;
		Target->Arrived += E.Count;
	}
	else
	{
		if (Individual)
		{
			Individual->State = E.Kind == EKSIEcologyEventKind::Death ? EKSIEcologyLifeState::Dead : EKSIEcologyLifeState::Captured;
		}
		else
		{
			Cohort->Count -= E.Count;
		}

		(E.Kind == EKSIEcologyEventKind::Death ? Source->Died : Source->Captured) += E.Count;
	}

	return true;
}

bool FKSIEcologySimulation::Materialize(FGuid RequestId, FGuid PopulationId, FGuid CohortId, FGuid IndividualId, bool bPersistent, FString& Error)
{
	FKSIEcologyEvent E;
	E.Id = RequestId;
	E.Kind = EKSIEcologyEventKind::Materialize;
	E.Source = PopulationId;
	E.CohortId = CohortId;
	E.IndividualId = IndividualId;
	E.bPersistent = bPersistent;
	const auto* Population = Populations.Find(PopulationId);
	const auto* Cohort = Population ? Population->Cohorts.Find(CohortId) : nullptr;
	if (!Cohort)
	{
		return Fail(Error, TEXT("Cannot materialize an unknown cohort."));
	}

	E.BirthWorldSeconds = Cohort->BirthWorldSeconds;
	return Apply(E, Error);
}

bool FKSIEcologySimulation::Acquire(FGuid Id, EKSIEcologyRepresentation Representation, FGuid Writer, FKSIEcologyLease& OutLease, FString& Error)
{
	const auto* I = Individuals.Find(Id);
	if (!I || I->State != EKSIEcologyLifeState::Alive || !Writer.IsValid() || Leases.Contains(Id)
		|| NextLease == MAX_uint64 || (Representation != EKSIEcologyRepresentation::Actor && Representation != EKSIEcologyRepresentation::Mass))
	{
		return Fail(Error, TEXT("Ecology execution lease is unavailable or invalid."));
	}

	FKSIEcologyLease Lease = {Id, Writer, Epoch, NextLease++, Representation};
	Leases.Add(Id, Lease);
	OutLease = Lease;
	Error.Reset();
	return true;
}

bool FKSIEcologySimulation::Release(const FKSIEcologyLease& Lease, FString& Error)
{
	if (!IsLeaseCurrent(Lease))
	{
		return Fail(Error, TEXT("Ecology execution lease is stale or belongs to another writer."));
	}

	FKSIEcologySimulation Candidate = *this;
	auto& I = Candidate.Individuals[Lease.IndividualId];
	if (!I.bPersistent)
	{
		auto& Population = Candidate.Populations[I.PopulationId];
		auto& Cohort = Population.Cohorts[I.CohortId];
		const auto& Policy = Species[Population.SpeciesId];
		if (Policy.AggregateDietMask)
		{
			Cohort.Nutrition.Merge(Cohort.Count, I.Nutrition, 1, WorldSeconds, Policy.AggregateMetabolism);
		}
		++Cohort.Count;
		Candidate.Individuals.Remove(Lease.IndividualId);
	}

	Candidate.Leases.Remove(Lease.IndividualId);
	if (!Candidate.Validate(Error))
	{
		return false;
	}

	*this = MoveTemp(Candidate);
	return true;
}

bool FKSIEcologySimulation::Suspend(const FKSIEcologyLease& Lease, FString& Error)
{
	if (!IsLeaseCurrent(Lease))
	{
		return Fail(Error, TEXT("Cannot suspend a stale ecology writer."));
	}

	Leases.Remove(Lease.IndividualId);
	Error.Reset();
	return true;
}

bool FKSIEcologySimulation::ReturnToAggregate(FGuid Id, FString& Error)
{
	const auto* I = Individuals.Find(Id);
	if (!I || I->State != EKSIEcologyLifeState::Alive || Leases.Contains(Id))
	{
		return Fail(Error, TEXT("Only an unowned live reservation may return to its aggregate."));
	}

	FKSIEcologySimulation Candidate = *this;
	if (!I->bPersistent)
	{
		auto& Population = Candidate.Populations[I->PopulationId];
		auto& Cohort = Population.Cohorts[I->CohortId];
		const auto& Policy = Species[Population.SpeciesId];
		if (Policy.AggregateDietMask)
		{
			Cohort.Nutrition.Merge(Cohort.Count, I->Nutrition, 1, WorldSeconds, Policy.AggregateMetabolism);
		}
		++Cohort.Count;
		Candidate.Individuals.Remove(Id);
	}

	if (!Candidate.Validate(Error))
	{
		return false;
	}

	*this = MoveTemp(Candidate);
	return true;
}

bool FKSIEcologySimulation::Transfer(const FKSIEcologyLease& Lease, EKSIEcologyRepresentation Representation,
	FGuid Writer, FKSIEcologyLease& OutLease, FString& Error)
{
	if (!IsLeaseCurrent(Lease) || !Writer.IsValid() || NextLease == MAX_uint64
		|| (Representation != EKSIEcologyRepresentation::Actor && Representation != EKSIEcologyRepresentation::Mass))
	{
		return Fail(Error, TEXT("Cannot transfer an invalid ecology execution lease."));
	}

	FKSIEcologyLease Replacement = {Lease.IndividualId, Writer, Epoch, NextLease++, Representation};
	Leases[Lease.IndividualId] = Replacement;
	OutLease = Replacement;
	Error.Reset();
	return true;
}

bool FKSIEcologySimulation::UpdatePosition(const FKSIEcologyLease& Lease, const FVector& PositionMeters, FString& Error)
{
	if (!IsLeaseCurrent(Lease))
	{
		return Fail(Error, TEXT("Only the current ecology writer may update position."));
	}

	auto& I = Individuals[Lease.IndividualId];
	const auto& Habitat = Habitats[Populations[I.PopulationId].HabitatId];
	if (PositionMeters.ContainsNaN() || !FBox(Habitat.MinimumMeters, Habitat.MaximumMeters).IsInsideOrOn(PositionMeters))
	{
		return Fail(Error, TEXT("Individual position is outside its habitat."));
	}

	if (Behaviors.Contains(I.Id))
	{
		return Fail(Error, TEXT("Autonomous ecology position is owned by the committed behavior model."));
	}

	I.PositionMeters = PositionMeters;
	Error.Reset();
	return true;
}

bool FKSIEcologySimulation::Finish(const FKSIEcologyLease& Lease, const FKSIEcologyEvent& Event, FString& Error)
{
	if (const auto* Previous = Receipts.Find(Event.Id); Previous && *Previous == Event)
	{
		Error.Reset();
		return true;
	}

	if (!IsLeaseCurrent(Lease) || Event.IndividualId != Lease.IndividualId
		|| (Event.Kind != EKSIEcologyEventKind::Death && Event.Kind != EKSIEcologyEventKind::Capture && Event.Kind != EKSIEcologyEventKind::Migration))
	{
		return Fail(Error, TEXT("Ecology result requires the current individual writer and a terminal or migration event."));
	}

	FKSIEcologySimulation Candidate = *this;
	Candidate.Leases.Remove(Lease.IndividualId);
	if (!Candidate.Apply(Event, Error))
	{
		return false;
	}

	*this = MoveTemp(Candidate);
	return true;
}

bool FKSIEcologySimulation::IsLeaseCurrent(const FKSIEcologyLease& Lease) const
{
	const auto* Current = Leases.Find(Lease.IndividualId);
	const auto* I = Individuals.Find(Lease.IndividualId);
	return I && I->State == EKSIEcologyLifeState::Alive && Current && Current->Writer == Lease.Writer
		&& Current->Epoch == Lease.Epoch && Current->Sequence == Lease.Sequence && Current->Representation == Lease.Representation;
}

int64 FKSIEcologySimulation::AggregateCount(FGuid PopulationId) const
{
	int64 Result = 0;
	if (const auto* P = Populations.Find(PopulationId))
	{
		for (const auto& Pair : P->Cohorts)
		{
			Result += Pair.Value.Count;
		}
	}

	return Result;
}

int64 FKSIEcologySimulation::LivingCount(FGuid PopulationId) const
{
	int64 Result = AggregateCount(PopulationId);
	for (const auto& Pair : Individuals)
	{
		Result += Pair.Value.PopulationId == PopulationId && Pair.Value.State == EKSIEcologyLifeState::Alive ? 1 : 0;
	}

	return Result;
}

bool FKSIEcologySimulation::Validate(FString& Error) const
{
	if (!WorldId.IsValid() || !Time(WorldSeconds) || WorldSeconds < 0. || Species.Num() > 256 || Habitats.Num() > 256
		|| Populations.Num() > 256 || Individuals.Num() > 4096 || Receipts.Num() > 65536 || UsedIndividualIds.Num() > 65536
		|| !Epoch.IsValid() || !NextLease)
	{
		return Fail(Error, TEXT("Invalid ecology identity, time or state budget."));
	}

	for (const auto& Pair : Species)
	{
		const auto& S = Pair.Value;
		if (!S.Id.IsValid() || Pair.Key != S.Id || uint8(S.Movement) > 3 || !Time(S.MaturitySeconds) || S.MaturitySeconds < 0.
			|| !Time(S.LifespanSeconds) || S.LifespanSeconds <= S.MaturitySeconds
			|| !Time(S.BirthIntervalSeconds) || S.BirthIntervalSeconds < 0.
			|| FMath::FloorToDouble(S.BirthIntervalSeconds) != S.BirthIntervalSeconds)
		{
			return Fail(Error, TEXT("Invalid species definition."));
		}
	}

	for (const auto& Pair : Habitats)
	{
		const auto& H = Pair.Value;
		if (!H.Id.IsValid() || H.Id != Pair.Key || H.MinimumMeters.ContainsNaN() || H.MaximumMeters.ContainsNaN()
			|| H.MinimumMeters.GetAbsMax() > 1.e9 || H.MaximumMeters.GetAbsMax() > 1.e9
			|| H.MinimumMeters.X >= H.MaximumMeters.X || H.MinimumMeters.Y >= H.MaximumMeters.Y || H.MinimumMeters.Z >= H.MaximumMeters.Z
			|| !H.MovementMask || (H.MovementMask & ~15) || !Quantity(H.Capacity) || !H.Capacity)
		{
			return Fail(Error, TEXT("Invalid habitat geometry, medium or capacity."));
		}
	}

	int32 CohortCount = 0;
	for (const auto& Pair : Populations)
	{
		const auto& P = Pair.Value;
		const auto* S = Species.Find(P.SpeciesId);
		const auto* H = Habitats.Find(P.HabitatId);
		if (!P.Id.IsValid() || P.Id != Pair.Key || !S || !H || !(H->MovementMask & (1 << uint8(S->Movement)))
			|| !Quantity(P.Initial, MaximumLedger) || !Quantity(P.Born, MaximumLedger) || !Quantity(P.Arrived, MaximumLedger)
			|| !Quantity(P.Died, MaximumLedger) || !Quantity(P.Captured, MaximumLedger) || !Quantity(P.Departed, MaximumLedger))
		{
			return Fail(Error, TEXT("Invalid population ownership, medium or ledger."));
		}

		CohortCount += P.Cohorts.Num();
		if (CohortCount > 16384)
		{
			return Fail(Error, TEXT("Ecology cohort budget exceeded."));
		}

		for (const auto& Cohort : P.Cohorts)
		{
			const auto& C = Cohort.Value;
			if (!C.Id.IsValid() || C.Id != Cohort.Key || !Time(C.BirthWorldSeconds) || C.BirthWorldSeconds > WorldSeconds || !Quantity(C.Count))
			{
				return Fail(Error, TEXT("Invalid cohort identity, birth time or population."));
			}
		}

		const int64 Living = LivingCount(P.Id);
		if (!Quantity(Living) || Living != P.Initial + P.Born + P.Arrived - P.Died - P.Captured - P.Departed)
		{
			return Fail(Error, TEXT("Population conservation ledger does not balance."));
		}
	}

	for (const auto& Pair : Habitats)
	{
		const auto& H = Pair.Value;
		int64 Total = 0;
		for (const auto& Population : Populations)
		{
			Total += Population.Value.HabitatId == H.Id ? LivingCount(Population.Key) : 0;
		}

		if (Total > H.Capacity)
		{
			return Fail(Error, TEXT("Habitat capacity exceeded."));
		}
	}

	for (const auto& Pair : Individuals)
	{
		const auto& I = Pair.Value;
		const auto* P = Populations.Find(I.PopulationId);
		const auto* C = P ? P->Cohorts.Find(I.CohortId) : nullptr;
		const auto* H = P ? Habitats.Find(P->HabitatId) : nullptr;
		if (!I.Id.IsValid() || I.Id != Pair.Key || !UsedIndividualIds.Contains(I.Id) || !C
			|| I.BirthWorldSeconds != C->BirthWorldSeconds || uint8(I.State) > 2 || I.bPersistent > 1
			|| !H || I.PositionMeters.ContainsNaN() || !FBox(H->MinimumMeters, H->MaximumMeters).IsInsideOrOn(I.PositionMeters))
		{
			return Fail(Error, TEXT("Invalid identified individual or age ownership."));
		}
	}

	for (FGuid Id : UsedIndividualIds)
	{
		if (!Id.IsValid())
		{
			return Fail(Error, TEXT("Invalid individual identity tombstone."));
		}
	}

	for (const auto& Pair : Receipts)
	{
		if (Pair.Key != Pair.Value.Id || !EventShape(Pair.Value, WorldSeconds))
		{
			return Fail(Error, TEXT("Invalid ecology event receipt."));
		}
	}

	for (const auto& Pair : Leases)
	{
		const auto* I = Individuals.Find(Pair.Key);
		const auto& L = Pair.Value;
		if (!I || I->State != EKSIEcologyLifeState::Alive || L.IndividualId != Pair.Key || !L.Writer.IsValid()
			|| L.Epoch != Epoch || !L.Sequence || L.Sequence >= NextLease
			|| (L.Representation != EKSIEcologyRepresentation::Actor && L.Representation != EKSIEcologyRepresentation::Mass))
		{
			return Fail(Error, TEXT("Invalid ecology execution lease."));
		}
	}

	if (!ValidateBehavior(Error))
	{
		return false;
	}

	Error.Reset();
	return true;
}

bool FKSIEcologySimulation::Serialize(FArchive& Ar)
{
	uint32 Magic = 0x4F434345;
	uint32 Version = 6;
	Ar << Magic << Version << WorldId << WorldSeconds;
	if (Magic != 0x4F434345 || (Version < 1 || Version > 6))
	{
		Ar.SetError();
		return false;
	}

	if (!Map(Ar, Species, 256, Version >= 2 ? 41 : 33, [&](auto& S)
	{
		Ar << S.Id;
		Enum(Ar, S.Movement);
		Ar << S.MaturitySeconds << S.LifespanSeconds;
		if (Version >= 2)
		{
			Ar << S.BirthIntervalSeconds;
		}
		if (Version >= 6)
		{
			Ar << S.AggregateDietMask << S.AggregateMetabolism << S.AggregateEnergyPerUnit << S.AggregateStarvationSeconds;
		}
	}) || !Map(Ar, Habitats, 256, 73, [&](auto& H)
	{
		Ar << H.Id;
		Vector(Ar, H.MinimumMeters);
		Vector(Ar, H.MaximumMeters);
		Ar << H.MovementMask << H.Capacity;
	}) || !Map(Ar, Populations, 256, 100, [&](auto& P)
	{
		Ar << P.Id << P.SpeciesId << P.HabitatId << P.Initial << P.Born << P.Arrived << P.Died << P.Captured << P.Departed;
		Map(Ar, P.Cohorts, 16384, 32, [&](auto& C)
		{
			Ar << C.Id << C.BirthWorldSeconds << C.Count;
			if (Version >= 6)
			{
				Ar << C.Nutrition.Energy << C.Nutrition.StarvedSeconds << C.Nutrition.LastStepSeconds;
			}
		});
		if (Version >= 6)
		{
			Ar << P.AggregateConsumedUnits;
		}
	}) || !Map(Ar, Individuals, 4096, Version >= 3 ? 82 : 58, [&](auto& I)
	{
		Ar << I.Id << I.PopulationId << I.CohortId << I.BirthWorldSeconds;
		Enum(Ar, I.State);
		Ar << I.bPersistent;
		if (Version >= 3)
		{
			Vector(Ar, I.PositionMeters);
		}
		else if (Ar.IsLoading())
		{
			const auto* P = Populations.Find(I.PopulationId);
			const auto* H = P ? Habitats.Find(P->HabitatId) : nullptr;
			I.PositionMeters = H ? (H->MinimumMeters + H->MaximumMeters) * 0.5 : FVector::ZeroVector;
		}
		if (Version >= 6)
		{
			Ar << I.Nutrition.Energy << I.Nutrition.StarvedSeconds << I.Nutrition.LastStepSeconds;
		}
	}) || !Map(Ar, Receipts, 65536, 98, [&](auto& E)
	{
		Ar << E.Id;
		Enum(Ar, E.Kind);
		Ar << E.Source << E.Target << E.CohortId << E.IndividualId << E.Count << E.BirthWorldSeconds << E.bPersistent;
	}))
	{
		return false;
	}

	TArray<FGuid> Used = UsedIndividualIds.Array();
	int32 N = Used.Num();
	if (!Count(Ar, N, 65536, 16))
	{
		return false;
	}

	if (Ar.IsLoading())
	{
		UsedIndividualIds.Reset();
		for (int32 I = 0; I < N; ++I)
		{
			FGuid Id;
			Ar << Id;
			if (!Id.IsValid() || UsedIndividualIds.Contains(Id))
			{
				Ar.SetError();
				return false;
			}

			UsedIndividualIds.Add(Id);
		}
	}
	else
	{
		Used.Sort();
		for (auto& Id : Used)
		{
			Ar << Id;
		}
	}

	if (Version >= 4)
	{
		if (!Map(Ar, Groups, 256, 64, [&](auto& G)
		{
			Ar << G.Id << G.HabitatId;
			Vector(Ar, G.AnchorMeters);
			Ar << G.RadiusMeters;
		}) || !Map(Ar, ForageSources, 4096, 116, [&](auto& F)
		{
			Ar << F.Id << F.HabitatId;
			Vector(Ar, F.PositionMeters);
			Ar << F.FoodMask << F.InitialUnits << F.AvailableUnits << F.CapacityUnits << F.RegrowthPerSecond
				<< F.GrownUnits << F.ConsumedUnits << F.LastStepSeconds;
		}) || !Map(Ar, Behaviors, 4096, Version >= 5 ? 142 : 133, [&](auto& B)
		{
			Ar << B.Id << B.GroupId << B.TargetFoodId << B.SpeedMetersPerSecond << B.DietMask << B.Energy
				<< B.MetabolismPerSecond << B.BiteUnitsPerSecond << B.EnergyPerUnit << B.HungerThreshold
				<< B.StarvationLimitSeconds << B.StarvedSeconds << B.ConsumedUnits << B.LastStepSeconds;
			Enum(Ar, B.Activity);
			if (Version >= 5)
			{
				Ar << B.bUseWorldSpace << B.RadiusMeters;
			}
		}))
		{
			return false;
		}
	}

	return !Ar.IsError();
}

bool FKSIEcologySimulation::Capture(TArray<uint8>& Bytes, FString& Error) const
{
	if (!Validate(Error))
	{
		return false;
	}

	FKSIEcologySimulation Candidate = *this;
	TArray<uint8> Result;
	FMemoryWriter Writer(Result, true);
	if (!Candidate.Serialize(Writer) || Result.Num() > MaximumBytes - 4)
	{
		return Fail(Error, TEXT("Ecology snapshot exceeds its byte budget."));
	}

	uint32 Crc = FCrc::MemCrc32(Result.GetData(), Result.Num());
	Writer << Crc;
	Bytes = MoveTemp(Result);
	Error.Reset();
	return true;
}

bool FKSIEcologySimulation::Restore(const TArray<uint8>& Bytes, FString& Error)
{
	if (Bytes.Num() < 60 || Bytes.Num() > MaximumBytes)
	{
		return Fail(Error, TEXT("Ecology snapshot length is invalid."));
	}

	uint32 Expected = 0;
	FMemory::Memcpy(&Expected, Bytes.GetData() + Bytes.Num() - 4, 4);
	if (Expected != FCrc::MemCrc32(Bytes.GetData(), Bytes.Num() - 4))
	{
		return Fail(Error, TEXT("Ecology snapshot checksum failed."));
	}

	FKSIEcologySimulation Candidate;
	FMemoryReader Reader(Bytes, true);
	if (!Candidate.Serialize(Reader) || Reader.Tell() != Bytes.Num() - 4 || !Candidate.Validate(Error))
	{
		return Fail(Error, TEXT("Ecology snapshot contents are invalid."));
	}

	*this = MoveTemp(Candidate);
	Error.Reset();
	return true;
}
