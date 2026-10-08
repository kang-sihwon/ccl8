#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CCLAgentFeatures.h"
#include "CCLEconomy.h"
#include "CCLGoalPolicy.generated.h"

UENUM()
enum class ECCLGoalMetric : uint8 { Balance, RepaidDebt, DeliveredResources, FacilityLevel, SkillExperience };

// No Actor or world access: policies can also run in the reduced and Mass adapters.
UCLASS(EditInlineNew, DefaultToInstanced)
class CCL_API UCCLGoalProgressEvaluator : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere)
    ECCLGoalMetric Metric = ECCLGoalMetric::Balance;
    UPROPERTY(EditAnywhere)
    FGameplayTag Skill;
    virtual float Evaluate(const FCCLAgentRecord& Agent, const FCCLLifeGoalState& Goal,
        const FCCLEconomyState& Economy) const;
};

UCLASS(EditInlineNew, DefaultToInstanced)
class CCL_API UCCLGoalCompletionPolicy : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere)
    uint8 bMaintenance = 0;
    virtual bool IsComplete(float Progress) const { return !bMaintenance && Progress >= 1; }
};

UCLASS()
class CCL_API UCCLLifeGoalDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere)
    FGameplayTag GoalTag;
    UPROPERTY(EditAnywhere, Instanced)
    TObjectPtr<UCCLGoalProgressEvaluator> ProgressEvaluator;
    UPROPERTY(EditAnywhere, Instanced)
    TObjectPtr<UCCLGoalCompletionPolicy> CompletionPolicy;
    UPROPERTY(EditAnywhere)
    FGameplayTagContainer SupportingActivities;
    UPROPERTY(EditAnywhere)
    uint8 bMatchBeneficiary = 0;
    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(TEXT("LifeGoal"), GoalTag.GetTagName());
    }
};

namespace CCLGoalPolicy
{
CCL_API float Evaluate(ECCLGoalMetric Metric, FGameplayTag Skill, const FCCLAgentRecord& Agent,
    const FCCLLifeGoalState& Goal, const FCCLEconomyState& Economy);
}
