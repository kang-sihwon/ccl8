#include "CCLTerrainStore.h"

namespace
{
	bool Finite(const FVector& V)
	{
		return FMath::IsFinite(V.X) && FMath::IsFinite(V.Y) && FMath::IsFinite(V.Z);
	}

	bool ValidBox(const FBox& Box)
	{
		return Box.IsValid && Finite(Box.Min) && Finite(Box.Max)
			&& Box.Min.X < Box.Max.X && Box.Min.Y < Box.Max.Y && Box.Min.Z < Box.Max.Z;
	}

	bool Within(const FBox& Outer, const FBox& Inner)
	{
		return Outer.IsInsideOrOn(Inner.Min) && Outer.IsInsideOrOn(Inner.Max);
	}

	bool InSamples(const FCCLTerrainDefinition& D, const FIntVector& P)
	{
		return P.X >= D.MinimumCell.X && P.Y >= D.MinimumCell.Y && P.Z >= D.MinimumCell.Z
			&& P.X <= D.MaximumCell.X && P.Y <= D.MaximumCell.Y && P.Z <= D.MaximumCell.Z;
	}

	bool InCells(const FCCLTerrainDefinition& D, const FIntVector& P)
	{
		return InSamples(D, P) && P.X < D.MaximumCell.X && P.Y < D.MaximumCell.Y && P.Z < D.MaximumCell.Z;
	}

	int32 FloorDiv(int32 Value)
	{
		const int32 Quotient = Value / FCCLTerrainStore::SamplesPerAxis;
		return Quotient - (Value % FCCLTerrainStore::SamplesPerAxis < 0 ? 1 : 0);
	}

	bool Lexical(const FIntVector& A, const FIntVector& B)
	{
		return A.X != B.X ? A.X < B.X : A.Y != B.Y ? A.Y < B.Y : A.Z < B.Z;
	}

	FVector Position(const FCCLTerrainDefinition& D, const FIntVector& Sample)
	{
		return D.OriginMeters + FVector(Sample) * D.SampleSpacingMeters;
	}

	FCCLTerrainVoxel Quantize(const FCCLTerrainDefinition& D, double Distance, uint16 Material)
	{
		FCCLTerrainVoxel V;
		V.DistanceMillimeters = int16(FMath::RoundToInt(FMath::Clamp(Distance,
			-4. * D.SampleSpacingMeters, 4. * D.SampleSpacingMeters) * 1000.));
		V.Material = V.DistanceMillimeters > 0 ? 0 : Material;
		return V;
	}

	FCCLTerrainVoxel BaseSample(const FCCLTerrainDefinition& D, const FIntVector& Sample)
	{
		return Quantize(D, Position(D, Sample).Z - D.BaseHeightMeters, D.BaseMaterial);
	}
}

bool FCCLTerrainEdit::operator==(const FCCLTerrainEdit& Other) const
{
	return PrincipalId == Other.PrincipalId && Sequence == Other.Sequence && Epoch == Other.Epoch
		&& ExpectedRevision == Other.ExpectedRevision && Kind == Other.Kind && CenterMeters == Other.CenterMeters
		&& RadiusMeters == Other.RadiusMeters && Material == Other.Material;
}

bool FCCLTerrainSaveContext::operator==(const FCCLTerrainSaveContext& Other) const
{
	return WorldId == Other.WorldId && BaseWorldVersion == Other.BaseWorldVersion
		&& WorldGeneration == Other.WorldGeneration && GameSeconds == Other.GameSeconds && WorldSeconds == Other.WorldSeconds;
}

FIntVector FCCLTerrainStore::OwnerForSample(const FIntVector& Sample)
{
	return FIntVector(FloorDiv(Sample.X), FloorDiv(Sample.Y), FloorDiv(Sample.Z));
}

