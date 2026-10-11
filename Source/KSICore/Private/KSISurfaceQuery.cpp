#include "KSISurfaceQuery.h"

namespace
{
	bool FiniteVector(const FVector3d& Value)
	{
		return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y) && FMath::IsFinite(Value.Z)
			&& Value.GetAbsMax() <= 1.e9;
	}

	bool UnitVector(const FVector3d& Value)
	{
		return FiniteVector(Value) && FMath::Abs(Value.SquaredLength() - 1.) <= 1.e-6;
	}

	bool Extents(const FVector2d& Value)
	{
		return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y)
			&& Value.X > 0. && Value.Y > 0. && Value.X <= 1.e7 && Value.Y <= 1.e7;
	}

	bool Fraction(double Value)
	{
		return FMath::IsFinite(Value) && Value >= 0. && Value <= 1.;
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

	bool InsideOpening(const FVector2d& UV, const FKSISurfaceOpening& Opening)
	{
		const double LowU = Opening.CenterUV.X - Opening.HalfExtentsMeters.X;
		return Opening.OpenFraction > 0. && UV.X >= LowU
			&& UV.X <= LowU + 2. * Opening.HalfExtentsMeters.X * Opening.OpenFraction
			&& FMath::Abs(UV.Y - Opening.CenterUV.Y) <= Opening.HalfExtentsMeters.Y;
	}
}

bool FKSISurfaceScene::QuerySurfaces(const FKSISurfaceQuery& Query, TArray<FKSISurfaceSample>& OutSamples, FString& Error) const
{
	if (Revision == 0 || Query.BodyId.IsNone() || !FiniteVector(Query.OriginMeters) || !UnitVector(Query.Direction)
		|| !FMath::IsFinite(Query.MinimumDistanceMeters) || Query.MinimumDistanceMeters < 0.
		|| !FMath::IsFinite(Query.MaximumDistanceMeters) || Query.MaximumDistanceMeters <= Query.MinimumDistanceMeters
		|| Query.MaximumDistanceMeters > 1.e7 || Query.KindMask == 0 || (Query.KindMask & ~15) != 0
		|| Query.MaximumResults < 1 || Query.MaximumResults > 4096
		|| (Query.RequiredRevision != 0 && Query.RequiredRevision != Revision)
		|| (Query.RequiredEpoch.IsValid() && Query.RequiredEpoch != Epoch))
	{
		Error = TEXT("Surface query is invalid or its revision is stale.");
		return false;
	}

	TArray<FKSISurfaceSample> Candidate;
	for (const FKSISurfacePatch& Patch : Patches)
	{
		if (Patch.BodyId != Query.BodyId || (Query.KindMask & static_cast<uint8>(Patch.Kind)) == 0)
		{
			continue;
		}

		const double Denominator = FVector3d::DotProduct(Query.Direction, Patch.Normal);
		if (FMath::Abs(Denominator) <= 1.e-12)
		{
			continue;
		}

		const double Distance = FVector3d::DotProduct(Patch.CenterMeters - Query.OriginMeters, Patch.Normal) / Denominator;
		if (Distance < Query.MinimumDistanceMeters || Distance > Query.MaximumDistanceMeters)
		{
			continue;
		}

		const FVector3d Position = Query.OriginMeters + Query.Direction * Distance;
		const FVector3d Offset = Position - Patch.CenterMeters;
		const FVector2d UV(FVector3d::DotProduct(Offset, Patch.TangentU), FVector3d::DotProduct(Offset, Patch.TangentV));
		if (FMath::Abs(UV.X) > Patch.HalfExtentsMeters.X || FMath::Abs(UV.Y) > Patch.HalfExtentsMeters.Y)
		{
			continue;
		}

		bool bThroughOpening = false;
		if (const TArray<int32>* PatchOpenings = OpeningsBySurface.Find(Patch.SurfaceId))
		{
			for (const int32 Index : *PatchOpenings)
			{
				if (InsideOpening(UV, Openings[Index]))
				{
					bThroughOpening = true;
					break;
				}
			}
		}

		if (bThroughOpening)
		{
			continue;
		}

		if (Candidate.Num() == Query.MaximumResults)
		{
			Error = TEXT("Surface query exceeded its result budget; no partial occlusion result was published.");
			return false;
		}

		FKSISurfaceSample Sample;
		Sample.SurfaceId = Patch.SurfaceId;
		Sample.BodyId = Patch.BodyId;
		Sample.Revision = Revision;
		Sample.Epoch = Epoch;
		Sample.PositionMeters = Position;
		Sample.Normal = Patch.Normal;
		Sample.DistanceMeters = Distance;
		Sample.MaterialId = Patch.MaterialId;
		Sample.Kind = Patch.Kind;
		Sample.WaterDepthMeters = Patch.WaterDepthMeters;
		Sample.SnowDepthMeters = Patch.SnowDepthMeters;
		Sample.Transmission = Patch.Transmission;
		Candidate.Add(Sample);
	}

	for (const auto& Pair : GeometryProviders)
	{
		FKSISurfaceQuery GeometryQuery = Query;
		GeometryQuery.RequiredRevision = 0;
		GeometryQuery.RequiredEpoch.Invalidate();
		TArray<FKSISurfaceSample> GeometrySamples;
		if (!Pair.Value->QuerySurfaces(GeometryQuery, GeometrySamples, Error))
		{
			return false;
		}

		if (Candidate.Num() + GeometrySamples.Num() > Query.MaximumResults)
		{
			Error = TEXT("Combined surface query exceeded its result budget.");
			return false;
		}

		for (auto& Sample : GeometrySamples)
		{
			Sample.SourceRevision = Sample.Revision;
			Sample.SourceEpoch = Sample.Epoch;
			Sample.Revision = Revision;
			Sample.Epoch = Epoch;
			Candidate.Add(MoveTemp(Sample));
		}
	}

	Candidate.Sort([](const FKSISurfaceSample& A, const FKSISurfaceSample& B)
	{
		return A.DistanceMeters == B.DistanceMeters ? GuidLess(A.SurfaceId, B.SurfaceId) : A.DistanceMeters < B.DistanceMeters;
	});
	OutSamples = MoveTemp(Candidate);
	Error.Reset();
	return true;
}

