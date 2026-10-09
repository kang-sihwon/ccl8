#include "CCLCelestialSystem.h"

#include "Math/RandomStream.h"

namespace
{
	constexpr double Pi = 3.14159265358979323846;
	constexpr double Tau = 2. * Pi;
	constexpr double RadiansPerDegree = Pi / 180.;

	double Wrap(double Value, double Period)
	{
		const double Result = FMath::Fmod(Value, Period);
		return Result < 0. ? Result + Period : Result;
	}

	double Phase(double Elapsed, double Period, double InitialDegrees)
	{
		return Wrap(FMath::Fmod(Elapsed, FMath::Abs(Period)) / Period * Tau
			+ InitialDegrees * RadiansPerDegree, Tau);
	}

	FVector3d RotateZ(const FVector3d& Value, double Angle)
	{
		const double C = FMath::Cos(Angle);
		const double S = FMath::Sin(Angle);
		return FVector3d(C * Value.X - S * Value.Y, S * Value.X + C * Value.Y, Value.Z);
	}

	FVector3d RotateX(const FVector3d& Value, double Angle)
	{
		const double C = FMath::Cos(Angle);
		const double S = FMath::Sin(Angle);
		return FVector3d(Value.X, C * Value.Y - S * Value.Z, S * Value.Y + C * Value.Z);
	}

	FVector3d Orient(const FVector3d& Value, const FCCLCelestialBodyDefinition& Body)
	{
		return RotateZ(RotateX(RotateZ(Value, Body.PeriapsisDegrees * RadiansPerDegree),
			Body.InclinationDegrees * RadiansPerDegree), Body.AscendingNodeDegrees * RadiansPerDegree);
	}

	double EccentricAnomaly(double MeanAnomaly, double Eccentricity)
	{
		// E - e sin(E) is monotonic for 0 <= e < 1. Fixed iterations also cover e near one.
		double Low = 0.;
		double High = Tau;
		for (int32 Iteration = 0; Iteration < 64; ++Iteration)
		{
			const double Middle = (Low + High) * 0.5;
			if (Middle - Eccentricity * FMath::Sin(Middle) < MeanAnomaly)
			{
				Low = Middle;
			}
			else
			{
				High = Middle;
			}
		}

		return (Low + High) * 0.5;
	}

	bool Between(double Value, double Minimum, double Maximum)
	{
		return FMath::IsFinite(Value) && Value >= Minimum && Value <= Maximum;
	}

	bool SphereBlocks(const FVector3d& Origin, const FVector3d& Direction, double Distance,
		const FVector3d& Center, double Radius)
	{
		const FVector3d Offset = Center - Origin;
		const double Along = FVector3d::DotProduct(Offset, Direction);
		const double PerpendicularSquared = (Offset - Along * Direction).SquaredLength();
		if (PerpendicularSquared >= Radius * Radius)
		{
			return false;
		}

		const double HalfChord = FMath::Sqrt(Radius * Radius - PerpendicularSquared);
		return Along + HalfChord > 0. && Along - HalfChord < Distance;
	}
}

