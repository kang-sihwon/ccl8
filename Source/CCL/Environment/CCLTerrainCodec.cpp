#include "CCLTerrainStore.h"

#include "Misc/Crc.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace
{
	constexpr uint32 Magic = 0x334C4343;
	constexpr uint32 Schema = 1;

	bool Count(FArchive& Ar, int32& Value, int32 Maximum)
	{
		Ar << Value;
		return !Ar.IsError() && Value >= 0 && Value <= Maximum;
	}

	bool Name(FArchive& Ar, FName& Value)
	{
		FString Text = Ar.IsSaving() ? Value.ToString() : FString();
		int32 Length = Text.Len();
		if (!Count(Ar, Length, 128) || Length == 0)
		{
			return false;
		}

		for (int32 Index = 0; Index < Length; ++Index)
		{
			uint16 Unit = Ar.IsSaving() ? uint16(Text[Index]) : 0;
			Ar << Unit;
			if (Ar.IsError() || Unit == 0)
			{
				return false;
			}

			if (Ar.IsLoading())
			{
				Text.AppendChar(TCHAR(Unit));
			}
		}

		if (Ar.IsLoading())
		{
			Value = FName(*Text);
		}

		return true;
	}

	void Vector(FArchive& Ar, FVector& Value)
	{
		Ar << Value.X << Value.Y << Value.Z;
	}

	void Grid(FArchive& Ar, FIntVector& Value)
	{
		Ar << Value.X << Value.Y << Value.Z;
	}

	void Box(FArchive& Ar, FBox& Value)
	{
		Vector(Ar, Value.Min);
		Vector(Ar, Value.Max);
		if (Ar.IsLoading())
		{
			Value.IsValid = 1;
		}
	}

	bool GuidLess(const FGuid& A, const FGuid& B)
	{
		return A.A != B.A ? A.A < B.A : A.B != B.B ? A.B < B.B : A.C != B.C ? A.C < B.C : A.D < B.D;
	}

	bool Definition(FArchive& Ar, FCCLTerrainDefinition& D)
	{
		Ar << D.WorldId << D.RegionId << D.DefinitionId << D.BaseWorldVersion << D.DefinitionVersion;
		if (!Name(Ar, D.BodyId))
		{
			return false;
		}

		Vector(Ar, D.OriginMeters);
		Ar << D.SampleSpacingMeters << D.BaseHeightMeters;
		Grid(Ar, D.MinimumCell);
		Grid(Ar, D.MaximumCell);
		Ar << D.BaseMaterial;
		int32 Materials = D.Materials.Num();
		if (!Count(Ar, Materials, 256))
		{
			return false;
		}

		if (Ar.IsLoading())
		{
			D.Materials.SetNum(Materials);
		}

		for (FName& Material : D.Materials)
		{
			if (!Name(Ar, Material))
			{
				return false;
			}
		}

		int32 Protected = D.ProtectedRegions.Num();
		if (!Count(Ar, Protected, 128))
		{
			return false;
		}

		if (Ar.IsLoading())
		{
			D.ProtectedRegions.SetNum(Protected);
		}
		else
		{
			D.ProtectedRegions.Sort([](const auto& A, const auto& B) { return GuidLess(A.Id, B.Id); });
		}

		for (auto& Region : D.ProtectedRegions)
		{
			Ar << Region.Id;
			Box(Ar, Region.BoundsMeters);
		}

		return !Ar.IsError();
	}

	void Context(FArchive& Ar, FCCLTerrainSaveContext& C)
	{
		Ar << C.WorldId << C.BaseWorldVersion << C.WorldGeneration << C.GameSeconds << C.WorldSeconds;
	}

	bool ValidContext(const FCCLTerrainSnapshot& S, const FCCLTerrainSaveContext& C)
	{
		return C.WorldId == S.Definition.WorldId && C.BaseWorldVersion == S.Definition.BaseWorldVersion
			&& C.WorldGeneration > 0 && FMath::IsFinite(C.GameSeconds) && C.GameSeconds >= 0. && C.GameSeconds <= 1.e12
			&& FMath::IsFinite(C.WorldSeconds) && C.WorldSeconds >= 0. && C.WorldSeconds <= 1.e12;
	}

	void Request(FArchive& Ar, FCCLTerrainEdit& R)
	{
		Ar << R.PrincipalId << R.Sequence << R.Epoch << R.ExpectedRevision;
		uint8 Kind = uint8(R.Kind);
		Ar << Kind;
		R.Kind = ECCLTerrainEditKind(Kind);
		Vector(Ar, R.CenterMeters);
		Ar << R.RadiusMeters << R.Material;
	}

	bool Snapshot(FArchive& Ar, FCCLTerrainSnapshot& S)
	{
		if (!Definition(Ar, S.Definition))
		{
			return false;
		}

		Ar << S.Revision;
		int32 Chunks = S.Chunks.Num();
		if (!Count(Ar, Chunks, FCCLTerrainStore::MaximumChunks))
		{
			return false;
		}

		TArray<FIntVector> Keys;
		if (Ar.IsSaving())
		{
			S.Chunks.GetKeys(Keys);
			Keys.Sort([](const auto& A, const auto& B)
			{
				return A.X != B.X ? A.X < B.X : A.Y != B.Y ? A.Y < B.Y : A.Z < B.Z;
			});
		}

		int64 TotalSamples = 0;
		for (int32 Index = 0; Index < Chunks; ++Index)
		{
			FIntVector Key = Ar.IsSaving() ? Keys[Index] : FIntVector::ZeroValue;
			Grid(Ar, Key);
			TSharedPtr<FCCLTerrainChunkData, ESPMode::ThreadSafe> Loaded;
			if (Ar.IsLoading())
			{
				Loaded = MakeShared<FCCLTerrainChunkData, ESPMode::ThreadSafe>();
			}
			const auto* Source = Ar.IsSaving() ? S.Chunks[Key].Get() : Loaded.Get();
			uint64 Revision = Source->Revision;
			Ar << Revision;
			int32 Samples = Source->Overrides.Num();
			if (!Count(Ar, Samples, FCCLTerrainStore::SamplesPerChunk)
				|| (TotalSamples += Samples) > FCCLTerrainStore::MaximumOverrides
				|| (Ar.IsLoading() && S.Chunks.Contains(Key)))
			{
				return false;
			}

			TArray<int32> LocalKeys;
			if (Ar.IsSaving())
			{
				Source->Overrides.GetKeys(LocalKeys);
				LocalKeys.Sort();
			}

			for (int32 SampleIndex = 0; SampleIndex < Samples; ++SampleIndex)
			{
				int32 Local = Ar.IsSaving() ? LocalKeys[SampleIndex] : 0;
				FCCLTerrainVoxel V = Ar.IsSaving() ? Source->Overrides[Local] : FCCLTerrainVoxel();
				Ar << Local << V.DistanceMillimeters << V.Material;
				if (Ar.IsError() || Local < 0 || Local >= FCCLTerrainStore::SamplesPerChunk
					|| (Ar.IsLoading() && Loaded->Overrides.Contains(Local)))
				{
					return false;
				}

				if (Ar.IsLoading())
				{
					Loaded->Overrides.Add(Local, V);
				}
			}

			if (Ar.IsLoading())
			{
				Loaded->Revision = Revision;
				S.Chunks.Add(Key, Loaded);
			}
		}

		int32 Receipts = S.Receipts.Num();
		if (!Count(Ar, Receipts, FCCLTerrainStore::MaximumPrincipals))
		{
			return false;
		}

		TArray<FGuid> Principals;
		if (Ar.IsSaving())
		{
			S.Receipts.GetKeys(Principals);
			Principals.Sort(GuidLess);
		}

		for (int32 Index = 0; Index < Receipts; ++Index)
		{
			FGuid Key = Ar.IsSaving() ? Principals[Index] : FGuid();
			FCCLTerrainReceipt Receipt = Ar.IsSaving() ? S.Receipts[Key] : FCCLTerrainReceipt();
			Ar << Key;
			Request(Ar, Receipt.Request);
			Ar << Receipt.CommittedRevision;
			if (Ar.IsError() || (Ar.IsLoading() && S.Receipts.Contains(Key)))
			{
				return false;
			}

			if (Ar.IsLoading())
			{
				S.Receipts.Add(Key, Receipt);
			}
		}

		return !Ar.IsError();
	}
}