int32 FCCLTerrainStore::LocalSampleIndex(const FIntVector& Sample)
{
	const FIntVector Local = Sample - OwnerForSample(Sample) * SamplesPerAxis;
	return Local.X + SamplesPerAxis * (Local.Y + SamplesPerAxis * Local.Z);
}

FIntVector FCCLTerrainStore::GlobalSample(const FIntVector& Chunk, int32 LocalIndex)
{
	check(LocalIndex >= 0 && LocalIndex < SamplesPerChunk);
	return Chunk * SamplesPerAxis + FIntVector(LocalIndex % SamplesPerAxis,
		(LocalIndex / SamplesPerAxis) % SamplesPerAxis, LocalIndex / (SamplesPerAxis * SamplesPerAxis));
}

FBox FCCLTerrainStore::EditableBoundsMeters(const FCCLTerrainDefinition& Definition)
{
	return FBox(Position(Definition, Definition.MinimumCell), Position(Definition, Definition.MaximumCell));
}

bool FCCLTerrainStore::ValidateDefinition(const FCCLTerrainDefinition& D, FString& Error)
{
	Error.Reset();
	const FBox Bounds = EditableBoundsMeters(D);
	if (!D.WorldId.IsValid() || !D.RegionId.IsValid() || !D.DefinitionId.IsValid() || D.BaseWorldVersion == 0
		|| D.DefinitionVersion == 0 || D.BodyId.IsNone() || D.BodyId.ToString().Len() > 128
		|| !Finite(D.OriginMeters) || D.OriginMeters.GetAbsMax() > 1.e8
		|| !FMath::IsFinite(D.SampleSpacingMeters) || D.SampleSpacingMeters < 0.05 || D.SampleSpacingMeters > 4.
		|| !FMath::IsFinite(D.BaseHeightMeters) || !ValidBox(Bounds)
		|| D.BaseHeightMeters <= Bounds.Min.Z || D.BaseHeightMeters >= Bounds.Max.Z
		|| D.Materials.Num() < 2 || D.Materials.Num() > 256 || D.Materials[0] != TEXT("Air")
		|| D.BaseMaterial == 0 || D.BaseMaterial >= D.Materials.Num() || D.ProtectedRegions.Num() > 128)
	{
		Error = TEXT("Invalid terrain definition, grid, material palette or protection budget.");
		return false;
	}

	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		if (D.MinimumCell[Axis] < -1000000 || D.MaximumCell[Axis] > 1000000
			|| D.MinimumCell[Axis] >= D.MaximumCell[Axis])
		{
			Error = TEXT("Terrain cell bounds exceed the supported integer domain.");
			return false;
		}
	}

	TSet<FName> Names;
	for (FName Name : D.Materials)
	{
		if (Name.IsNone() || Name.ToString().Len() > 128 || Names.Contains(Name))
		{
			Error = TEXT("Terrain materials require unique bounded names.");
			return false;
		}

		Names.Add(Name);
	}

	TSet<FGuid> ProtectedIds;
	for (const auto& Region : D.ProtectedRegions)
	{
		if (!Region.Id.IsValid() || ProtectedIds.Contains(Region.Id) || !ValidBox(Region.BoundsMeters)
			|| !Within(Bounds, Region.BoundsMeters))
		{
			Error = TEXT("Protected regions require unique IDs and valid in-region bounds.");
			return false;
		}

		ProtectedIds.Add(Region.Id);
	}

	return true;
}

