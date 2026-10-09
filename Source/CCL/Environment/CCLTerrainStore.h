#pragma once

#include "CoreMinimal.h"

enum class ECCLTerrainEditKind : uint8
{
	Excavate,
	Deposit
};

enum class ECCLTerrainPrepareResult : uint8
{
	Failed,
	Prepared,
	AlreadyApplied
};

struct CCL_API FCCLTerrainVoxel
{
	int16 DistanceMillimeters = 0;
	uint16 Material = 1;

	bool operator==(const FCCLTerrainVoxel& Other) const
	{
		return DistanceMillimeters == Other.DistanceMillimeters && Material == Other.Material;
	}
};

struct CCL_API FCCLTerrainProtection
{
	FGuid Id;
	FBox BoundsMeters = FBox(EForceInit::ForceInit);
};

struct CCL_API FCCLTerrainDefinition
{
	FGuid WorldId;
	FGuid RegionId;
	FGuid DefinitionId;
	uint32 BaseWorldVersion = 1;
	uint32 DefinitionVersion = 1;
	FName BodyId = TEXT("World");
	FVector OriginMeters = FVector::ZeroVector;
	double SampleSpacingMeters = 0.5;
	double BaseHeightMeters = 0.;
	// Cells are [MinimumCell, MaximumCell); lattice samples include the maximum boundary.
	FIntVector MinimumCell = FIntVector(-128, -128, -32);
	FIntVector MaximumCell = FIntVector(128, 128, 32);
	TArray<FName> Materials = {TEXT("Air"), TEXT("Soil"), TEXT("Rock")};
	uint16 BaseMaterial = 1;
	TArray<FCCLTerrainProtection> ProtectedRegions;
};

struct CCL_API FCCLTerrainEdit
{
	FGuid PrincipalId;
	uint64 Sequence = 0;
	FGuid Epoch;
	uint64 ExpectedRevision = 0;
	ECCLTerrainEditKind Kind = ECCLTerrainEditKind::Excavate;
	FVector CenterMeters = FVector::ZeroVector;
	double RadiusMeters = 1.;
	uint16 Material = 1;

	bool operator==(const FCCLTerrainEdit& Other) const;
};

// Produced by server policy, never accepted directly from a client request.
struct CCL_API FCCLTerrainAuthority
{
	FGuid PrincipalId;
	uint64 PolicyRevision = 1;
	FBox AllowedBoundsMeters = FBox(EForceInit::ForceInit);
	double MaximumRadiusMeters = 8.;
	uint8 bCanExcavate = 0;
	uint8 bCanDeposit = 0;
};

struct CCL_API FCCLTerrainChunkData
{
	uint64 Revision = 1;
	TMap<int32, FCCLTerrainVoxel> Overrides;
};

struct CCL_API FCCLTerrainReceipt
{
	FCCLTerrainEdit Request;
	uint64 CommittedRevision = 0;
};

struct CCL_API FCCLTerrainSnapshot
{
	FCCLTerrainDefinition Definition;
	uint64 Revision = 1;
	TMap<FIntVector, TSharedPtr<const FCCLTerrainChunkData, ESPMode::ThreadSafe>> Chunks;
	TMap<FGuid, FCCLTerrainReceipt> Receipts;
};

// Bound to the owning world's completed save generation; the terrain store does not advance time.
struct CCL_API FCCLTerrainSaveContext
{
	FGuid WorldId;
	uint32 BaseWorldVersion = 1;
	uint64 WorldGeneration = 0;
	double GameSeconds = 0.;
	double WorldSeconds = 0.;

	bool operator==(const FCCLTerrainSaveContext& Other) const;
};