bool FCCLCelestialSystem::Initialize(const FCCLCelestialDefinitionData& Candidate, FString& Error)
{
	if (!Candidate.DefinitionId.IsValid() || Candidate.Version < 1 || Candidate.Bodies.IsEmpty()
		|| Candidate.Bodies.Num() > 64 || !Between(Candidate.EpochWorldSeconds, -1.e12, 1.e12))
	{
		Error = TEXT("Celestial definition identity, epoch or body count is invalid.");
		return false;
	}

	TMap<FName, int32> Indices;
	int32 Roots = 0;
	int32 Stars = 0;
	for (int32 Index = 0; Index < Candidate.Bodies.Num(); ++Index)
	{
		const FCCLCelestialBodyDefinition& Body = Candidate.Bodies[Index];
		if (Body.BodyId.IsNone() || Indices.Contains(Body.BodyId)
			|| static_cast<uint8>(Body.Kind) > static_cast<uint8>(ECCLCelestialKind::Moon)
			|| !Between(Body.RadiusKm, 1.e-6, 1.e9) || !Between(Body.LuminosityWatts, 0., 1.e35)
			|| !Between(Body.SemiMajorAxisKm, 0., 1.e12) || !Between(Body.Eccentricity, 0., 0.95)
			|| !Between(Body.OrbitalPeriodSeconds, 1., 1.e12)
			|| !Between(FMath::Abs(Body.SpinPeriodSeconds), 1., 1.e12)
			|| !Between(Body.InclinationDegrees, -180., 180.)
			|| !Between(Body.AscendingNodeDegrees, -360., 360.)
			|| !Between(Body.PeriapsisDegrees, -360., 360.)
			|| !Between(Body.MeanAnomalyDegrees, -360., 360.)
			|| !Between(Body.ObliquityDegrees, 0., 180.)
			|| !Between(Body.SpinPhaseDegrees, -360., 360.))
		{
			Error = FString::Printf(TEXT("Invalid or duplicate celestial body: %s."), *Body.BodyId.ToString());
			return false;
		}

		if ((Body.Kind == ECCLCelestialKind::Star) != (Body.LuminosityWatts > 0.))
		{
			Error = TEXT("Only stars emit luminosity, and every star needs positive luminosity.");
			return false;
		}

		Indices.Add(Body.BodyId, Index);
		Stars += Body.Kind == ECCLCelestialKind::Star ? 1 : 0;
		Roots += Body.ParentBodyId.IsNone() ? 1 : 0;
	}

	if (Roots != 1 || Stars == 0)
	{
		Error = TEXT("A celestial hierarchy requires exactly one root and at least one star.");
		return false;
	}

	for (const FCCLCelestialBodyDefinition& Body : Candidate.Bodies)
	{
		if (Body.ParentBodyId.IsNone())
		{
			if (Body.SemiMajorAxisKm != 0. || Body.Eccentricity != 0.)
			{
				Error = TEXT("The root is stationary at the origin and has no orbit.");
				return false;
			}
		}
		else
		{
			const int32* Parent = Indices.Find(Body.ParentBodyId);
			if (!Parent || Body.ParentBodyId == Body.BodyId
				|| Body.SemiMajorAxisKm * (1. - Body.Eccentricity)
					<= Body.RadiusKm + Candidate.Bodies[*Parent].RadiusKm)
			{
				Error = TEXT("An orbit has a missing parent or intersects its parent's surface.");
				return false;
			}
		}
	}

	FCCLCelestialDefinitionData Sorted = Candidate;
	Sorted.Bodies.Reset();
	TArray<FCCLCelestialBodyDefinition> Remaining = Candidate.Bodies;
	Remaining.Sort([](const auto& A, const auto& B) { return A.BodyId.LexicalLess(B.BodyId); });
	TMap<FName, int32> Published;
	TArray<int32> NewParents;
	while (!Remaining.IsEmpty())
	{
		const int32 Ready = Remaining.IndexOfByPredicate([&Published](const auto& Body)
		{
			return Body.ParentBodyId.IsNone() || Published.Contains(Body.ParentBodyId);
		});
		if (Ready == INDEX_NONE)
		{
			Error = TEXT("Celestial parent relationships contain a cycle.");
			return false;
		}

		const FCCLCelestialBodyDefinition Body = Remaining[Ready];
		NewParents.Add(Body.ParentBodyId.IsNone() ? INDEX_NONE : Published[Body.ParentBodyId]);
		Published.Add(Body.BodyId, Sorted.Bodies.Add(Body));
		Remaining.RemoveAt(Ready);
	}

	Definition = MoveTemp(Sorted);
	ParentIndices = MoveTemp(NewParents);
	Error.Reset();
	return true;
}

bool FCCLCelestialSystem::Evaluate(double WorldSeconds, TArray<FCCLCelestialBodyState>& OutStates, FString& Error) const
{
	if (Definition.Bodies.IsEmpty() || !Between(WorldSeconds, 0., 1.e12))
	{
		Error = TEXT("Celestial system is uninitialized or world time is outside [0, 1e12].");
		return false;
	}

	TArray<FCCLCelestialBodyState> Candidate;
	Candidate.Reserve(Definition.Bodies.Num());
	const double Elapsed = WorldSeconds - Definition.EpochWorldSeconds;
	for (int32 Index = 0; Index < Definition.Bodies.Num(); ++Index)
	{
		const FCCLCelestialBodyDefinition& Body = Definition.Bodies[Index];
		FCCLCelestialBodyState State;
		State.BodyId = Body.BodyId;
		if (ParentIndices[Index] != INDEX_NONE)
		{
			const double E = EccentricAnomaly(Phase(Elapsed, Body.OrbitalPeriodSeconds, Body.MeanAnomalyDegrees), Body.Eccentricity);
			const FVector3d Orbit(Body.SemiMajorAxisKm * (FMath::Cos(E) - Body.Eccentricity),
				Body.SemiMajorAxisKm * FMath::Sqrt(1. - Body.Eccentricity * Body.Eccentricity) * FMath::Sin(E), 0.);
			State.PositionKm = Candidate[ParentIndices[Index]].PositionKm + Orient(Orbit, Body);
		}

		const double Tilt = Body.ObliquityDegrees * RadiansPerDegree;
		State.EquatorX = Orient(FVector3d(FMath::Cos(Tilt), 0., -FMath::Sin(Tilt)), Body);
		State.EquatorY = Orient(FVector3d::YAxisVector, Body);
		State.SpinAxis = Orient(FVector3d(FMath::Sin(Tilt), 0., FMath::Cos(Tilt)), Body);
		State.SpinRadians = Phase(Elapsed, Body.SpinPeriodSeconds, Body.SpinPhaseDegrees);
		Candidate.Add(State);
	}

	OutStates = MoveTemp(Candidate);
	Error.Reset();
	return true;
}

