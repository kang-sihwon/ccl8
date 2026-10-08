#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/SaveGame.h"
#include "CCLAgentFeatures.h"
#include "CCLEconomy.h"
#include "CCLDecision.h"
#include "CCLGoalPolicy.h"
#include "UObject/StrongObjectPtr.h"
#include "CCLLifeSimulation.generated.h"

USTRUCT()
struct CCL_API FCCLWorldOpportunity
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FGuid OpportunityId;

	UPROPERTY(EditAnywhere)
	FGameplayTag Activity;

	UPROPERTY(EditAnywhere)
	FGuid ProviderId;

	UPROPERTY(EditAnywhere)
	FCCLLocationReference Location;

	UPROPERTY(EditAnywhere)
	FGuid OwnershipId;

	UPROPERTY(EditAnywhere)
	FGuid ObligationId;

	UPROPERTY(EditAnywhere)
	FGuid DiscoveredOpportunityId;

	UPROPERTY(EditAnywhere)
	FGameplayTag Resource;

	UPROPERTY(EditAnywhere)
	int64 Price = 0;

	UPROPERTY(EditAnywhere)
	int64 Quantity = 1;

	UPROPERTY(EditAnywhere)
	double ExpireTime = 0;

	UPROPERTY(EditAnywhere)
	int32 Revision = 1;

	UPROPERTY(EditAnywhere)
	TMap<FGameplayTag, float> TraitSignals;

	UPROPERTY(EditAnywhere)
	TMap<FGameplayTag, float> NeedRelief;

	UPROPERTY(EditAnywhere)
	float BaseUtility = 0;

	UPROPERTY(EditAnywhere)
	uint8 bAvailable = 1;
};

USTRUCT()
struct CCL_API FCCLScheduledWorldEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FGuid EventId;

	UPROPERTY(EditAnywhere)
	double Time = 0;

	UPROPERTY(EditAnywhere)
	FGameplayTag Kind;

	UPROPERTY(EditAnywhere)
	FGuid OpportunityId;

	UPROPERTY(EditAnywhere)
	FGuid InventoryId;

	UPROPERTY(EditAnywhere)
	FGameplayTag Resource;

	UPROPERTY(EditAnywhere)
	int64 Quantity = 0;

	UPROPERTY(EditAnywhere)
	uint8 bApplied = 0;
};

USTRUCT()
struct CCL_API FCCLLifeExecutionReceipt
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid RequestId;

	UPROPERTY()
	FGuid AgentId;

	UPROPERTY()
	FGuid OpportunityId;

	UPROPERTY()
	int32 Revision = 0;

	UPROPERTY()
	uint8 bSucceeded = 0;

	UPROPERTY()
	FString Failure;
};

USTRUCT()
struct CCL_API FCCLSimulationSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	int32 Version = 1;

	UPROPERTY(EditAnywhere)
	int32 Seed = 42;

	UPROPERTY(EditAnywhere)
	double Time = 0;

	UPROPERTY(EditAnywhere)
	int64 Sequence = 0;

	UPROPERTY(EditAnywhere)
	TArray<FCCLAgentRecord> Agents;

	UPROPERTY(EditAnywhere)
	FCCLEconomyState Economy;

	UPROPERTY(EditAnywhere)
	TArray<FCCLWorldOpportunity> Opportunities;

	UPROPERTY(EditAnywhere)
	TArray<FCCLScheduledWorldEvent> Events;

	UPROPERTY()
	TArray<FCCLLifeExecutionReceipt> Results;
};

UCLASS()
class CCL_API UCCLPopulationScenario : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FCCLSimulationSnapshot InitialState;
};

UCLASS()
class CCL_API UCCLLifeSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FCCLSimulationSnapshot State;
};

struct CCL_API FCCLLifeExecutionResult
{
	FGuid RequestId;
	uint8 bSucceeded = 0;
	FString Failure;
};

class CCL_API FCCLLifeSimulation
{
public:
	FCCLLifeSimulation();
	bool RegisterGoal(UCCLLifeGoalDefinition* Definition);
	bool Initialize(const FCCLSimulationSnapshot& Snapshot, FString& Error);
	bool Capture(FCCLSimulationSnapshot& Snapshot) const;
	bool Save(TArray<uint8>& Bytes) const;
	bool Load(const TArray<uint8>& Bytes, FString& Error);
	void AdvanceTo(double TargetTime);
	void SetActorActive(FGuid Id, bool bActive);
	bool SelectIntent(FGuid Id, const FCCLPersistentIntent& Intent);
	bool UpdateLocation(FGuid Id, FVector Position);
	bool OpenAccount(FGuid Id, int64 InitialBalance);
	bool ImportAccountBalance(FGuid Id, int64 Balance);
	FCCLEconomyState& GetServerEconomy() { return Economy; }
	FCCLDecisionResult Decide(FGuid AgentId) const;
	FCCLLifeExecutionResult Execute(FGuid AgentId, FGuid OpportunityId, int32 ExpectedRevision, FGuid RequestId);
	float GoalProgress(const FCCLAgentRecord& Agent, const FCCLLifeGoalState& Goal) const;
	const FCCLAgentRecord* Find(FGuid Id) const { return Agents.Find(Agents.GetHandle(Id)); }
	const FCCLWorldOpportunity* FindOpportunity(FGuid Id) const;
	const FCCLEconomyState& GetEconomy() const { return Economy; }
	double GetTime() const { return Time; }
	const TArray<FCCLDecisionTrace>& GetLastTrace(FGuid Id) const;
	FCCLAgentStore& GetAgents() { return Agents; }
	const FCCLFeatureRegistry& GetRegistry() const { return Registry; }
	FString DailyReport() const;
	static FCCLSimulationSnapshot MerchantScenario(int32 Seed);

private:
	bool BuildTransaction(const FCCLAgentRecord& Agent, const FCCLWorldOpportunity& Opportunity,
		FGuid RequestId, FCCLTransactionRequest& Request, FString& Error) const;
	void ApplyEvents();
	void ApplyOutcome(FCCLAgentRecord& Agent, const FCCLWorldOpportunity& Opportunity, FGuid RequestId);
	FGuid NextId();

private:
	TMap<FGameplayTag, TStrongObjectPtr<UCCLLifeGoalDefinition>> GoalDefinitions;
	FCCLFeatureRegistry Registry;
	FCCLAgentStore Agents;
	FCCLEconomyState Economy;
	TArray<FCCLWorldOpportunity> Opportunities;
	TArray<FCCLScheduledWorldEvent> Events;
	TArray<FCCLLifeExecutionReceipt> Results;
	TMap<FGuid, TArray<FCCLDecisionTrace>> Traces;
	TSet<FGuid> ActiveActors;
	int32 Seed = 42;
	double Time = 0;
	int64 Sequence = 0;
};
