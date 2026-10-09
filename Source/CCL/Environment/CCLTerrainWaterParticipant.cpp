#include "CCLTerrainWaterParticipant.h"

#include "CCLWorldSimulationSubsystem.h"

FCCLTerrainWaterParticipant::FCCLTerrainWaterParticipant(UCCLWorldSimulationSubsystem* InOwner, FGuid InRegionId, const FTransform& InTransform)
	: Owner(InOwner), RegionId(InRegionId), Transform(InTransform)
{
}

bool FCCLTerrainWaterParticipant::ReadBeds(const FCCLTerrainSnapshot& Terrain, const FTransform& Transform,
	const FCCLSurfaceGrid& Grid, TArray<double>& OutBeds, FString& Error)
{
	if (!Transform.GetRotation().Equals(FQuat::Identity) || !Transform.GetScale3D().Equals(FVector::OneVector))
	{
		Error = TEXT("Surface water requires an unrotated, unit-scale terrain region.");
		return false;
	}
	const FBox Bounds = FCCLTerrainStore::EditableBoundsMeters(Terrain.Definition);
	const double Interval = Terrain.Definition.SampleSpacingMeters * 0.5;
	TArray<double> Candidate;
	Candidate.Reserve(Grid.Cells.Num());
	for (int32 Y = 0; Y < Grid.Size.Y; ++Y)
	{
		for (int32 X = 0; X < Grid.Size.X; ++X)
		{
			FVector P = Grid.OriginMeters + FVector((X + 0.5) * Grid.SpacingMeters, (Y + 0.5) * Grid.SpacingMeters, 0.);
			P = Transform.InverseTransformPosition(P * 100.) / 100.;
			if (P.X < Bounds.Min.X || P.Y < Bounds.Min.Y || P.X > Bounds.Max.X || P.Y > Bounds.Max.Y)
			{
				Error = TEXT("Surface water grid extends outside its terrain density domain.");
				return false;
			}
			double Previous = 0.;
			P.Z = Bounds.Max.Z;
			if (!FCCLTerrainStore::ReadDensityMeters(Terrain, P, Previous, Error))
			{
				return false;
			}
			double Height = Previous <= 0. ? Bounds.Max.Z : Bounds.Min.Z;
			for (double Z = Bounds.Max.Z - Interval; Previous > 0. && Z >= Bounds.Min.Z; Z -= Interval)
			{
				P.Z = Z;
				double Density;
				if (!FCCLTerrainStore::ReadDensityMeters(Terrain, P, Density, Error))
				{
					return false;
				}
				if (Density <= 0.)
				{
					Height = Z + Interval * (-Density / (Previous - Density));
					break;
				}
				Previous = Density;
			}
			Candidate.Add(Height + Transform.GetLocation().Z / 100.);
		}
	}
	OutBeds = MoveTemp(Candidate);
	return true;
}

bool FCCLTerrainWaterParticipant::Prepare(const FCCLTerrainCandidate& Candidate, FString& Error)
{
	const auto* Runtime = Owner.Get();
	const auto* Grid = Runtime ? Runtime->GetSurfaceSimulation().FindRegion(RegionId) : nullptr;
	if (!Grid || Grid->TerrainId != Candidate.GetSnapshot().Definition.RegionId
		|| !ReadBeds(Candidate.GetSnapshot(), Transform, *Grid, Beds, Error))
	{
		return false;
	}
	Ticket = Candidate.GetTicket();
	return true;
}

bool FCCLTerrainWaterParticipant::ValidateCommit(const FCCLTerrainCandidate& Candidate, FString& Error) const
{
	const auto* Runtime = Owner.Get();
	if (!Runtime || Ticket != Candidate.GetTicket())
	{
		Error = TEXT("Terrain water owner or preparation ticket changed.");
		return false;
	}
	Prepared = Runtime->GetSurfaceSimulation();
	return Prepared.RebaseTerrain(RegionId, Beds, Candidate.GetSnapshot().Revision, Error);
}

void FCCLTerrainWaterParticipant::Commit(const FCCLTerrainCandidate& Candidate)
{
	check(Owner.IsValid() && Ticket == Candidate.GetTicket());
	Owner->SurfaceSimulation = MoveTemp(Prepared);
	Ticket.Invalidate();
	Beds.Reset();
}

void FCCLTerrainWaterParticipant::Abort(const FCCLTerrainCandidate& Candidate)
{
	Ticket.Invalidate();
	Beds.Reset();
	Prepared = FCCLSurfaceSimulation();
}

bool FCCLTerrainWaterParticipant::ValidateRestore(const FCCLTerrainSnapshot& Terrain, const FCCLWorldSnapshot* World, FString& Error) const
{
	const auto* Runtime = Owner.Get();
	if (!Runtime)
	{
		Error = TEXT("Terrain water owner was destroyed.");
		return false;
	}
	bRestoreWorld = World != nullptr;
	if (World)
	{
		if (!Prepared.Restore(World->Surface, Error))
		{
			return false;
		}
		if (World->bSurfaceMigrated && Prepared.GetRegions().IsEmpty())
		{
			return true;
		}
	}
	else
	{
		Prepared = Runtime->GetSurfaceSimulation();
	}
	const auto* Grid = Prepared.FindRegion(RegionId);
	TArray<double> RestoreBeds;
	if (!Grid || Grid->TerrainId != Terrain.Definition.RegionId || Prepared.GetWorldId() != Terrain.Definition.WorldId
		|| !ReadBeds(Terrain, Transform, *Grid, RestoreBeds, Error))
	{
		Error = TEXT("Restored terrain and surface water identity or geometry do not match.");
		return false;
	}
	if (!World)
	{
		return Prepared.RebaseTerrain(RegionId, RestoreBeds, Terrain.Revision, Error, true);
	}
	if (Grid->TerrainRevision != Terrain.Revision)
	{
		Error = TEXT("Restored water references a different terrain revision.");
		return false;
	}
	for (int32 I = 0; I < RestoreBeds.Num(); ++I)
	{
		if (!FMath::IsNearlyEqual(RestoreBeds[I], Grid->Cells[I].BedMeters, 1.e-6))
		{
			Error = TEXT("Saved water bed does not agree with saved terrain density.");
			return false;
		}
	}
	return true;
}

void FCCLTerrainWaterParticipant::CommitRestore()
{
	if (!bRestoreWorld)
	{
		check(Owner.IsValid());
		Owner->SurfaceSimulation = MoveTemp(Prepared);
	}
	Prepared = FCCLSurfaceSimulation();
}
