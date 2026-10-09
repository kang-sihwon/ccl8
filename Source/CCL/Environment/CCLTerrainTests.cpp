#include "CCLTerrainStore.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/Crc.h"
#include "Serialization/MemoryWriter.h"

namespace
{
	FCCLTerrainDefinition Definition()
	{
		FCCLTerrainDefinition D;
		D.WorldId = FGuid(1, 2, 3, 4);
		D.RegionId = FGuid(5, 6, 7, 8);
		D.DefinitionId = FGuid(9, 10, 11, 12);
		return D;
	}

	FCCLTerrainAuthority Authority(const FCCLTerrainDefinition& D, uint32 Id = 1)
	{
		FCCLTerrainAuthority A;
		A.PrincipalId = FGuid(100, 200, 300, Id);
		A.AllowedBoundsMeters = FCCLTerrainStore::EditableBoundsMeters(D);
		A.bCanExcavate = 1;
		A.bCanDeposit = 1;
		return A;
	}

	FCCLTerrainEdit Edit(const FCCLTerrainStore& Store, const FCCLTerrainAuthority& A, uint64 Sequence = 1)
	{
		FCCLTerrainEdit R;
		R.PrincipalId = A.PrincipalId;
		R.Sequence = Sequence;
		R.Epoch = Store.GetEpoch();
		R.ExpectedRevision = Store.GetRevision();
		R.RadiusMeters = 2.;
		return R;
	}

	FCCLTerrainSaveContext Context()
	{
		FCCLTerrainSaveContext C;
		C.WorldId = Definition().WorldId;
		C.WorldGeneration = 7;
		C.GameSeconds = 10.;
		C.WorldSeconds = 600.;
		return C;
	}