bool FCCLTerrainStore::ValidateSnapshot(const FCCLTerrainSnapshot& S, FString& Error)
{
	if (!ValidateDefinition(S.Definition, Error))
	{
		return false;
	}

	if (S.Revision == 0 || S.Chunks.Num() > MaximumChunks || S.Receipts.Num() > MaximumPrincipals)
	{
		Error = TEXT("Terrain snapshot exceeds a revision or collection bound.");
		return false;
	}

	int64 Total = 0;
	const FIntVector MinOwner = OwnerForSample(S.Definition.MinimumCell);
	const FIntVector MaxOwner = OwnerForSample(S.Definition.MaximumCell);
	const int32 BandMillimeters = FMath::RoundToInt(4000. * S.Definition.SampleSpacingMeters);
	for (const auto& Entry : S.Chunks)
	{
		const auto& Key = Entry.Key;
		const auto& Chunk = Entry.Value;
		if (!Chunk || Chunk->Revision == 0 || Chunk->Revision > S.Revision || Chunk->Overrides.Num() > SamplesPerChunk
			|| Key.X < MinOwner.X || Key.Y < MinOwner.Y || Key.Z < MinOwner.Z
			|| Key.X > MaxOwner.X || Key.Y > MaxOwner.Y || Key.Z > MaxOwner.Z)
		{
			Error = TEXT("Terrain chunk identity or revision is invalid.");
			return false;
		}

		Total += Chunk->Overrides.Num();
		if (Total > MaximumOverrides)
		{
			Error = TEXT("Terrain override budget exceeded.");
			return false;
		}

		for (const auto& Sample : Chunk->Overrides)
		{
			const auto& V = Sample.Value;
			if (Sample.Key < 0 || Sample.Key >= SamplesPerChunk || FMath::Abs(int32(V.DistanceMillimeters)) > BandMillimeters
				|| V.Material >= S.Definition.Materials.Num() || (V.DistanceMillimeters > 0 ? V.Material != 0 : V.Material == 0))
			{
				Error = TEXT("Terrain sample density, material or local index is invalid.");
				return false;
			}

			const FIntVector Global = GlobalSample(Key, Sample.Key);
			if (!InSamples(S.Definition, Global) || V == BaseSample(S.Definition, Global))
			{
				Error = TEXT("Terrain overrides must be in bounds and differ from the base field.");
				return false;
			}
		}
	}

	for (const auto& Entry : S.Receipts)
	{
		const auto& Receipt = Entry.Value;
		const auto& Request = Receipt.Request;
		if (!Entry.Key.IsValid() || Entry.Key != Request.PrincipalId || Request.Sequence == 0
			|| !Request.Epoch.IsValid() || Request.ExpectedRevision == 0 || Request.ExpectedRevision == MAX_uint64
			|| Receipt.CommittedRevision != Request.ExpectedRevision + 1 || Receipt.CommittedRevision > S.Revision
			|| !Finite(Request.CenterMeters) || !FMath::IsFinite(Request.RadiusMeters) || Request.RadiusMeters <= 0.
			|| (Request.Kind != ECCLTerrainEditKind::Excavate && Request.Kind != ECCLTerrainEditKind::Deposit)
			|| Request.Material == 0 || Request.Material >= S.Definition.Materials.Num())
		{
			Error = TEXT("Terrain request receipt is invalid.");
			return false;
		}
	}

	return true;
}

FCCLTerrainVoxel FCCLTerrainStore::ReadSample(const FCCLTerrainSnapshot& Snapshot, const FIntVector& Sample)
{
	if (const auto* Chunk = Snapshot.Chunks.Find(OwnerForSample(Sample)); Chunk && *Chunk)
	{
		if (const auto* Value = (*Chunk)->Overrides.Find(LocalSampleIndex(Sample)))
		{
			return *Value;
		}
	}

	return BaseSample(Snapshot.Definition, Sample);
}

