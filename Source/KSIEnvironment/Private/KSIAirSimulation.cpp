#include "KSIAirSimulation.h"

#include "KSISurfaceQuery.h"

namespace
{
	bool Range(double Value, double Low, double High)
	{
		return FMath::IsFinite(Value) && Value >= Low && Value <= High;
	}

	bool ValidContents(const FKSIAirContents& C)
	{
		return Range(C.HeatJoules, 0., 1.e15) && Range(C.VaporKg, 0., 1.e12) && Range(C.SmokeKg, 0., 1.e12);
	}

	bool Near(double A, double B)
	{
		return FMath::Abs(A - B) <= FMath::Max(1.e-8, FMath::Max(FMath::Abs(A), FMath::Abs(B)) * 1.e-9);
	}

	bool Same(const FKSIAirContents& A, const FKSIAirContents& B)
	{
		return Near(A.HeatJoules, B.HeatJoules) && Near(A.VaporKg, B.VaporKg) && Near(A.SmokeKg, B.SmokeKg);
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

	bool Fail(FString& Error, const TCHAR* Message)
	{
		Error = Message;
		return false;
	}

	struct FExchange
	{
		FGuid A;
		FGuid B;
		double Volume;
		FKSIAirContents ExteriorDensity;
	};
}

FKSIAirContents FKSIAirContents::operator+(const FKSIAirContents& Other) const
{
	return {HeatJoules + Other.HeatJoules, VaporKg + Other.VaporKg, SmokeKg + Other.SmokeKg};
}

FKSIAirContents FKSIAirContents::operator-(const FKSIAirContents& Other) const
{
	return {HeatJoules - Other.HeatJoules, VaporKg - Other.VaporKg, SmokeKg - Other.SmokeKg};
}

FKSIAirContents FKSIAirContents::operator*(double Scale) const
{
	return {HeatJoules * Scale, VaporKg * Scale, SmokeKg * Scale};
}

void FKSIAirContents::Serialize(FArchive& Ar)
{
	Ar << HeatJoules << VaporKg << SmokeKg;
}

bool FKSIAirSpace::Initialize(double Temperature, double VaporDensity, FString& Error)
{
	if (!Range(Temperature, -100., 2000.) || !Range(VaporDensity, 0., 1000.))
	{
		return Fail(Error, TEXT("Invalid initial air temperature or vapour density."));
	}
	FKSIAirSpace Candidate = *this;
	Candidate.Initial = Candidate.Contents = {(Temperature + 273.15) * VolumeCubicMeters * FKSIAirSimulation::HeatCapacityPerCubicMeter,
		VaporDensity * VolumeCubicMeters, 0.};
	Candidate.Added = Candidate.Removed = {};
	if (!Candidate.Validate(Error))
	{
		return false;
	}
	*this = MoveTemp(Candidate);
	return true;
}

double FKSIAirSpace::TemperatureCelsius() const
{
	return Contents.HeatJoules / (VolumeCubicMeters * FKSIAirSimulation::HeatCapacityPerCubicMeter) - 273.15;
}

bool FKSIAirSpace::Contains(const FVector& Position) const
{
	return Position.X >= MinimumMeters.X && Position.Y >= MinimumMeters.Y && Position.Z >= MinimumMeters.Z
		&& Position.X < MaximumMeters.X && Position.Y < MaximumMeters.Y && Position.Z < MaximumMeters.Z;
}

bool FKSIAirSpace::Validate(FString& Error) const
{
	if (!SpaceId.IsValid() || MinimumMeters.ContainsNaN() || MaximumMeters.ContainsNaN()
		|| MinimumMeters.GetAbsMax() > 1.e7 || MaximumMeters.GetAbsMax() > 1.e7
		|| MaximumMeters.X <= MinimumMeters.X || MaximumMeters.Y <= MinimumMeters.Y || MaximumMeters.Z <= MinimumMeters.Z
		|| !Range(VolumeCubicMeters, 0.1, 1.e9) || !ValidContents(Contents) || !ValidContents(Initial)
		|| !ValidContents(Added) || !ValidContents(Removed) || !Same(Contents, Initial + Added - Removed))
	{
		return Fail(Error, TEXT("Invalid air space, inventory or conservation ledger."));
	}
	return true;
}

void FKSIAirSpace::Serialize(FArchive& Ar)
{
	Ar << SpaceId << ClimateRegionId << MinimumMeters << MaximumMeters << VolumeCubicMeters;
	Contents.Serialize(Ar);
	Initial.Serialize(Ar);
	Added.Serialize(Ar);
	Removed.Serialize(Ar);
}

bool FKSIAirSimulation::AddSpace(const FKSIAirSpace& Space, FString& Error)
{
	if (Spaces.Num() >= MaximumSpaces || Spaces.Contains(Space.SpaceId) || !Space.Validate(Error)
		|| !Same(Space.Added, {}) || !Same(Space.Removed, {}))
	{
		return Fail(Error, TEXT("Air space must be fresh, unique and within the space budget."));
	}
	for (const auto& Pair : Spaces)
	{
		const auto& Other = Pair.Value;
		if (Space.MinimumMeters.X < Other.MaximumMeters.X && Space.MaximumMeters.X > Other.MinimumMeters.X
			&& Space.MinimumMeters.Y < Other.MaximumMeters.Y && Space.MaximumMeters.Y > Other.MinimumMeters.Y
			&& Space.MinimumMeters.Z < Other.MaximumMeters.Z && Space.MaximumMeters.Z > Other.MinimumMeters.Z)
		{
			return Fail(Error, TEXT("Air space ownership bounds overlap."));
		}
	}
	Spaces.Add(Space.SpaceId, Space);
	return true;
}

bool FKSIAirSimulation::AddSource(FGuid Id, double Heat, double Vapor, double Smoke, FString& Error)
{
	const auto* Existing = Spaces.Find(Id);
	if (!Existing || !Range(Heat, -1.e15, 1.e15) || !Range(Vapor, 0., 1.e12) || !Range(Smoke, 0., 1.e12)
		|| -Heat > Existing->Contents.HeatJoules)
	{
		return Fail(Error, TEXT("Invalid air source or insufficient stored heat."));
	}
	FKSIAirSimulation Candidate = *this;
	auto& Space = Candidate.Spaces[Id];
	const FKSIAirContents Added = {FMath::Max(0., Heat), Vapor, Smoke};
	const FKSIAirContents Removed = {FMath::Max(0., -Heat), 0., 0.};
	Space.Contents = Space.Contents + Added - Removed;
	Space.Added = Space.Added + Added;
	Space.Removed = Space.Removed + Removed;
	Candidate.SourcesAdded = Candidate.SourcesAdded + Added;
	Candidate.SourcesRemoved = Candidate.SourcesRemoved + Removed;
	if (!Candidate.Validate(Error))
	{
		return false;
	}
	*this = MoveTemp(Candidate);
	return true;
}

bool FKSIAirSimulation::Advance(double Seconds, const TArray<FKSISurfaceOpening>& Openings,
	const TMap<FGuid, FKSIAirExterior>& Exteriors, FString& Error)
{
	if (!Validate(Error) || !Range(Seconds, 0., 1024.) || Openings.Num() > MaximumOpenings)
	{
		return Fail(Error, TEXT("Invalid air step or work budget."));
	}
	for (const auto& Pair : Exteriors)
	{
		const auto& E = Pair.Value;
		if (!Spaces.Contains(Pair.Key) || !Range(E.TemperatureCelsius, -100., 2000.)
			|| !Range(E.VaporKgPerCubicMeter, 0., 1000.) || !Range(E.WindMetersPerSecond, 0., 200.))
		{
			return Fail(Error, TEXT("Invalid exterior air boundary."));
		}
	}
	TSet<FGuid> Seen;
	TArray<FKSISurfaceOpening> Active;
	for (const auto& O : Openings)
	{
		if (!O.OpeningId.IsValid() || Seen.Contains(O.OpeningId) || !Range(O.OpenFraction, 0., 1.)
			|| !Range(O.HalfExtentsMeters.X, 0.001, 10000.) || !Range(O.HalfExtentsMeters.Y, 0.001, 10000.)
			|| O.SpaceA == O.SpaceB)
		{
			return Fail(Error, TEXT("Invalid or duplicate ventilation opening."));
		}
		Seen.Add(O.OpeningId);
		const bool A = Spaces.Contains(O.SpaceA), B = Spaces.Contains(O.SpaceB);
		if (!A && !B)
		{
			continue;
		}
		if ((O.SpaceA.IsValid() && !A) || (O.SpaceB.IsValid() && !B)
			|| ((!A || !B) && !Exteriors.Contains(A ? O.SpaceA : O.SpaceB)))
		{
			return Fail(Error, TEXT("Ventilation requires both indoor endpoints and its exterior boundary."));
		}
		Active.Add(O);
	}
	Active.Sort([](const auto& A, const auto& B) { return GuidLess(A.OpeningId, B.OpeningId); });

	FKSIAirSimulation Candidate = *this;
	const int32 Steps = FMath::Max(1, FMath::CeilToInt32(Seconds / 0.25));
	const double Dt = Seconds / Steps;
	for (int32 I = 0; I < Steps && Dt > 0.; ++I)
	{
		TArray<FExchange> Exchanges;
		TMap<FGuid, double> RequestedVolumes;
		for (const auto& O : Active)
		{
			const auto* A = Candidate.FindSpace(O.SpaceA);
			const auto* B = Candidate.FindSpace(O.SpaceB);
			const auto* Outside = (!A || !B) ? Exteriors.Find(A ? O.SpaceA : O.SpaceB) : nullptr;
			const double TA = A ? A->TemperatureCelsius() : Outside->TemperatureCelsius;
			const double TB = B ? B->TemperatureCelsius() : Outside->TemperatureCelsius;
			const double Limit = A && B ? FMath::Min(A->VolumeCubicMeters, B->VolumeCubicMeters)
				: (A ? A->VolumeCubicMeters : B->VolumeCubicMeters);
			const double Wind = Outside ? Outside->WindMetersPerSecond : 0.;
			const double Speed = 0.05 + Wind * 0.1 + FMath::Sqrt(FMath::Abs(TA - TB)) * 0.01;
			const double Volume = FMath::Min(Limit * 0.25, 4. * O.HalfExtentsMeters.X * O.HalfExtentsMeters.Y * O.OpenFraction * Speed * Dt);
			FExchange X = {O.SpaceA, O.SpaceB, Volume, {}};
			if (Outside)
			{
				X.ExteriorDensity = {(Outside->TemperatureCelsius + 273.15) * HeatCapacityPerCubicMeter, Outside->VaporKgPerCubicMeter, 0.};
			}
			Exchanges.Add(X);
			if (A)
			{
				RequestedVolumes.FindOrAdd(O.SpaceA) += Volume;
			}
			if (B)
			{
				RequestedVolumes.FindOrAdd(O.SpaceB) += Volume;
			}
		}
		TMap<FGuid, double> Fractions;
		TMap<FGuid, FKSIAirContents> Incoming;
		auto Scale = [&](FGuid Id)
		{
			const auto* S = Candidate.FindSpace(Id);
			const double Requested = RequestedVolumes.FindRef(Id);
			return S && Requested > 0. ? FMath::Min(1., S->VolumeCubicMeters * 0.5 / Requested) : 1.;
		};
		for (const auto& X : Exchanges)
		{
			const auto* A = Candidate.FindSpace(X.A);
			const auto* B = Candidate.FindSpace(X.B);
			const double Volume = X.Volume * FMath::Min(Scale(X.A), Scale(X.B));
			const auto FromA = A ? A->Contents * (Volume / A->VolumeCubicMeters) : X.ExteriorDensity * Volume;
			const auto FromB = B ? B->Contents * (Volume / B->VolumeCubicMeters) : X.ExteriorDensity * Volume;
			if (A)
			{
				Fractions.FindOrAdd(X.A) += Volume / A->VolumeCubicMeters;
				Incoming.FindOrAdd(X.A) = Incoming.FindRef(X.A) + FromB;
			}
			else
			{
				Candidate.ExteriorImported = Candidate.ExteriorImported + FromA;
				Candidate.ExteriorExported = Candidate.ExteriorExported + FromB;
			}
			if (B)
			{
				Fractions.FindOrAdd(X.B) += Volume / B->VolumeCubicMeters;
				Incoming.FindOrAdd(X.B) = Incoming.FindRef(X.B) + FromA;
			}
			else
			{
				Candidate.ExteriorImported = Candidate.ExteriorImported + FromB;
				Candidate.ExteriorExported = Candidate.ExteriorExported + FromA;
			}
		}
		for (auto& Pair : Candidate.Spaces)
		{
			auto& S = Pair.Value;
			const double Fraction = Fractions.FindRef(Pair.Key);
			const auto Lost = S.Contents * Fraction;
			const auto Received = Incoming.FindRef(Pair.Key);
			S.Contents = S.Contents * (1. - Fraction) + Received;
			S.Added = S.Added + Received;
			S.Removed = S.Removed + Lost;
		}
	}
	if (!Candidate.Validate(Error))
	{
		return false;
	}
	*this = MoveTemp(Candidate);
	return true;
}

FKSIAirContents FKSIAirSimulation::Total() const
{
	FKSIAirContents Result;
	for (const auto& Pair : Spaces)
	{
		Result = Result + Pair.Value.Contents;
	}
	return Result;
}

const FKSIAirSpace* FKSIAirSimulation::FindSpaceAt(const FVector& Position) const
{
	for (const auto& Pair : Spaces)
	{
		if (Pair.Value.Contains(Position))
		{
			return &Pair.Value;
		}
	}
	return nullptr;
}

bool FKSIAirSimulation::Validate(FString& Error) const
{
	if (Spaces.Num() > MaximumSpaces || !ValidContents(SourcesAdded) || !ValidContents(SourcesRemoved)
		|| !ValidContents(ExteriorImported) || !ValidContents(ExteriorExported))
	{
		return Fail(Error, TEXT("Invalid global air ledger."));
	}
	FKSIAirContents Initial;
	TArray<FGuid> Ids;
	Spaces.GetKeys(Ids);
	for (int32 I = 0; I < Ids.Num(); ++I)
	{
		const auto& S = Spaces[Ids[I]];
		if (Ids[I] != S.SpaceId || !S.Validate(Error))
		{
			return false;
		}
		Initial = Initial + S.Initial;
		for (int32 J = I + 1; J < Ids.Num(); ++J)
		{
			const auto& Other = Spaces[Ids[J]];
			if (S.MinimumMeters.X < Other.MaximumMeters.X && S.MaximumMeters.X > Other.MinimumMeters.X
				&& S.MinimumMeters.Y < Other.MaximumMeters.Y && S.MaximumMeters.Y > Other.MinimumMeters.Y
				&& S.MinimumMeters.Z < Other.MaximumMeters.Z && S.MaximumMeters.Z > Other.MinimumMeters.Z)
			{
				return Fail(Error, TEXT("Air space ownership bounds overlap."));
			}
		}
	}
	if (!Same(Total(), Initial + SourcesAdded - SourcesRemoved + ExteriorImported - ExteriorExported))
	{
		return Fail(Error, TEXT("Air exchange does not conserve global heat, vapour or smoke."));
	}
	return true;
}

bool FKSIAirSimulation::Serialize(FArchive& Ar)
{
	uint32 Version = 1;
	int32 Count = Spaces.Num();
	Ar << Version << Count;
	if (Ar.IsError() || Version != 1 || Count < 0 || Count > MaximumSpaces
		|| (Ar.IsLoading() && int64(Count) * 184 + 96 > Ar.TotalSize() - Ar.Tell()))
	{
		return false;
	}
	FKSIAirSimulation Candidate;
	if (Ar.IsSaving())
	{
		Candidate = *this;
	}
	Candidate.SourcesAdded.Serialize(Ar);
	Candidate.SourcesRemoved.Serialize(Ar);
	Candidate.ExteriorImported.Serialize(Ar);
	Candidate.ExteriorExported.Serialize(Ar);
	TArray<FGuid> Ids;
	Candidate.Spaces.GetKeys(Ids);
	Ids.Sort(GuidLess);
	for (int32 I = 0; I < Count; ++I)
	{
		FKSIAirSpace Space;
		if (Ar.IsSaving())
		{
			Space = Candidate.Spaces[Ids[I]];
		}
		Space.Serialize(Ar);
		if (Ar.IsLoading())
		{
			if (Candidate.Spaces.Contains(Space.SpaceId))
			{
				return false;
			}
			Candidate.Spaces.Add(Space.SpaceId, MoveTemp(Space));
		}
	}
	FString Error;
	if (Ar.IsError() || !Candidate.Validate(Error))
	{
		return false;
	}
	if (Ar.IsLoading())
	{
		*this = MoveTemp(Candidate);
	}
	return true;
}