bool FKSISurfaceScene::Replace(const TArray<FKSISurfacePatch>& CandidatePatches, const TArray<FKSISurfaceOpening>& CandidateOpenings,
	uint64 NewRevision, FString& Error)
{
	if (NewRevision <= Revision || CandidatePatches.Num() > 4096 || CandidateOpenings.Num() > 4096)
	{
		Error = TEXT("Surface replacement requires a newer revision and at most 4096 patches/openings.");
		return false;
	}

	TMap<FGuid, int32> PatchIndices;
	for (int32 Index = 0; Index < CandidatePatches.Num(); ++Index)
	{
		const FKSISurfacePatch& Patch = CandidatePatches[Index];
		const uint8 Kind = static_cast<uint8>(Patch.Kind);
		if (!Patch.SurfaceId.IsValid() || PatchIndices.Contains(Patch.SurfaceId) || Patch.BodyId.IsNone()
			|| !FiniteVector(Patch.CenterMeters) || !UnitVector(Patch.Normal)
			|| !UnitVector(Patch.TangentU) || !UnitVector(Patch.TangentV)
			|| !FVector3d::CrossProduct(Patch.TangentU, Patch.TangentV).Equals(Patch.Normal, 1.e-6)
			|| !Extents(Patch.HalfExtentsMeters) || Patch.MaterialId.IsNone()
			|| (Kind != 1 && Kind != 2 && Kind != 4 && Kind != 8)
			|| !Fraction(Patch.Transmission.Sun) || !Fraction(Patch.Transmission.Precipitation) || !Fraction(Patch.Transmission.Wind)
			|| !FMath::IsFinite(Patch.WaterDepthMeters) || Patch.WaterDepthMeters < 0. || Patch.WaterDepthMeters > 1.e6
			|| !FMath::IsFinite(Patch.SnowDepthMeters) || Patch.SnowDepthMeters < 0. || Patch.SnowDepthMeters > 1.e6)
		{
			Error = TEXT("Surface patch identity, basis, material or physical values are invalid.");
			return false;
		}

		PatchIndices.Add(Patch.SurfaceId, Index);
	}

	TSet<FGuid> OpeningIds;
	TMap<FGuid, TArray<int32>> NewOpeningMap;
	for (int32 Index = 0; Index < CandidateOpenings.Num(); ++Index)
	{
		const FKSISurfaceOpening& Opening = CandidateOpenings[Index];
		const int32* PatchIndex = PatchIndices.Find(Opening.SurfaceId);
		if (!Opening.OpeningId.IsValid() || OpeningIds.Contains(Opening.OpeningId) || !PatchIndex
			|| !Extents(Opening.HalfExtentsMeters) || !FMath::IsFinite(Opening.CenterUV.X) || !FMath::IsFinite(Opening.CenterUV.Y)
			|| !Fraction(Opening.OpenFraction) || Opening.SpaceA == Opening.SpaceB)
		{
			Error = TEXT("Opening identity, geometry, fraction or connected spaces are invalid.");
			return false;
		}

		const FKSISurfacePatch& Patch = CandidatePatches[*PatchIndex];
		if (FMath::Abs(Opening.CenterUV.X) + Opening.HalfExtentsMeters.X > Patch.HalfExtentsMeters.X
			|| FMath::Abs(Opening.CenterUV.Y) + Opening.HalfExtentsMeters.Y > Patch.HalfExtentsMeters.Y)
		{
			Error = TEXT("Opening extends beyond its parent surface.");
			return false;
		}

		TArray<int32>& OnPatch = NewOpeningMap.FindOrAdd(Opening.SurfaceId);
		for (const int32 OtherIndex : OnPatch)
		{
			const FKSISurfaceOpening& Other = CandidateOpenings[OtherIndex];
			if (FMath::Abs(Opening.CenterUV.X - Other.CenterUV.X) < Opening.HalfExtentsMeters.X + Other.HalfExtentsMeters.X
				&& FMath::Abs(Opening.CenterUV.Y - Other.CenterUV.Y) < Opening.HalfExtentsMeters.Y + Other.HalfExtentsMeters.Y)
			{
				Error = TEXT("Overlapping openings would double-count ventilation area.");
				return false;
			}
		}

		OnPatch.Add(Index);
		OpeningIds.Add(Opening.OpeningId);
	}

	Patches = CandidatePatches;
	Openings = CandidateOpenings;
	OpeningsBySurface = MoveTemp(NewOpeningMap);
	Revision = NewRevision;
	Error.Reset();
	return true;
}

