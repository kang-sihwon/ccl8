#pragma once

#include "CoreMinimal.h"
#include "CCLAgentTypes.h"
#include "CCLAgentFeatures.generated.h"

USTRUCT(BlueprintType)
struct CCL_API FCCLAgentTraits
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	TMap<FGameplayTag, float> Axes;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLSocialLink
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FGuid OtherAgentId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FGameplayTag LinkType;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLSkillProgress
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	float Proficiency = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	float Experience = 0;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLLifeGoalParameters
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	int64 TargetAmount = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FGuid SubjectId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FGameplayTag Resource;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLLifeGoalState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FGuid GoalId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FGameplayTag GoalTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FPrimaryAssetId DefinitionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FCCLTargetReference Beneficiary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FInstancedStruct Parameters;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	float Commitment = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	double CreatedTime = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	double Deadline = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FGameplayTag Status;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLLifeState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FGameplayTag Occupation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FGameplayTagContainer Roles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FCCLLocationReference Residence;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FCCLLocationReference Workplace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	TArray<FCCLSocialLink> SocialLinks;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	TMap<FGameplayTag, FCCLSkillProgress> Skills;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	TArray<FCCLLifeGoalState> LifeGoals;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLAgentResourceLinks
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FGuid Account;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FGuid Inventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	TArray<FGuid> Ownerships;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	TArray<FGuid> ObligationIds;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLRelationship
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	float Familiarity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	float Trust = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	float Affection = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	float Fear = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	float Resentment = 0;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLObservation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FGuid EventId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FGuid EvidenceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FCCLTargetReference PerceivedSubject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FGameplayTag EventType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	float Confidence = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	double Time = 0;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLMemory
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FGuid MemoryId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FCCLObservation Observation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	float Importance = 0.5f;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLBelief
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FCCLTargetReference Subject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	FGameplayTag Predicate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	float Confidence = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	TArray<FGuid> EvidenceIds;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLAgentExperience
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	TArray<FCCLMemory> Memories;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	TArray<FCCLBelief> Beliefs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	TMap<FGuid, FCCLRelationship> Relationships;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	TArray<FGuid> KnownOpportunities;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLEmotionState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	float Intensity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	float Baseline = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	double HalfLifeSeconds = 300;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLAgentNeeds
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	TMap<FGameplayTag, float> Urgency;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Agent")
	TMap<FGameplayTag, FCCLEmotionState> Emotions;
};

namespace CCLAgentFeatures
{
CCL_API void Register(FCCLFeatureRegistry& Registry);
CCL_API float Trait(const FCCLAgentTraits& Traits, FGameplayTag Axis);
CCL_API bool Observe(FCCLAgentExperience& Experience, const FCCLObservation& Observation);
CCL_API void AdvanceNeeds(FCCLAgentNeeds& Needs, double ElapsedSeconds);
}