	void FixCRC(TArray<uint8>& Bytes)
	{
		const uint32 CRC = FCrc::MemCrc32(Bytes.GetData(), Bytes.Num() - 4);
		FMemory::Memcpy(Bytes.GetData() + Bytes.Num() - 4, &CRC, 4);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLTerrainBoundaryTest, "CCL.Environment.Terrain.BoundaryOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLTerrainBoundaryTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLTerrainStore Store;
	const auto D = Definition();
	const auto A = Authority(D);
	TestTrue(TEXT("initialize"), Store.Initialize(D, Error));
	TestEqual(TEXT("base field has no dense allocation"), Store.GetSnapshot().Chunks.Num(), 0);
	for (int32 Value : {-65, -64, -33, -32, -31, -1, 0, 1, 31, 32, 33, 64})
	{
		const FIntVector Sample(Value, -Value, Value);
		const int32 Local = FCCLTerrainStore::LocalSampleIndex(Sample);
		TestTrue(TEXT("local index in canonical owner"), Local >= 0 && Local < FCCLTerrainStore::SamplesPerChunk);
		TestEqual(TEXT("signed owner roundtrip"), FCCLTerrainStore::GlobalSample(FCCLTerrainStore::OwnerForSample(Sample), Local), Sample);
	}

	TestEqual(TEXT("negative one uses the preceding chunk"), FCCLTerrainStore::OwnerForSample(FIntVector(-1, 0, 0)).X, -1);
	FCCLTerrainCandidate Candidate;
	const auto R = Edit(Store, A);
	if (!TestTrue(TEXT("prepare boundary excavation"), Store.PrepareEdit(R, A, Candidate, Error) == ECCLTerrainPrepareResult::Prepared))
	{
		AddError(Error);
		return false;
	}

	TestEqual(TEXT("candidate does not publish"), Store.GetRevision(), uint64(1));
	TestEqual(TEXT("old center still ground"), FCCLTerrainStore::ReadSample(Store.GetSnapshot(), FIntVector::ZeroValue).DistanceMillimeters, int16(0));
	TestTrue(TEXT("candidate center is excavated"), FCCLTerrainStore::ReadSample(Candidate.GetSnapshot(), FIntVector::ZeroValue).DistanceMillimeters > 0);
	TestTrue(TEXT("negative-side mesh included"), Candidate.GetMeshChunks().Contains(FIntVector(-1, -1, -1)));
	TestTrue(TEXT("positive-side mesh included"), Candidate.GetMeshChunks().Contains(FIntVector(0, 0, 0)));
	const auto Before = Store.GetSnapshot();
	TestTrue(TEXT("commit prepared value state"), Store.CommitEdit(Candidate, A, Error));
	TestEqual(TEXT("immutable previous snapshot retained"), FCCLTerrainStore::ReadSample(Before, FIntVector::ZeroValue).DistanceMillimeters, int16(0));
	double Left = 0.;
	double Right = 0.;
	TestTrue(TEXT("sample across chunk boundary"), FCCLTerrainStore::ReadDensityMeters(Store.GetSnapshot(), FVector(-1.e-6, 0., -1.), Left, Error)
		&& FCCLTerrainStore::ReadDensityMeters(Store.GetSnapshot(), FVector(1.e-6, 0., -1.), Right, Error));
	TestTrue(TEXT("no density seam across signed owner boundary"), FMath::Abs(Left - Right) < 1.e-6);
	for (const auto& Entry : Store.GetSnapshot().Chunks)
	{
		for (const auto& Sample : Entry.Value->Overrides)
		{
			const FIntVector Global = FCCLTerrainStore::GlobalSample(Entry.Key, Sample.Key);
			TestEqual(TEXT("every override has exactly its canonical owner"), FCCLTerrainStore::OwnerForSample(Global), Entry.Key);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLTerrainAtomicTest, "CCL.Environment.Terrain.AtomicCandidates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLTerrainAtomicTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLTerrainStore Store;
	const auto D = Definition();
	auto A = Authority(D);
	const auto B = Authority(D, 2);
	Store.Initialize(D, Error);
	FCCLTerrainCandidate First;
	FCCLTerrainCandidate Second;
	auto R = Edit(Store, A);
	auto Other = Edit(Store, B);
	Other.CenterMeters.X = 12.;
	TestTrue(TEXT("prepare first candidate"), Store.PrepareEdit(R, A, First, Error) == ECCLTerrainPrepareResult::Prepared);
	TestTrue(TEXT("prepare concurrent candidate"), Store.PrepareEdit(Other, B, Second, Error) == ECCLTerrainPrepareResult::Prepared);
	++A.PolicyRevision;
	TestFalse(TEXT("changed permission policy invalidates prepared work"), Store.CommitEdit(First, A, Error));
	--A.PolicyRevision;
	A.bCanExcavate = 0;
	TestFalse(TEXT("permission rechecked at publication"), Store.CommitEdit(First, A, Error));
	TestEqual(TEXT("all rejected attempts retain revision"), Store.GetRevision(), uint64(1));
	A.bCanExcavate = 1;
	TestTrue(TEXT("first commit"), Store.CommitEdit(First, A, Error));
	TestFalse(TEXT("stale concurrent work cannot overwrite first edit"), Store.CommitEdit(Second, B, Error));
	TestFalse(TEXT("same candidate cannot publish twice"), Store.CommitEdit(First, A, Error));
	TestEqual(TEXT("one revision for one commit"), Store.GetRevision(), uint64(2));
	TestEqual(TEXT("second candidate remains unpublished"), FCCLTerrainStore::ReadSample(Store.GetSnapshot(), FIntVector(24, 0, 0)).DistanceMillimeters, int16(0));
	Other = Edit(Store, B);
	Other.CenterMeters.X = 12.;
	TestTrue(TEXT("rebase by preparing against current state"), Store.PrepareEdit(Other, B, Second, Error) == ECCLTerrainPrepareResult::Prepared);
	TestTrue(TEXT("sequential second commit succeeds"), Store.CommitEdit(Second, B, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLTerrainAuthorityTest, "CCL.Environment.Terrain.ProtectionAndBudgets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLTerrainAuthorityTest::RunTest(const FString& Parameters)
{
	FString Error;
	auto D = Definition();
	FCCLTerrainProtection Protection;
	Protection.Id = FGuid(20, 30, 40, 50);
	Protection.BoundsMeters = FBox(FVector(10., 10., -2.), FVector(12., 12., 2.));
	D.ProtectedRegions.Add(Protection);
	FCCLTerrainStore Store;
	TestTrue(TEXT("protected world initializes"), Store.Initialize(D, Error));
	auto A = Authority(D);
	FCCLTerrainCandidate Candidate;
	auto R = Edit(Store, A);
	R.CenterMeters = FVector(8., 10., 0.);
	R.RadiusMeters = 0.5;
	TestTrue(TEXT("interpolation margin cannot cross protected border"), Store.PrepareEdit(R, A, Candidate, Error) == ECCLTerrainPrepareResult::Failed);
	R = Edit(Store, A);
	A.bCanExcavate = 0;
	TestTrue(TEXT("unauthorized excavation refused"), Store.PrepareEdit(R, A, Candidate, Error) == ECCLTerrainPrepareResult::Failed);
	A.bCanExcavate = 1;
	R.PrincipalId = FGuid::NewGuid();
	TestTrue(TEXT("client cannot choose another principal"), Store.PrepareEdit(R, A, Candidate, Error) == ECCLTerrainPrepareResult::Failed);
	R = Edit(Store, A);
	R.RadiusMeters = 9.;
	TestTrue(TEXT("radius is bounded by server policy"), Store.PrepareEdit(R, A, Candidate, Error) == ECCLTerrainPrepareResult::Failed);
	R = Edit(Store, A);
	R.CenterMeters.X = 63.;
	TestTrue(TEXT("edge edit cannot escape the editable region"), Store.PrepareEdit(R, A, Candidate, Error) == ECCLTerrainPrepareResult::Failed);
	R = Edit(Store, A);
	R.CenterMeters.Z = 8.;
	TestTrue(TEXT("excavating air is a no-op failure"), Store.PrepareEdit(R, A, Candidate, Error) == ECCLTerrainPrepareResult::Failed);
	TestEqual(TEXT("no request sequence consumed by failures"), Store.GetSnapshot().Receipts.Num(), 0);
	TestEqual(TEXT("no speculative chunks retained"), Store.GetSnapshot().Chunks.Num(), 0);
	D = Definition();
	D.SampleSpacingMeters = 0.05;
	D.MinimumCell = FIntVector(-512);
	D.MaximumCell = FIntVector(512);
	Store.Initialize(D, Error);
	A = Authority(D);
	R = Edit(Store, A);
	R.RadiusMeters = 8.;
	TestTrue(TEXT("oversized voxel work refused before allocation"), Store.PrepareEdit(R, A, Candidate, Error) == ECCLTerrainPrepareResult::Failed);
	TestTrue(TEXT("work budget is the rejection cause"), Error.Contains(TEXT("sample budget")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLTerrainVolumeTest, "CCL.Environment.Terrain.VolumetricExcavationAndDeposit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLTerrainVolumeTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLTerrainStore Store;
	const auto D = Definition();
	const auto A = Authority(D);
	Store.Initialize(D, Error);
	auto Cave = Edit(Store, A);
	Cave.CenterMeters.Z = -6.;
	Cave.RadiusMeters = 1.;
	FCCLTerrainCandidate Candidate;
	TestTrue(TEXT("prepare underground cavity"), Store.PrepareEdit(Cave, A, Candidate, Error) == ECCLTerrainPrepareResult::Prepared);
	TestTrue(TEXT("commit cavity"), Store.CommitEdit(Candidate, A, Error));
	TestTrue(TEXT("interior below ground is empty"), FCCLTerrainStore::ReadSample(Store.GetSnapshot(), FIntVector(0, 0, -12)).DistanceMillimeters > 0);
	TestTrue(TEXT("solid roof remains above cavity"), FCCLTerrainStore::ReadSample(Store.GetSnapshot(), FIntVector(0, 0, -8)).DistanceMillimeters < 0);
	TestEqual(TEXT("ground surface above cavity is unchanged"), FCCLTerrainStore::ReadSample(Store.GetSnapshot(), FIntVector::ZeroValue).DistanceMillimeters, int16(0));
	auto Hill = Edit(Store, A, 2);
	Hill.Kind = ECCLTerrainEditKind::Deposit;
	Hill.CenterMeters.X = 8.;
	Hill.Material = 2;
	TestTrue(TEXT("prepare rock deposit"), Store.PrepareEdit(Hill, A, Candidate, Error) == ECCLTerrainPrepareResult::Prepared);
	TestTrue(TEXT("commit deposit"), Store.CommitEdit(Candidate, A, Error));
	const auto Raised = FCCLTerrainStore::ReadSample(Store.GetSnapshot(), FIntVector(16, 0, 2));
	TestTrue(TEXT("solid raised above original ground"), Raised.DistanceMillimeters < 0);
	TestEqual(TEXT("deposited material retained"), Raised.Material, uint16(2));
	TestTrue(TEXT("deposit does not flatten underground cavity"), FCCLTerrainStore::ReadSample(Store.GetSnapshot(), FIntVector(0, 0, -12)).DistanceMillimeters > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLTerrainReplayTest, "CCL.Environment.Terrain.RequestReplayAndRestore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLTerrainReplayTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLTerrainStore Store;
	const auto D = Definition();
	const auto A = Authority(D);
	Store.Initialize(D, Error);
	const auto R = Edit(Store, A);
	FCCLTerrainCandidate Candidate;
	TestTrue(TEXT("prepare"), Store.PrepareEdit(R, A, Candidate, Error) == ECCLTerrainPrepareResult::Prepared);
	TestTrue(TEXT("commit"), Store.CommitEdit(Candidate, A, Error));
	TestTrue(TEXT("exact retransmission acknowledged without applying again"), Store.PrepareEdit(R, A, Candidate, Error) == ECCLTerrainPrepareResult::AlreadyApplied);
	auto Changed = R;
	Changed.CenterMeters.X += 1.;
	TestTrue(TEXT("same sequence with different payload rejected"), Store.PrepareEdit(Changed, A, Candidate, Error) == ECCLTerrainPrepareResult::Failed);
	auto Next = Edit(Store, A, 2);
	Next.CenterMeters.X = 8.;
	TestTrue(TEXT("pending candidate before restore"), Store.PrepareEdit(Next, A, Candidate, Error) == ECCLTerrainPrepareResult::Prepared);
	TArray<uint8> Bytes;
	const auto C = Context();
	TestTrue(TEXT("capture committed state only"), Store.Capture(C, Bytes, Error));
	const FGuid Epoch = Store.GetEpoch();
	TestTrue(TEXT("restore matching save generation"), Store.Restore(Bytes, C, Error));
	TestTrue(TEXT("restore invalidates prior execution epoch"), Store.GetEpoch() != Epoch);
	TestFalse(TEXT("pre-restore prepared work cannot publish"), Store.CommitEdit(Candidate, A, Error));
	TestTrue(TEXT("persisted receipt survives restart"), Store.PrepareEdit(R, A, Candidate, Error) == ECCLTerrainPrepareResult::AlreadyApplied);
	TestTrue(TEXT("new request with old epoch rejected"), Store.PrepareEdit(Next, A, Candidate, Error) == ECCLTerrainPrepareResult::Failed);
	Next = Edit(Store, A, 2);
	Next.CenterMeters.X = 8.;
	TestTrue(TEXT("next sequence against restored epoch works"), Store.PrepareEdit(Next, A, Candidate, Error) == ECCLTerrainPrepareResult::Prepared);
	TestTrue(TEXT("commit new request after restore"), Store.CommitEdit(Candidate, A, Error));
	TestTrue(TEXT("old acknowledged sequence cannot apply twice"), Store.PrepareEdit(R, A, Candidate, Error) == ECCLTerrainPrepareResult::Failed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLTerrainCodecTest, "CCL.Environment.Terrain.RegionCodec",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLTerrainCodecTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLTerrainStore Store;
	const auto D = Definition();
	const auto A = Authority(D);
	Store.Initialize(D, Error);
	FCCLTerrainCandidate Candidate;
	Store.PrepareEdit(Edit(Store, A), A, Candidate, Error);
	Store.CommitEdit(Candidate, A, Error);
	TArray<uint8> Bytes;
	const auto C = Context();
	TestTrue(TEXT("encode sparse terrain"), Store.Capture(C, Bytes, Error));
	auto Reordered = Store.GetSnapshot();
	TArray<FIntVector> Keys;
	Reordered.Chunks.GetKeys(Keys);
	Reordered.Chunks.Reset();
	for (int32 I = Keys.Num() - 1; I >= 0; --I)
	{
		Reordered.Chunks.Add(Keys[I], Store.GetSnapshot().Chunks[Keys[I]]);
	}

	TArray<uint8> Again;
	TestTrue(TEXT("encode different map insertion order"), FCCLTerrainCodec::Encode(Reordered, C, Again, Error));
	TestTrue(TEXT("byte stable canonical encoding"), Again == Bytes);
	for (int32 Fault = 0; Fault < 5; ++Fault)
	{
		auto Bad = Bytes;
		if (Fault == 0)
		{
			Bad[Bad.Num() / 2] ^= 1;
		}

		if (Fault == 1)
		{
			Bad.SetNum(Bad.Num() - 1);
		}

		if (Fault == 2)
		{
			const uint32 Version = 99;
			FMemory::Memcpy(Bad.GetData() + 4, &Version, 4);
			FixCRC(Bad);
		}

		if (Fault == 3)
		{
			Bad.Insert(uint8(0), Bad.Num() - 4);
			FixCRC(Bad);
		}

		if (Fault == 4)
		{
			// Header 8 + save context 44 + definition IDs/versions 56 precede the body-name length.
			const int32 Length = MAX_int32;
			FMemory::Memcpy(Bad.GetData() + 108, &Length, 4);
			FixCRC(Bad);
		}

		FCCLTerrainSnapshot Output;
		Output.Revision = 999;
		FCCLTerrainSaveContext OutputContext;
		OutputContext.WorldGeneration = 999;
		TestFalse(TEXT("corrupt, truncated, unknown schema, trailing or oversized string rejected"), FCCLTerrainCodec::Decode(Bad, Output, OutputContext, Error));
		TestEqual(TEXT("failure preserves snapshot output"), Output.Revision, uint64(999));
		TestEqual(TEXT("failure preserves context output"), OutputContext.WorldGeneration, uint64(999));
	}

	const auto Epoch = Store.GetEpoch();
	auto WrongContext = C;
	++WrongContext.WorldGeneration;
	TestFalse(TEXT("mixed save generation refused"), Store.Restore(Bytes, WrongContext, Error));
	WrongContext = C;
	WrongContext.WorldId = FGuid::NewGuid();
	TestFalse(TEXT("other world refused"), Store.Restore(Bytes, WrongContext, Error));
	WrongContext = C;
	WrongContext.WorldSeconds += 1.;
	TestFalse(TEXT("different completed time refused"), Store.Restore(Bytes, WrongContext, Error));
	TestEqual(TEXT("rejected restore leaves current epoch"), Store.GetEpoch(), Epoch);
	FCCLTerrainStore ChangedDefinition;
	auto Changed = D;
	Changed.BaseHeightMeters = 1.;
	ChangedDefinition.Initialize(Changed, Error);
	TestFalse(TEXT("same version with changed base definition cannot load silently"), ChangedDefinition.Restore(Bytes, C, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLTerrainMalformedTest, "CCL.Environment.Terrain.MalformedCollections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLTerrainMalformedTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLTerrainStore Store;
	Store.Initialize(Definition(), Error);
	TArray<uint8> Bytes;
	TestTrue(TEXT("empty region record"), Store.Capture(Context(), Bytes, Error));
	const int32 ChunkCountOffset = Bytes.Num() - 12;
	for (int32 Fault = 0; Fault < 3; ++Fault)
	{
		auto Bad = Bytes;
		if (Fault == 0)
		{
			const int32 NegativeCount = -1;
			FMemory::Memcpy(Bad.GetData() + ChunkCountOffset, &NegativeCount, 4);
		}
		else
		{
			TArray<uint8> Entries;
			FMemoryWriter Writer(Entries);
			for (int32 Index = 0; Index < (Fault == 1 ? 2 : 1); ++Index)
			{
				int32 Zero = 0;
				uint64 Revision = 1;
				int32 Samples = Fault == 1 ? 0 : FCCLTerrainStore::SamplesPerChunk + 1;
				Writer << Zero << Zero << Zero << Revision << Samples;
			}

			const int32 Chunks = Fault == 1 ? 2 : 1;
			FMemory::Memcpy(Bad.GetData() + ChunkCountOffset, &Chunks, 4);
			Bad.Insert(Entries, ChunkCountOffset + 4);
		}

		FixCRC(Bad);
		FCCLTerrainSnapshot Output;
		FCCLTerrainSaveContext C;
		TestFalse(TEXT("negative count, duplicate chunk or oversized sample array rejected"), FCCLTerrainCodec::Decode(Bad, Output, C, Error));
	}

	auto Invalid = Store.GetSnapshot();
	auto Chunk = MakeShared<FCCLTerrainChunkData, ESPMode::ThreadSafe>();
	FCCLTerrainVoxel V;
	V.DistanceMillimeters = -100;
	V.Material = 255;
	Chunk->Overrides.Add(0, V);
	Invalid.Chunks.Add(FIntVector::ZeroValue, Chunk);
	TArray<uint8> Unchanged = {1, 2, 3};
	TestFalse(TEXT("unknown material cannot be saved"), FCCLTerrainCodec::Encode(Invalid, Context(), Unchanged, Error));
	TestTrue(TEXT("failed encoding leaves caller bytes"), Unchanged == TArray<uint8>({1, 2, 3}));
	return true;
}
#endif