double FKSISurfaceScene::GetEffectiveOpeningAreaM2(const FGuid& SpaceId) const
{
	double Area = 0.;
	for (const FKSISurfaceOpening& Opening : Openings)
	{
		if (Opening.SpaceA == SpaceId || Opening.SpaceB == SpaceId)
		{
			Area += 4. * Opening.HalfExtentsMeters.X * Opening.HalfExtentsMeters.Y * Opening.OpenFraction;
		}
	}

	return Area;
}

bool FKSIShelterEvaluator::Evaluate(const IKSISurfaceProvider& Provider, const FKSIShelterQuery& Query,
	FKSIShelterSample& OutSample, FString& Error)
{
	const uint64 Revision = Provider.GetRevision();
	const FGuid Epoch = Provider.GetEpoch();
	if (Revision == 0 || !Epoch.IsValid() || (Query.RequiredEpoch.IsValid() && Query.RequiredEpoch != Epoch)
		|| (Query.RequiredRevision != 0 && Query.RequiredRevision != Revision)
		|| !FMath::IsFinite(Query.ProbeRadiusMeters) || Query.ProbeRadiusMeters < 0. || Query.ProbeRadiusMeters > 10000.)
	{
		Error = TEXT("Shelter query has an invalid footprint or revision.");
		return false;
	}

	FKSIShelterSample Candidate;
	Candidate.Revision = Revision;
	Candidate.Epoch = Epoch;
	const FVector3d Directions[] = { Query.ToSun, Query.ToPrecipitationSource, Query.ToWindSource };
	const FVector3d Offsets[] = { FVector3d::ZeroVector, FVector3d(Query.ProbeRadiusMeters, 0., 0.),
		FVector3d(-Query.ProbeRadiusMeters, 0., 0.), FVector3d(0., Query.ProbeRadiusMeters, 0.), FVector3d(0., -Query.ProbeRadiusMeters, 0.) };
	const int32 ProbeCount = Query.ProbeRadiusMeters > 0. ? 5 : 1;
	double Channels[3] = { 0., 0., 0. };
	for (int32 Channel = 0; Channel < 3; ++Channel)
	{
		for (int32 Probe = 0; Probe < ProbeCount; ++Probe)
		{
			FKSISurfaceQuery Ray;
			Ray.BodyId = Query.BodyId;
			Ray.OriginMeters = Query.PositionMeters + Offsets[Probe];
			Ray.Direction = Directions[Channel];
			Ray.MaximumDistanceMeters = Query.MaximumDistanceMeters;
			Ray.RequiredRevision = Revision;
			Ray.RequiredEpoch = Epoch;
			TArray<FKSISurfaceSample> Samples;
			if (!Provider.QuerySurfaces(Ray, Samples, Error))
			{
				return false;
			}

			double Transmission = 1.;
			for (const FKSISurfaceSample& Sample : Samples)
			{
				if (Sample.Revision != Revision || Sample.Epoch != Epoch || !Fraction(Sample.Transmission.Sun)
					|| !Fraction(Sample.Transmission.Precipitation) || !Fraction(Sample.Transmission.Wind))
				{
					Error = TEXT("Surface provider returned an inconsistent shelter sample.");
					return false;
				}

				Transmission *= Channel == 0 ? Sample.Transmission.Sun
					: Channel == 1 ? Sample.Transmission.Precipitation : Sample.Transmission.Wind;
			}

			Channels[Channel] += Transmission / ProbeCount;
		}
	}

	if (Provider.GetRevision() != Revision || Provider.GetEpoch() != Epoch)
	{
		Error = TEXT("Surface revision changed while evaluating shelter.");
		return false;
	}

	Candidate.Transmission.Sun = Channels[0];
	Candidate.Transmission.Precipitation = Channels[1];
	Candidate.Transmission.Wind = Channels[2];
	OutSample = Candidate;
	Error.Reset();
	return true;
}