bool FCCLTerrainStore::ReadDensityMeters(const FCCLTerrainSnapshot& Snapshot, const FVector& PositionMeters,
	double& OutDensity, FString& Error)
{
	Error.Reset();
	const auto& D = Snapshot.Definition;
	if (!Finite(PositionMeters) || !FMath::IsFinite(D.SampleSpacingMeters) || D.SampleSpacingMeters <= 0.)
	{
		Error = TEXT("Invalid density sample position or grid spacing.");
		return false;
	}

	const FVector Grid = (PositionMeters - D.OriginMeters) / D.SampleSpacingMeters;
	if (!Finite(Grid) || Grid.GetAbsMax() > 1000001.)
	{
		Error = TEXT("Density sample exceeds the supported grid domain.");
		return false;
	}

	const FIntVector Low(FMath::FloorToInt(Grid.X), FMath::FloorToInt(Grid.Y), FMath::FloorToInt(Grid.Z));
	const FVector Alpha = Grid - FVector(Low);
	double Density = 0.;
	for (int32 Z = 0; Z < 2; ++Z)
	{
		for (int32 Y = 0; Y < 2; ++Y)
		{
			for (int32 X = 0; X < 2; ++X)
			{
				Density += ReadSample(Snapshot, Low + FIntVector(X, Y, Z)).DistanceMillimeters * 0.001
					* (X ? Alpha.X : 1. - Alpha.X) * (Y ? Alpha.Y : 1. - Alpha.Y) * (Z ? Alpha.Z : 1. - Alpha.Z);
			}
		}
	}

	OutDensity = Density;
	return true;
}

bool FCCLTerrainStore::Initialize(const FCCLTerrainDefinition& Definition, FString& Error)
{
	if (!ValidateDefinition(Definition, Error))
	{
		return false;
	}

	auto State = MakeShared<FCCLTerrainSnapshot, ESPMode::ThreadSafe>();
	State->Definition = Definition;
	Committed = State;
	Epoch = FGuid::NewGuid();
	return true;
}

bool FCCLTerrainStore::ValidateRequest(const FCCLTerrainEdit& R, const FCCLTerrainAuthority& A,
	FBox& OutAffectedBounds, FString& Error) const
{
	if (!Committed || !A.PrincipalId.IsValid() || R.PrincipalId != A.PrincipalId || A.PolicyRevision == 0
		|| !ValidBox(A.AllowedBoundsMeters) || !FMath::IsFinite(A.MaximumRadiusMeters) || A.MaximumRadiusMeters <= 0.
		|| !Finite(R.CenterMeters) || !FMath::IsFinite(R.RadiusMeters) || R.RadiusMeters < Committed->Definition.SampleSpacingMeters
		|| R.RadiusMeters > A.MaximumRadiusMeters || R.Material == 0 || R.Material >= Committed->Definition.Materials.Num()
		|| (R.Kind != ECCLTerrainEditKind::Excavate && R.Kind != ECCLTerrainEditKind::Deposit)
		|| (R.Kind == ECCLTerrainEditKind::Excavate ? !A.bCanExcavate : !A.bCanDeposit))
	{
		Error = TEXT("Terrain edit denied by server authority, shape or material constraints.");
		return false;
	}

	const double Extent = R.RadiusMeters + 5. * Committed->Definition.SampleSpacingMeters;
	const FBox Affected(R.CenterMeters - FVector(Extent), R.CenterMeters + FVector(Extent));
	if (!ValidBox(Affected) || !Within(EditableBoundsMeters(Committed->Definition), Affected) || !Within(A.AllowedBoundsMeters, Affected))
	{
		Error = TEXT("Terrain brush and interpolation margin exceed allowed bounds.");
		return false;
	}

	for (const auto& Protected : Committed->Definition.ProtectedRegions)
	{
		if (Affected.Intersect(Protected.BoundsMeters))
		{
			Error = TEXT("Terrain edit would affect a protected region.");
			return false;
		}
	}

	OutAffectedBounds = Affected;
	return true;
}