bool FCCLTerrainCodec::Encode(const FCCLTerrainSnapshot& Value, const FCCLTerrainSaveContext& SaveContext,
	TArray<uint8>& OutBytes, FString& Error)
{
	Error.Reset();
	if (!FCCLTerrainStore::ValidateSnapshot(Value, Error))
	{
		return false;
	}

	if (!ValidContext(Value, SaveContext))
	{
		Error = TEXT("Terrain save context must match its world, base version and completed generation.");
		return false;
	}

	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes);
	uint32 SavedMagic = Magic;
	uint32 SavedSchema = Schema;
	Writer << SavedMagic << SavedSchema;
	auto ContextCopy = SaveContext;
	Context(Writer, ContextCopy);
	auto Copy = Value;
	if (!Snapshot(Writer, Copy) || Bytes.Num() > MaximumBytes - 4)
	{
		Error = TEXT("Terrain region encoding failed or exceeded its independent size limit.");
		return false;
	}

	uint32 CRC = FCrc::MemCrc32(Bytes.GetData(), Bytes.Num());
	Writer << CRC;
	OutBytes = MoveTemp(Bytes);
	return true;
}

bool FCCLTerrainCodec::Decode(const TArray<uint8>& Bytes, FCCLTerrainSnapshot& OutSnapshot,
	FCCLTerrainSaveContext& OutContext, FString& Error)
{
	Error.Reset();
	if (Bytes.Num() < 64 || Bytes.Num() > MaximumBytes)
	{
		Error = TEXT("Terrain region record is empty, truncated or too large.");
		return false;
	}

	uint32 SavedCRC = 0;
	FMemory::Memcpy(&SavedCRC, Bytes.GetData() + Bytes.Num() - 4, 4);
	if (FCrc::MemCrc32(Bytes.GetData(), Bytes.Num() - 4) != SavedCRC)
	{
		Error = TEXT("Terrain region checksum mismatch.");
		return false;
	}

	FMemoryReader Reader(Bytes);
	uint32 SavedMagic = 0;
	uint32 SavedSchema = 0;
	Reader << SavedMagic << SavedSchema;
	FCCLTerrainSnapshot Candidate;
	FCCLTerrainSaveContext CandidateContext;
	Context(Reader, CandidateContext);
	if (SavedMagic != Magic || SavedSchema != Schema || !Snapshot(Reader, Candidate)
		|| Reader.IsError() || Reader.Tell() != Bytes.Num() - 4)
	{
		Error = TEXT("Terrain region schema, collection bounds, duplicate keys or record length is invalid.");
		return false;
	}

	if (!FCCLTerrainStore::ValidateSnapshot(Candidate, Error) || !ValidContext(Candidate, CandidateContext))
	{
		if (Error.IsEmpty())
		{
			Error = TEXT("Terrain region context does not match its payload.");
		}

		return false;
	}

	OutSnapshot = MoveTemp(Candidate);
	OutContext = CandidateContext;
	return true;
}

bool FCCLTerrainCodec::SameDefinition(const FCCLTerrainDefinition& A, const FCCLTerrainDefinition& B)
{
	FString Error;
	if (!FCCLTerrainStore::ValidateDefinition(A, Error) || !FCCLTerrainStore::ValidateDefinition(B, Error))
	{
		return false;
	}

	TArray<uint8> Left;
	TArray<uint8> Right;
	FMemoryWriter LeftWriter(Left);
	FMemoryWriter RightWriter(Right);
	auto LeftDefinition = A;
	auto RightDefinition = B;
	return Definition(LeftWriter, LeftDefinition) && Definition(RightWriter, RightDefinition) && Left == Right;
}
