#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "KSIAgentTypes.generated.h"

USTRUCT(BlueprintType)
struct KSIAGENT_API FKSITargetReference
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FGameplayTag Kind;

	UPROPERTY(EditAnywhere)
	FGuid Id;
};

USTRUCT(BlueprintType)
struct KSIAGENT_API FKSILocationReference
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FGuid PlaceId;

	UPROPERTY(EditAnywhere)
	FVector Position = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct KSIAGENT_API FKSIPersistentIntent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FGameplayTag Activity;

	UPROPERTY(EditAnywhere)
	FKSITargetReference Target;

	UPROPERTY(EditAnywhere)
	FGuid SupportingLifeGoalId;

	UPROPERTY(EditAnywhere)
	FGuid OpportunityId;

	UPROPERTY(EditAnywhere)
	double StartedTime = 0;

	UPROPERTY(EditAnywhere)
	double ExpireTime = 0;
};

USTRUCT(BlueprintType)
struct KSIAGENT_API FKSIAgentFeatureState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	int32 Version = 1;

	UPROPERTY(EditAnywhere)
	FInstancedStruct Data;
};

USTRUCT(BlueprintType)
struct KSIAGENT_API FKSIAgentRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FGuid Id;

	UPROPERTY(EditAnywhere)
	FPrimaryAssetId DefinitionId;

	UPROPERTY(EditAnywhere)
	FGameplayTagContainer Roles;

	UPROPERTY(EditAnywhere)
	FKSILocationReference Location;

	UPROPERTY(EditAnywhere)
	double LastSimulatedTime = 0;

	UPROPERTY(EditAnywhere)
	FKSIPersistentIntent Intent;

	UPROPERTY(EditAnywhere)
	TMap<FGameplayTag, FKSIAgentFeatureState> Features;
};

struct KSIAGENT_API FKSIAgentHandle
{
	FGuid Id;
	uint64 Generation = 0;
};

// All channels of this initial adapter share one lease. It is invalidated on handoff.
struct KSIAGENT_API FKSIAgentLease
{
	FKSIAgentHandle Handle;
	FGuid Writer;
	uint64 Epoch = 0;
};

struct KSIAGENT_API FKSIFeatureRegistration
{
	FGameplayTag Tag;
	UScriptStruct* Type = nullptr;
	int32 Version = 1;
	FGameplayTagContainer Dependencies;
	TFunction<bool(int32, FInstancedStruct&)> Migrate;
	TFunction<bool(const FInstancedStruct&)> Validate;
};

class KSIAGENT_API FKSIFeatureRegistry
{
public:
	bool Register(FKSIFeatureRegistration Registration);
	bool UpgradeAndValidate(FKSIAgentRecord& Record, FString& Error) const;
	const FKSIFeatureRegistration* Find(FGameplayTag Tag) const { return Entries.Find(Tag); }

private:
	TMap<FGameplayTag, FKSIFeatureRegistration> Entries;
};

// Game-thread owner. Borrowed pointers must not survive a mutation of this store.
class KSIAGENT_API FKSIAgentStore
{
public:
	FKSIAgentHandle Add(FKSIAgentRecord Record, const FKSIFeatureRegistry& Registry, FString& Error);
	FKSIAgentLease Acquire(FKSIAgentHandle Handle, FGuid Writer);
	bool Commit(const FKSIAgentLease& Lease, FKSIAgentRecord Record, const FKSIFeatureRegistry& Registry, FString& Error);
	bool CommitBatch(TArray<TPair<FKSIAgentLease, FKSIAgentRecord>> Updates, const FKSIFeatureRegistry& Registry, FString& Error);
	bool Release(const FKSIAgentLease& Lease);
	bool Replace(TArray<FKSIAgentRecord> Records, const FKSIFeatureRegistry& Registry, FString& Error);
	bool Snapshot(TArray<FKSIAgentRecord>& OutRecords) const;
	const FKSIAgentRecord* Find(FKSIAgentHandle Handle) const;
	FKSIAgentHandle GetHandle(FGuid Id) const;

private:
	struct FEntry
	{
		FKSIAgentRecord Record;
		uint64 Generation = 0;
		FGuid Writer;
		uint64 Epoch = 0;
	};

	TMap<FGuid, FEntry> Records;
	uint64 NextGeneration = 1;
	uint64 NextEpoch = 1;
};
