#pragma once

#include "CoreMinimal.h"

enum class EKSITerrainEditKind : uint8
{
	Excavate,
	Deposit
};

enum class EKSITerrainPrepareResult : uint8
{
	Failed,
	Prepared,
	AlreadyApplied
};

struct KSITERRAIN_API FKSITerrainVoxel
{
	int16 DistanceMillimeters = 0;
	uint16 Material = 1;

	bool operator==(const FKSITerrainVoxel& Other) const
	{
		return DistanceMillimeters == Other.DistanceMillimeters && Material == Other.Material;
	}
};

struct KSITERRAIN_API FKSITerrainProtection
{
	FGuid Id;
	FBox BoundsMeters = FBox(EForceInit::ForceInit);
};

struct KSITERRAIN_API FKSITerrainDefinition
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
	TArray<FKSITerrainProtection> ProtectedRegions;
};

struct KSITERRAIN_API FKSITerrainEdit
{
	FGuid PrincipalId;
	uint64 Sequence = 0;
	FGuid Epoch;
	uint64 ExpectedRevision = 0;
	EKSITerrainEditKind Kind = EKSITerrainEditKind::Excavate;
	FVector CenterMeters = FVector::ZeroVector;
	double RadiusMeters = 1.;
	uint16 Material = 1;

	bool operator==(const FKSITerrainEdit& Other) const;
};

// Produced by server policy, never accepted directly from a client request.
struct KSITERRAIN_API FKSITerrainAuthority
{
	FGuid PrincipalId;
	uint64 PolicyRevision = 1;
	FBox AllowedBoundsMeters = FBox(EForceInit::ForceInit);
	double MaximumRadiusMeters = 8.;
	uint8 bCanExcavate = 0;
	uint8 bCanDeposit = 0;
};

struct KSITERRAIN_API FKSITerrainChunkData
{
	uint64 Revision = 1;
	TMap<int32, FKSITerrainVoxel> Overrides;
};

struct KSITERRAIN_API FKSITerrainReceipt
{
	FKSITerrainEdit Request;
	uint64 CommittedRevision = 0;
};

struct KSITERRAIN_API FKSITerrainSnapshot
{
	FKSITerrainDefinition Definition;
	uint64 Revision = 1;
	TMap<FIntVector, TSharedPtr<const FKSITerrainChunkData, ESPMode::ThreadSafe>> Chunks;
	TMap<FGuid, FKSITerrainReceipt> Receipts;
};

// Bound to the owning world's completed save generation; the terrain store does not advance time.
struct KSITERRAIN_API FKSITerrainSaveContext
{
	FGuid WorldId;
	uint32 BaseWorldVersion = 1;
	uint64 WorldGeneration = 0;
	double GameSeconds = 0.;
	double WorldSeconds = 0.;

	bool operator==(const FKSITerrainSaveContext& Other) const;
};

class KSITERRAIN_API FKSITerrainCandidate
{
public:
	bool IsValid() const { return Before.IsValid() && After.IsValid(); }
	const FKSITerrainSnapshot& GetSnapshot() const { check(After); return *After; }
	const TArray<FIntVector>& GetMeshChunks() const { return MeshChunks; }
	const FBox& GetAffectedBoundsMeters() const { return AffectedBoundsMeters; }
	FGuid GetTicket() const { return Ticket; }

private:
	friend class FKSITerrainStore;
	TSharedPtr<const FKSITerrainSnapshot, ESPMode::ThreadSafe> Before;
	TSharedPtr<const FKSITerrainSnapshot, ESPMode::ThreadSafe> After;
	TArray<FIntVector> MeshChunks;
	FBox AffectedBoundsMeters = FBox(EForceInit::ForceInit);
	FKSITerrainEdit Request;
	FGuid Ticket;
	uint64 PolicyRevision = 0;
};

class KSITERRAIN_API FKSITerrainStore
{
public:
	bool Initialize(const FKSITerrainDefinition& Definition, FString& Error);
	EKSITerrainPrepareResult PrepareEdit(const FKSITerrainEdit& Request, const FKSITerrainAuthority& Authority,
		FKSITerrainCandidate& OutCandidate, FString& Error) const;
	// Scalar publication only. The runtime coordinator must prepare collision and all dependent state first.
	bool ValidateCandidate(const FKSITerrainCandidate& Candidate, const FKSITerrainAuthority& Authority, FString& Error) const;
	bool CommitEdit(const FKSITerrainCandidate& Candidate, const FKSITerrainAuthority& Authority, FString& Error);
	bool Capture(const FKSITerrainSaveContext& Context, TArray<uint8>& OutBytes, FString& Error) const;
	bool Restore(const TArray<uint8>& Bytes, const FKSITerrainSaveContext& ExpectedContext, FString& Error);

	bool IsInitialized() const { return Committed.IsValid(); }
	FGuid GetEpoch() const { return Epoch; }
	uint64 GetRevision() const { return Committed ? Committed->Revision : 0; }
	const FKSITerrainSnapshot& GetSnapshot() const { check(Committed); return *Committed; }

	static bool ValidateDefinition(const FKSITerrainDefinition& Definition, FString& Error);
	static bool ValidateSnapshot(const FKSITerrainSnapshot& Snapshot, FString& Error);
	static FKSITerrainVoxel ReadSample(const FKSITerrainSnapshot& Snapshot, const FIntVector& Sample);
	static bool ReadDensityMeters(const FKSITerrainSnapshot& Snapshot, const FVector& PositionMeters, double& OutDensity, FString& Error);
	static FIntVector OwnerForSample(const FIntVector& Sample);
	static int32 LocalSampleIndex(const FIntVector& Sample);
	static FIntVector GlobalSample(const FIntVector& Chunk, int32 LocalIndex);
	static FBox EditableBoundsMeters(const FKSITerrainDefinition& Definition);

	static constexpr int32 SamplesPerAxis = 32;
	static constexpr int32 SamplesPerChunk = 32 * 32 * 32;
	static constexpr int32 MaximumChunks = 4096;
	static constexpr int32 MaximumOverrides = 2 * 1024 * 1024;
	static constexpr int32 MaximumBrushSamples = 262144;
	static constexpr int32 MaximumPrincipals = 4096;

private:
	bool ValidateRequest(const FKSITerrainEdit& Request, const FKSITerrainAuthority& Authority,
		FBox& OutAffectedBounds, FString& Error) const;

private:
	TSharedPtr<const FKSITerrainSnapshot, ESPMode::ThreadSafe> Committed;
	FGuid Epoch;
};

class KSITERRAIN_API FKSITerrainCodec
{
public:
	static bool Encode(const FKSITerrainSnapshot& Snapshot, const FKSITerrainSaveContext& Context,
		TArray<uint8>& OutBytes, FString& Error);
	static bool Decode(const TArray<uint8>& Bytes, FKSITerrainSnapshot& OutSnapshot,
		FKSITerrainSaveContext& OutContext, FString& Error);
	static bool SameDefinition(const FKSITerrainDefinition& A, const FKSITerrainDefinition& B);

	static constexpr int32 MaximumBytes = 32 * 1024 * 1024;
};