void FKSISurfaceScene::SetGeometryProvider(FGuid Id, TSharedPtr<const IKSISurfaceProvider> Provider)
{
	if (Provider)
	{
		GeometryProviders.Add(Id, MoveTemp(Provider));
	}
	else
	{
		GeometryProviders.Remove(Id);
	}

	Epoch = FGuid::NewGuid();
}

void FKSISurfaceScene::CopyGeometryProviders(const FKSISurfaceScene& Other)
{
	GeometryProviders = Other.GeometryProviders;
}


bool IKSISurfaceProvider::QuerySolidPoint(FName BodyId, const FVector3d& PositionMeters, bool& bOutSolid, FString& Error) const
{
	Error = TEXT("Surface provider does not support volume occupancy queries.");
	return false;
}

bool IKSISurfaceProvider::QueryOpeningFraction(const FKSISurfaceOpening& Opening, double& OutFraction, FString& Error) const
{
	Error = TEXT("Surface provider does not resolve ventilation openings.");
	return false;
}

bool FKSISurfaceScene::QuerySolidPoint(FName BodyId, const FVector3d& PositionMeters, bool& bOutSolid, FString& Error) const
{
	if (!Revision || BodyId.IsNone() || !FiniteVector(PositionMeters))
	{
		Error = TEXT("Scene occupancy query is invalid.");
		return false;
	}

	bool bSolid = false;
	for (const auto& Pair : GeometryProviders)
	{
		bool bProviderSolid = false;
		if (!Pair.Value->QuerySolidPoint(BodyId, PositionMeters, bProviderSolid, Error))
		{
			return false;
		}

		bSolid |= bProviderSolid;
	}

	bOutSolid = bSolid;
	Error.Reset();
	return true;
}