bool FCCLCelestialSystem::Observe(double WorldSeconds, const FCCLCelestialObserver& Observer,
	FCCLCelestialObservation& OutObservation, FString& Error) const
{
	const int32 ObserverIndex = Definition.Bodies.IndexOfByPredicate([&Observer](const auto& Body)
	{
		return Body.BodyId == Observer.BodyId;
	});
	if (ObserverIndex == INDEX_NONE || !Between(Observer.LatitudeDegrees, -90., 90.)
		|| !Between(Observer.LongitudeDegrees, -180., 180.) || !Between(Observer.AltitudeMeters, 0., 1.e9)
		|| Definition.Bodies[ObserverIndex].Kind == ECCLCelestialKind::Star)
	{
		Error = TEXT("Observer body or geodetic coordinates are invalid.");
		return false;
	}

	TArray<FCCLCelestialBodyState> States;
	if (!Evaluate(WorldSeconds, States, Error))
	{
		return false;
	}

	const FCCLCelestialBodyState& Home = States[ObserverIndex];
	const double Latitude = Observer.LatitudeDegrees * RadiansPerDegree;
	const double Meridian = Home.SpinRadians + Observer.LongitudeDegrees * RadiansPerDegree;
	const FVector3d Equatorial = FMath::Cos(Meridian) * Home.EquatorX + FMath::Sin(Meridian) * Home.EquatorY;
	const FVector3d Up = FMath::Cos(Latitude) * Equatorial + FMath::Sin(Latitude) * Home.SpinAxis;
	const FVector3d North = -FMath::Sin(Latitude) * Equatorial + FMath::Cos(Latitude) * Home.SpinAxis;
	const FVector3d East = -FMath::Sin(Meridian) * Home.EquatorX + FMath::Cos(Meridian) * Home.EquatorY;
	const auto ToLocal = [&North, &East, &Up](const FVector3d& Direction)
	{
		return FVector3d(FVector3d::DotProduct(Direction, North), FVector3d::DotProduct(Direction, East),
			FVector3d::DotProduct(Direction, Up));
	};
	FCCLCelestialObservation Candidate;
	Candidate.WorldSeconds = WorldSeconds;
	Candidate.ObserverPositionKm = Home.PositionKm + Up * (Definition.Bodies[ObserverIndex].RadiusKm + Observer.AltitudeMeters * 0.001);
	double HighestIrradiance = -1.;
	int32 DominantIndex = INDEX_NONE;
	for (int32 Index = 0; Index < Definition.Bodies.Num(); ++Index)
	{
		const FCCLCelestialBodyDefinition& Body = Definition.Bodies[Index];
		if (Body.Kind != ECCLCelestialKind::Star)
		{
			continue;
		}

		const FVector3d Offset = States[Index].PositionKm - Candidate.ObserverPositionKm;
		const double Distance = Offset.Length();
		if (Distance <= Body.RadiusKm)
		{
			Error = TEXT("Observer is inside a luminous body.");
			return false;
		}

		const FVector3d Direction = Offset / Distance;
		FCCLStellarObservation Star;
		Star.BodyId = Body.BodyId;
		Star.DistanceKm = Distance;
		Star.LocalDirection = ToLocal(Direction);
		Star.ElevationDegrees = FMath::Asin(FMath::Clamp(Star.LocalDirection.Z, -1., 1.)) / RadiansPerDegree;
		Star.AzimuthDegrees = Wrap(FMath::Atan2(Star.LocalDirection.Y, Star.LocalDirection.X) / RadiansPerDegree, 360.);
		Star.DeclinationDegrees = FMath::Asin(FMath::Clamp(FVector3d::DotProduct(Direction, Home.SpinAxis), -1., 1.)) / RadiansPerDegree;
		const double StarMeridian = FMath::Atan2(FVector3d::DotProduct(Direction, Home.EquatorY), FVector3d::DotProduct(Direction, Home.EquatorX));
		Star.SolarHours = Wrap(12. + (Meridian - StarMeridian) * 24. / Tau, 24.);
		const double DistanceMeters = Distance * 1000.;
		Star.NormalIrradianceWattsPerM2 = Body.LuminosityWatts / (4. * Pi * DistanceMeters * DistanceMeters);
		for (int32 Blocker = 0; Blocker < States.Num(); ++Blocker)
		{
			if (Blocker != Index && Blocker != ObserverIndex
				&& SphereBlocks(Candidate.ObserverPositionKm, Direction, Distance, States[Blocker].PositionKm, Definition.Bodies[Blocker].RadiusKm))
			{
				Star.bOcculted = 1;
				break;
			}
		}

		Star.HorizontalIrradianceWattsPerM2 = Star.bOcculted ? 0.
			: Star.NormalIrradianceWattsPerM2 * FMath::Max(0., Star.LocalDirection.Z);
		Candidate.TotalHorizontalIrradianceWattsPerM2 += Star.HorizontalIrradianceWattsPerM2;
		if (Star.NormalIrradianceWattsPerM2 > HighestIrradiance)
		{
			HighestIrradiance = Star.NormalIrradianceWattsPerM2;
			DominantIndex = Index;
			Candidate.DominantStarId = Body.BodyId;
		}

		Candidate.Stars.Add(Star);
	}

	for (int32 Index = 0; Index < States.Num(); ++Index)
	{
		const FCCLCelestialBodyDefinition& Body = Definition.Bodies[Index];
		if (Index == ObserverIndex || Body.Kind == ECCLCelestialKind::Star)
		{
			continue;
		}

		const FVector3d Offset = States[Index].PositionKm - Candidate.ObserverPositionKm;
		const double Distance = Offset.Length();
		if (Distance <= Body.RadiusKm)
		{
			Error = TEXT("Observer is inside another celestial body.");
			return false;
		}

		FCCLCelestialSkyBody SkyBody;
		SkyBody.BodyId = Body.BodyId;
		SkyBody.LocalDirection = ToLocal(Offset / Distance);
		SkyBody.AngularRadiusDegrees = FMath::Asin(Body.RadiusKm / Distance) / RadiansPerDegree;
		const FVector3d ToSun = (States[DominantIndex].PositionKm - States[Index].PositionKm).GetSafeNormal();
		SkyBody.IlluminatedFraction = FMath::Clamp((1. + FVector3d::DotProduct(ToSun, -Offset / Distance)) * 0.5, 0., 1.);
		Candidate.SkyBodies.Add(SkyBody);
	}

	OutObservation = MoveTemp(Candidate);
	Error.Reset();
	return true;
}

