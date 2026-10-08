#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "CCLAgentTypes.generated.h"

USTRUCT(BlueprintType)
struct CCL_API FCCLTargetReference
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Kind;

	UPROPERTY()
	FGuid Id;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLLocationReference
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid PlaceId;

	UPROPERTY()
	FVector Position = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLPersistentIntent
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Activity;

	UPROPERTY()
	FCCLTargetReference Target;

	UPROPERTY()
	FGuid SupportingLifeGoalId;

	UPROPERTY()
	FGuid OpportunityId;

	UPROPERTY()
	double StartedTime = 0;

	UPROPERTY()
	double ExpireTime = 0;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLAgentFeatureState
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Version = 1;

	UPROPERTY()
	FInstancedStruct Data;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLAgentRecord
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid Id;

	UPROPERTY()
	FPrimaryAssetId DefinitionId;

	UPROPERTY()
	FGameplayTagContainer Roles;

	UPROPERTY()
	FCCLLocationReference Location;

	UPROPERTY()
	double LastSimulatedTime = 0;

	UPROPERTY()
	FCCLPersistentIntent Intent;

	UPROPERTY()
	TMap<FGameplayTag, FCCLAgentFeatureState> Features;
};

struct CCL_API FCCLAgentHandle
{
	FGuid Id;
	uint64 Generation = 0;
};

// All channels of this initial adapter share one lease. It is invalidated on handoff.
struct CCL_API FCCLAgentLease
{
	FCCLAgentHandle Handle;
	FGuid Writer;
	uint64 Epoch = 0;
};

struct CCL_API FCCLFeatureRegistration
{
	FGameplayTag Tag;
	UScriptStruct* Type = nullptr;
	int32 Version = 1;
	FGameplayTagContainer Dependencies;
	TFunction<bool(int32, FInstancedStruct&)> Migrate;
	TFunction<bool(const FInstancedStruct&)> Validate;
};

class CCL_API FCCLFeatureRegistry
{
public:
	bool Register(FCCLFeatureRegistration Registration);
	bool UpgradeAndValidate(FCCLAgentRecord& Record, FString& Error) const;
	const FCCLFeatureRegistration* Find(FGameplayTag Tag) const { return Entries.Find(Tag); }

private:
	TMap<FGameplayTag, FCCLFeatureRegistration> Entries;
};

// Game-thread owner. Borrowed pointers must not survive a mutation of this store.
class CCL_API FCCLAgentStore
{
public:
	FCCLAgentHandle Add(FCCLAgentRecord Record, const FCCLFeatureRegistry& Registry, FString& Error);
	FCCLAgentLease Acquire(FCCLAgentHandle Handle, FGuid Writer);
	bool Commit(const FCCLAgentLease& Lease, FCCLAgentRecord Record, const FCCLFeatureRegistry& Registry, FString& Error);
	bool CommitBatch(TArray<TPair<FCCLAgentLease, FCCLAgentRecord>> Updates, const FCCLFeatureRegistry& Registry, FString& Error);
	bool Release(const FCCLAgentLease& Lease);
	bool Replace(TArray<FCCLAgentRecord> Records, const FCCLFeatureRegistry& Registry, FString& Error);
	bool Snapshot(TArray<FCCLAgentRecord>& OutRecords) const;
	const FCCLAgentRecord* Find(FCCLAgentHandle Handle) const;
	FCCLAgentHandle GetHandle(FGuid Id) const;

private:
	struct FEntry
	{
		FCCLAgentRecord Record;
		uint64 Generation = 0;
		FGuid Writer;
		uint64 Epoch = 0;
	};

	TMap<FGuid, FEntry> Records;
	uint64 NextGeneration = 1;
	uint64 NextEpoch = 1;
};