bool FKSISurfaceScene::QueryOpeningFraction(const FKSISurfaceOpening& Opening, double& OutFraction, FString& Error) const
{
	const auto* Current = Openings.FindByPredicate([&](const auto& O) { return O.OpeningId == Opening.OpeningId; });
	const auto* Patch = Patches.FindByPredicate([&](const auto& P) { return P.SurfaceId == Opening.SurfaceId; });
	if (!Revision || !Current || !Patch || Current->SurfaceId != Opening.SurfaceId
		|| Current->SpaceA != Opening.SpaceA || Current->SpaceB != Opening.SpaceB
		|| Current->CenterUV != Opening.CenterUV || Current->HalfExtentsMeters != Opening.HalfExtentsMeters
		|| Current->OpenFraction != Opening.OpenFraction)
	{
		Error = TEXT("Ventilation opening does not match the committed surface scene.");
		return false;
	}

	const uint64 QueryRevision = Revision;
	const FGuid QueryEpoch = Epoch;
	double Sum = 0.;
	constexpr int32 AxisSamples = 4;
	for (int32 V = 0; V < AxisSamples && Opening.OpenFraction > 0.; ++V)
	{
		for (int32 U = 0; U < AxisSamples; ++U)
		{
			const double X = Opening.CenterUV.X - Opening.HalfExtentsMeters.X
				+ 2. * Opening.HalfExtentsMeters.X * Opening.OpenFraction * (U + 0.5) / AxisSamples;
			const double Y = Opening.CenterUV.Y - Opening.HalfExtentsMeters.Y
				+ 2. * Opening.HalfExtentsMeters.Y * (V + 0.5) / AxisSamples;
			const FVector3d Position = Patch->CenterMeters + X * Patch->TangentU + Y * Patch->TangentV;
			bool bSolid = false;
			if (!QuerySolidPoint(Patch->BodyId, Position, bSolid, Error))
			{
				return false;
			}

			if (bSolid)
			{
				continue;
			}

			FKSISurfaceQuery Ray;
			Ray.BodyId = Patch->BodyId;
			Ray.OriginMeters = Position - Patch->Normal * 0.5;
			Ray.Direction = Patch->Normal;
			Ray.MinimumDistanceMeters = 0.;
			Ray.MaximumDistanceMeters = 1.;
			Ray.RequiredRevision = QueryRevision;
			Ray.RequiredEpoch = QueryEpoch;
			Ray.KindMask = uint8(EKSISurfaceKind::Terrain) | uint8(EKSISurfaceKind::Structure);
			TArray<FKSISurfaceSample> Hits;
			if (!QuerySurfaces(Ray, Hits, Error))
			{
				return false;
			}

			double Transmission = 1.;
			for (const auto& Hit : Hits)
			{
				if (Hit.Revision != QueryRevision || Hit.Epoch != QueryEpoch || !Fraction(Hit.Transmission.Wind))
				{
					Error = TEXT("Ventilation geometry returned an inconsistent sample.");
					return false;
				}

				Transmission *= Hit.Transmission.Wind;
			}

			Sum += Transmission;
		}
	}

	if (Revision != QueryRevision || Epoch != QueryEpoch)
	{
		Error = TEXT("Surface scene changed during ventilation query.");
		return false;
	}

	OutFraction = Opening.OpenFraction * Sum / (AxisSamples * AxisSamples);
	Error.Reset();
	return true;
}