FCCLCelestialDefinitionData FCCLCelestialSystem::MakeDefaultDefinition(int32 Seed)
{
	FCCLCelestialDefinitionData Result;
	Result.DefinitionId = FGuid(0xCC1CE1E5, 0x20261009, 0x00000001, 0x00000001);
	Result.Seed = Seed;
	FCCLCelestialBodyDefinition Star;
	Star.BodyId = TEXT("Star");
	Star.Kind = ECCLCelestialKind::Star;
	Star.RadiusKm = 695700.;
	Star.LuminosityWatts = 3.828e26;
	Star.SpinPeriodSeconds = 25. * 86400.;
	Result.Bodies.Add(Star);
	FCCLCelestialBodyDefinition World;
	World.BodyId = TEXT("World");
	World.ParentBodyId = Star.BodyId;
	World.RadiusKm = 6371.;
	World.SemiMajorAxisKm = 149597870.7;
	World.Eccentricity = 0.0167;
	World.OrbitalPeriodSeconds = 365.25 * 86400.;
	World.SpinPeriodSeconds = 86164.0905;
	World.ObliquityDegrees = 23.44;
	World.SpinPhaseDegrees = 180.;
	Result.Bodies.Add(World);
	FRandomStream Random(Seed);
	FCCLCelestialBodyDefinition Moon;
	Moon.BodyId = TEXT("Moon");
	Moon.ParentBodyId = World.BodyId;
	Moon.Kind = ECCLCelestialKind::Moon;
	Moon.RadiusKm = 1737.4;
	Moon.SemiMajorAxisKm = 384400.;
	Moon.Eccentricity = 0.0549;
	Moon.OrbitalPeriodSeconds = 27.321661 * 86400.;
	Moon.SpinPeriodSeconds = Moon.OrbitalPeriodSeconds;
	Moon.InclinationDegrees = 5.145;
	Moon.MeanAnomalyDegrees = Random.GetFraction() * 360.;
	Result.Bodies.Add(Moon);
	FCCLCelestialBodyDefinition Outer = World;
	Outer.BodyId = TEXT("OuterPlanet");
	Outer.RadiusKm = 3389.5;
	Outer.SemiMajorAxisKm = 227939200.;
	Outer.Eccentricity = 0.0934;
	Outer.OrbitalPeriodSeconds = 686.98 * 86400.;
	Outer.SpinPeriodSeconds = 88642.7;
	Outer.ObliquityDegrees = 25.19;
	Outer.MeanAnomalyDegrees = Random.GetFraction() * 360.;
	Result.Bodies.Add(Outer);
	return Result;
}