ECCLTerrainPrepareResult FCCLTerrainStore::PrepareEdit(const FCCLTerrainEdit& Request, const FCCLTerrainAuthority& Authority,
	FCCLTerrainCandidate& OutCandidate, FString& Error) const
{
	Error.Reset();
	FBox Affected;
	if (!ValidateRequest(Request, Authority, Affected, Error))
	{
		return ECCLTerrainPrepareResult::Failed;
	}

	const auto* Receipt = Committed->Receipts.Find(Request.PrincipalId);
	if (Receipt && Request.Sequence == Receipt->Request.Sequence && Request == Receipt->Request)
	{
		return ECCLTerrainPrepareResult::AlreadyApplied;
	}

	const uint64 LastSequence = Receipt ? Receipt->Request.Sequence : 0;
	if (LastSequence == MAX_uint64 || Request.Sequence != LastSequence + 1 || Request.Epoch != Epoch
		|| Request.ExpectedRevision != Committed->Revision || Committed->Revision == MAX_uint64
		|| (!Receipt && Committed->Receipts.Num() >= MaximumPrincipals))
	{
		Error = TEXT("Stale terrain request, nonsequential request ID or exhausted request budget.");
		return ECCLTerrainPrepareResult::Failed;
	}

	const auto& D = Committed->Definition;
	const double RadiusWithBand = Request.RadiusMeters + 4. * D.SampleSpacingMeters;
	const FVector LowGrid = (Request.CenterMeters - FVector(RadiusWithBand) - D.OriginMeters) / D.SampleSpacingMeters;
	const FVector HighGrid = (Request.CenterMeters + FVector(RadiusWithBand) - D.OriginMeters) / D.SampleSpacingMeters;
	const FIntVector Low(FMath::FloorToInt(LowGrid.X), FMath::FloorToInt(LowGrid.Y), FMath::FloorToInt(LowGrid.Z));
	const FIntVector High(FMath::CeilToInt(HighGrid.X), FMath::CeilToInt(HighGrid.Y), FMath::CeilToInt(HighGrid.Z));
	int64 SampleCount = 1;
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		const int64 AxisCount = int64(High[Axis]) - Low[Axis] + 1;
		if (AxisCount <= 0 || AxisCount > MaximumBrushSamples || SampleCount > MaximumBrushSamples / AxisCount)
		{
			Error = TEXT("Terrain brush sample budget exceeded before allocation.");
			return ECCLTerrainPrepareResult::Failed;
		}

		SampleCount *= AxisCount;
	}

	auto Next = MakeShared<FCCLTerrainSnapshot, ESPMode::ThreadSafe>(*Committed);
	++Next->Revision;
	TMap<FIntVector, TSharedPtr<FCCLTerrainChunkData, ESPMode::ThreadSafe>> MutableChunks;
	auto MutableChunk = [&](FIntVector Key)
	{
		if (const auto* Existing = MutableChunks.Find(Key))
		{
			return *Existing;
		}

		auto Chunk = MakeShared<FCCLTerrainChunkData, ESPMode::ThreadSafe>();
		if (const auto* Previous = Committed->Chunks.Find(Key))
		{
			*Chunk = **Previous;
		}

		Chunk->Revision = Next->Revision;
		MutableChunks.Add(Key, Chunk);
		Next->Chunks.Add(Key, Chunk);
		return TSharedPtr<FCCLTerrainChunkData, ESPMode::ThreadSafe>(Chunk);
	};

	TSet<FIntVector> MeshChunks;
	for (int32 Z = Low.Z; Z <= High.Z; ++Z)
	{
		for (int32 Y = Low.Y; Y <= High.Y; ++Y)
		{
			for (int32 X = Low.X; X <= High.X; ++X)
			{
				const FIntVector Sample(X, Y, Z);
				const auto Before = ReadSample(*Committed, Sample);
				const double Sphere = (Position(D, Sample) - Request.CenterMeters).Length() - Request.RadiusMeters;
				const double Prior = Before.DistanceMillimeters * 0.001;
				const double Distance = Request.Kind == ECCLTerrainEditKind::Excavate ? FMath::Max(Prior, -Sphere) : FMath::Min(Prior, Sphere);
				const uint16 Material = Request.Kind == ECCLTerrainEditKind::Deposit && Distance < Prior ? Request.Material : Before.Material;
				const auto After = Quantize(D, Distance, Material ? Material : Request.Material);
				if (After == Before)
				{
					continue;
				}

				auto Chunk = MutableChunk(OwnerForSample(Sample));
				const int32 Index = LocalSampleIndex(Sample);
				if (After == BaseSample(D, Sample))
				{
					Chunk->Overrides.Remove(Index);
				}
				else
				{
					Chunk->Overrides.Add(Index, After);
				}

				for (int32 DZ = -1; DZ <= 0; ++DZ)
				{
					for (int32 DY = -1; DY <= 0; ++DY)
					{
						for (int32 DX = -1; DX <= 0; ++DX)
						{
							const FIntVector Cell = Sample + FIntVector(DX, DY, DZ);
							if (InCells(D, Cell))
							{
								MeshChunks.Add(OwnerForSample(Cell));
							}
						}
					}
				}
			}
		}
	}

	if (MutableChunks.IsEmpty())
	{
		Error = TEXT("Terrain brush does not change any sample; sequence remains unconsumed.");
		return ECCLTerrainPrepareResult::Failed;
	}

	for (const FIntVector& Key : MeshChunks)
	{
		MutableChunk(Key);
	}

	FCCLTerrainReceipt NewReceipt;
	NewReceipt.Request = Request;
	NewReceipt.CommittedRevision = Next->Revision;
	Next->Receipts.Add(Request.PrincipalId, NewReceipt);
	if (!ValidateSnapshot(*Next, Error))
	{
		return ECCLTerrainPrepareResult::Failed;
	}

	FCCLTerrainCandidate Candidate;
	Candidate.Before = Committed;
	Candidate.After = Next;
	Candidate.MeshChunks = MeshChunks.Array();
	Candidate.MeshChunks.Sort(Lexical);
	Candidate.AffectedBoundsMeters = Affected;
	Candidate.Request = Request;
	Candidate.PolicyRevision = Authority.PolicyRevision;
	Candidate.Ticket = FGuid::NewGuid();
	OutCandidate = MoveTemp(Candidate);
	return ECCLTerrainPrepareResult::Prepared;
}

