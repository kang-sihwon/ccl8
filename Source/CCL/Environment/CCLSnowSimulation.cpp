#include "CCLSurfaceSimulation.h"

// Snow mass uses liquid-water-equivalent m3; visible volume carries density and compaction.
double FCCLSurfaceGrid::SnowDepthMeters(int32 Index) const
{
	return Cells[Index].SnowVolumeCubicMeters / FMath::Square(SpacingMeters);
}

bool FCCLSurfaceSimulation::SampleSnow(const FVector& Position, FCCLSnowSample& Sample) const
{
	Sample = {};
	if (Position.ContainsNaN())
	{
		return false;
	}
	double BestDistance = MAX_dbl;
	for (const auto& Pair : Regions)
	{
		const auto& G = Pair.Value;
		const FVector Local = (Position - G.OriginMeters) / G.SpacingMeters;
		if (Local.X < 0. || Local.Y < 0. || Local.X >= G.Size.X || Local.Y >= G.Size.Y)
		{
			continue;
		}
		const int32 X = FMath::FloorToInt(Local.X), Y = FMath::FloorToInt(Local.Y);
		if (X < 0 || Y < 0 || X >= G.Size.X || Y >= G.Size.Y)
		{
			continue;
		}
		const int32 I = X + Y * G.Size.X;
		const auto& C = G.Cells[I];
		const double Distance = FMath::Abs(Position.Z - C.BedMeters);
		if (C.SnowCubicMeters <= 0. || Distance > 1.5 || Distance >= BestDistance)
		{
			continue;
		}
		BestDistance = Distance;
		Sample.RegionId = G.RegionId;
		Sample.Revision = G.Revision;
		Sample.BedMeters = C.BedMeters;
		Sample.DepthMeters = G.SnowDepthMeters(I);
		Sample.DensityRatio = C.SnowCubicMeters / C.SnowVolumeCubicMeters;
	}
	return Sample.RegionId.IsValid();
}

bool FCCLSurfaceSimulation::ApplySnowContact(FGuid RegionId, FGuid SourceId, uint64 Sequence,
	const FVector& Position, double Radius, FString& Error)
{
	const auto* Existing = Regions.Find(RegionId);
	if (!Existing || !SourceId.IsValid() || Sequence == 0 || Position.ContainsNaN()
		|| !FMath::IsFinite(Radius) || Radius < 0.05 || Radius > 1.)
	{
		Error = TEXT("Invalid snow contact identity, position or footprint.");
		return false;
	}
	if (Sequence <= SnowContacts.FindRef(SourceId))
	{
		return true;
	}
	if ((!SnowContacts.Contains(SourceId) && SnowContacts.Num() >= 128) || Existing->Revision == MAX_uint64)
	{
		Error = TEXT("Snow contact receipt or revision budget exceeded.");
		return false;
	}
	FCCLSurfaceGrid G = *Existing;
	const double Area = FMath::Square(G.SpacingMeters);
	TArray<int32> Touched;
	for (int32 I = 0; I < G.Cells.Num(); ++I)
	{
		const FVector Center = G.OriginMeters + FVector((I % G.Size.X + 0.5) * G.SpacingMeters,
			(I / G.Size.X + 0.5) * G.SpacingMeters, 0.);
		if (FVector::DistSquared2D(Center, Position) <= Radius * Radius
			&& FMath::Abs(Position.Z - G.Cells[I].BedMeters) < 1.5)
		{
			Touched.Add(I);
		}
	}
	if (Touched.IsEmpty())
	{
		Error = TEXT("Snow contact is outside its support surface.");
		return false;
	}
	for (int32 I : Touched)
	{
		auto& C = G.Cells[I];
		if (C.SnowCubicMeters <= 0.)
		{
			continue;
		}
		C.SnowVolumeCubicMeters = FMath::Lerp(C.SnowVolumeCubicMeters, C.SnowCubicMeters / 0.6, 0.7);
		const double Ratio = C.SnowVolumeCubicMeters / C.SnowCubicMeters;
		double RemainingMass = C.SnowCubicMeters * 0.12;
		const int32 X = I % G.Size.X, Y = I / G.Size.X;
		for (const FIntPoint Offset : {FIntPoint(-1, 0), FIntPoint(1, 0), FIntPoint(0, -1), FIntPoint(0, 1)})
		{
			const int32 NX = X + Offset.X, NY = Y + Offset.Y, NI = NX + NY * G.Size.X;
			if (NX < 0 || NY < 0 || NX >= G.Size.X || NY >= G.Size.Y || Touched.Contains(NI))
			{
				continue;
			}
			auto& N = G.Cells[NI];
			if (FMath::Min(C.CeilingMeters, N.CeilingMeters) <= FMath::Max(C.BedMeters, N.BedMeters))
			{
				continue;
			}
			const double Room = FMath::Max(0., (N.CeilingMeters - N.BedMeters) * Area
				- N.WaterCubicMeters - N.IceCubicMeters - N.SnowVolumeCubicMeters);
			const double Amount = FMath::Min(RemainingMass, Room / Ratio);
			C.SnowCubicMeters -= Amount;
			C.SnowVolumeCubicMeters -= Amount * Ratio;
			N.SnowCubicMeters += Amount;
			N.SnowVolumeCubicMeters += Amount * Ratio;
			RemainingMass -= Amount;
		}
	}
	++G.Revision;
	if (!ValidateRegion(G, Error))
	{
		return false;
	}
	Regions[RegionId] = MoveTemp(G);
	SnowContacts.Add(SourceId, Sequence);
	return true;
}
