#include "CCLTerrainSurface.h"

FCCLTerrainSurface::FCCLTerrainSurface(const FCCLTerrainDefinition& InDefinition, uint64 InRevision, FGuid InEpoch,
	const FTransform& InTransform, TArray<TSharedRef<const FCCLTerrainMesh>> InMeshes)
	: Definition(InDefinition), Revision(InRevision), Epoch(InEpoch), Transform(InTransform), Meshes(MoveTemp(InMeshes))
{
}

bool FCCLTerrainSurface::QuerySurfaces(const FCCLSurfaceQuery& Query, TArray<FCCLSurfaceSample>& OutSamples, FString& Error) const
{
	if (!Revision || !Epoch.IsValid() || Query.BodyId.IsNone() || Query.OriginMeters.ContainsNaN()
		|| Query.OriginMeters.GetAbsMax() > 1.e9 || Query.Direction.ContainsNaN()
		|| FMath::Abs(Query.Direction.SizeSquared() - 1.) > 1.e-6
		|| !FMath::IsFinite(Query.MinimumDistanceMeters) || Query.MinimumDistanceMeters < 0.
		|| !FMath::IsFinite(Query.MaximumDistanceMeters) || Query.MaximumDistanceMeters <= Query.MinimumDistanceMeters
		|| Query.MaximumDistanceMeters > 1.e7 || !Query.KindMask || (Query.KindMask & ~15)
		|| Query.MaximumResults < 1 || Query.MaximumResults > 4096
		|| (Query.RequiredRevision && Query.RequiredRevision != Revision)
		|| (Query.RequiredEpoch.IsValid() && Query.RequiredEpoch != Epoch))
	{
		Error = TEXT("Terrain surface query is invalid or stale.");
		return false;
	}

	TArray<FCCLSurfaceSample> Hits;
	if (Query.BodyId == Definition.BodyId && (Query.KindMask & uint8(ECCLSurfaceKind::Terrain)))
	{
		const FVector Origin = Transform.InverseTransformPosition(Query.OriginMeters * 100.) * 0.01;
		const FVector Direction = Transform.InverseTransformVectorNoScale(Query.Direction);
		for (const auto& Mesh : Meshes)
		{
			for (int32 Index = 0; Index < Mesh->Triangles.Num(); ++Index)
			{
				const auto T = Mesh->Triangles[Index];
				const FVector A = Mesh->OriginMeters + Mesh->VerticesMeters[T.X];
				const FVector E1 = Mesh->VerticesMeters[T.Y] - Mesh->VerticesMeters[T.X];
				const FVector E2 = Mesh->VerticesMeters[T.Z] - Mesh->VerticesMeters[T.X];
				const FVector P = FVector::CrossProduct(Direction, E2);
				const double Det = FVector::DotProduct(E1, P);
				if (FMath::Abs(Det) < 1.e-12)
				{
					continue;
				}

				const FVector Offset = Origin - A;
				const double U = FVector::DotProduct(Offset, P) / Det;
				const FVector Q = FVector::CrossProduct(Offset, E1);
				const double V = FVector::DotProduct(Direction, Q) / Det;
				const double Distance = FVector::DotProduct(E2, Q) / Det;
				if (U < -1.e-9 || V < -1.e-9 || U + V > 1. + 1.e-9
					|| Distance < Query.MinimumDistanceMeters || Distance > Query.MaximumDistanceMeters)
				{
					continue;
				}

				// Unreal clockwise winding: geometric outward normal is the reversed cross product.
				const FVector LocalNormal = FVector::CrossProduct(E2, E1).GetSafeNormal();
				const FVector Abs = LocalNormal.GetAbs();
				const int32 Axis = Abs.X >= Abs.Y && Abs.X >= Abs.Z ? 0 : (Abs.Y >= Abs.Z ? 1 : 2);
				const int32 Face = Axis * 2 + (LocalNormal[Axis] < 0. ? 1 : 0);
				const FIntVector Cell = Mesh->TriangleCells[Index];
				FCCLSurfaceSample Hit;
				Hit.SurfaceId = FGuid::NewDeterministicGuid(FString::Printf(TEXT("Terrain/%s/%s/%d/%d/%d/%d"),
					*Definition.WorldId.ToString(), *Definition.RegionId.ToString(), Cell.X, Cell.Y, Cell.Z, Face));
				Hit.BodyId = Definition.BodyId;
				Hit.Revision = Revision;
				Hit.Epoch = Epoch;
				Hit.SourceRevision = Revision;
				Hit.SourceEpoch = Epoch;
				Hit.PositionMeters = Transform.TransformPosition((Origin + Direction * Distance) * 100.) * 0.01;
				Hit.Normal = Transform.TransformVectorNoScale(LocalNormal).GetSafeNormal();
				Hit.DistanceMeters = Distance;
				Hit.MaterialId = Definition.Materials[Mesh->TriangleMaterials[Index]];
				Hit.Kind = ECCLSurfaceKind::Terrain;
				Hits.Add(Hit);
			}
		}
	}

	Hits.Sort([](const FCCLSurfaceSample& A, const FCCLSurfaceSample& B)
	{
		return A.DistanceMeters == B.DistanceMeters ? A.SurfaceId < B.SurfaceId : A.DistanceMeters < B.DistanceMeters;
	});
	TArray<FCCLSurfaceSample> Unique;
	for (const auto& Hit : Hits)
	{
		// Adjacent triangles may both contain an edge/vertex hit. Preserve distinct coincident layers.
		if (!Unique.IsEmpty() && FMath::Abs(Unique.Last().DistanceMeters - Hit.DistanceMeters) <= 1.e-7
			&& FVector::DotProduct(Unique.Last().Normal, Hit.Normal) > 0.999999)
		{
			continue;
		}

		if (Unique.Num() >= Query.MaximumResults)
		{
			Error = TEXT("Terrain surface result budget exceeded; no partial result published.");
			return false;
		}

		Unique.Add(Hit);
	}

	OutSamples = MoveTemp(Unique);
	Error.Reset();
	return true;
}