class CCL_API FCCLTerrainCandidate
{
public:
	bool IsValid() const { return Before.IsValid() && After.IsValid(); }
	const FCCLTerrainSnapshot& GetSnapshot() const { check(After); return *After; }
	const TArray<FIntVector>& GetMeshChunks() const { return MeshChunks; }
	const FBox& GetAffectedBoundsMeters() const { return AffectedBoundsMeters; }
	FGuid GetTicket() const { return Ticket; }

private:
	friend class FCCLTerrainStore;
	TSharedPtr<const FCCLTerrainSnapshot, ESPMode::ThreadSafe> Before;
	TSharedPtr<const FCCLTerrainSnapshot, ESPMode::ThreadSafe> After;
	TArray<FIntVector> MeshChunks;
	FBox AffectedBoundsMeters = FBox(EForceInit::ForceInit);
	FCCLTerrainEdit Request;
	FGuid Ticket;
	uint64 PolicyRevision = 0;
};

class CCL_API FCCLTerrainStore
{
public:
	bool Initialize(const FCCLTerrainDefinition& Definition, FString& Error);
	ECCLTerrainPrepareResult PrepareEdit(const FCCLTerrainEdit& Request, const FCCLTerrainAuthority& Authority,
		FCCLTerrainCandidate& OutCandidate, FString& Error) const;
	// Scalar publication only. The runtime coordinator must prepare collision and all dependent state first.
	bool ValidateCandidate(const FCCLTerrainCandidate& Candidate, const FCCLTerrainAuthority& Authority, FString& Error) const;
	bool CommitEdit(const FCCLTerrainCandidate& Candidate, const FCCLTerrainAuthority& Authority, FString& Error);
	bool Capture(const FCCLTerrainSaveContext& Context, TArray<uint8>& OutBytes, FString& Error) const;
	bool Restore(const TArray<uint8>& Bytes, const FCCLTerrainSaveContext& ExpectedContext, FString& Error);

	bool IsInitialized() const { return Committed.IsValid(); }
	FGuid GetEpoch() const { return Epoch; }
	uint64 GetRevision() const { return Committed ? Committed->Revision : 0; }
	const FCCLTerrainSnapshot& GetSnapshot() const { check(Committed); return *Committed; }

	static bool ValidateDefinition(const FCCLTerrainDefinition& Definition, FString& Error);
	static bool ValidateSnapshot(const FCCLTerrainSnapshot& Snapshot, FString& Error);
	static FCCLTerrainVoxel ReadSample(const FCCLTerrainSnapshot& Snapshot, const FIntVector& Sample);
	static bool ReadDensityMeters(const FCCLTerrainSnapshot& Snapshot, const FVector& PositionMeters, double& OutDensity, FString& Error);
	static FIntVector OwnerForSample(const FIntVector& Sample);
	static int32 LocalSampleIndex(const FIntVector& Sample);
	static FIntVector GlobalSample(const FIntVector& Chunk, int32 LocalIndex);
	static FBox EditableBoundsMeters(const FCCLTerrainDefinition& Definition);

	static constexpr int32 SamplesPerAxis = 32;
	static constexpr int32 SamplesPerChunk = 32 * 32 * 32;
	static constexpr int32 MaximumChunks = 4096;
	static constexpr int32 MaximumOverrides = 2 * 1024 * 1024;
	static constexpr int32 MaximumBrushSamples = 262144;
	static constexpr int32 MaximumPrincipals = 4096;

private:
	bool ValidateRequest(const FCCLTerrainEdit& Request, const FCCLTerrainAuthority& Authority,
		FBox& OutAffectedBounds, FString& Error) const;

private:
	TSharedPtr<const FCCLTerrainSnapshot, ESPMode::ThreadSafe> Committed;
	FGuid Epoch;
};

class CCL_API FCCLTerrainCodec
{
public:
	static bool Encode(const FCCLTerrainSnapshot& Snapshot, const FCCLTerrainSaveContext& Context,
		TArray<uint8>& OutBytes, FString& Error);
	static bool Decode(const TArray<uint8>& Bytes, FCCLTerrainSnapshot& OutSnapshot,
		FCCLTerrainSaveContext& OutContext, FString& Error);
	static bool SameDefinition(const FCCLTerrainDefinition& A, const FCCLTerrainDefinition& B);

	static constexpr int32 MaximumBytes = 32 * 1024 * 1024;
};