bool FCCLTerrainStore::CommitEdit(const FCCLTerrainCandidate& Candidate, const FCCLTerrainAuthority& Authority, FString& Error)
{
	Error.Reset();
	FBox Affected;
	if (!Candidate.IsValid() || Candidate.Before != Committed || Candidate.Request.Epoch != Epoch
		|| Candidate.PolicyRevision != Authority.PolicyRevision)
	{
		Error = TEXT("Terrain candidate or authority changed while its dependent data was prepared.");
		return false;
	}

	if (!ValidateRequest(Candidate.Request, Authority, Affected, Error))
	{
		return false;
	}

	Committed = Candidate.After;
	return true;
}

bool FCCLTerrainStore::Capture(const FCCLTerrainSaveContext& Context, TArray<uint8>& OutBytes, FString& Error) const
{
	if (!Committed)
	{
		Error = TEXT("Terrain store is not initialized.");
		return false;
	}

	return FCCLTerrainCodec::Encode(*Committed, Context, OutBytes, Error);
}

bool FCCLTerrainStore::Restore(const TArray<uint8>& Bytes, const FCCLTerrainSaveContext& ExpectedContext, FString& Error)
{
	Error.Reset();
	if (!Committed)
	{
		Error = TEXT("Terrain store is not initialized.");
		return false;
	}

	FCCLTerrainSnapshot Snapshot;
	FCCLTerrainSaveContext Context;
	if (!FCCLTerrainCodec::Decode(Bytes, Snapshot, Context, Error))
	{
		return false;
	}

	if (!(Context == ExpectedContext) || !FCCLTerrainCodec::SameDefinition(Snapshot.Definition, Committed->Definition))
	{
		Error = TEXT("Terrain record does not match the world save generation or base region definition.");
		return false;
	}

	Committed = MakeShared<FCCLTerrainSnapshot, ESPMode::ThreadSafe>(MoveTemp(Snapshot));
	Epoch = FGuid::NewGuid();
	return true;
}
