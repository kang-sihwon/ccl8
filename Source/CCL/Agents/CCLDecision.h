#pragma once

#include "CoreMinimal.h"
#include "CCLAgentFeatures.h"

struct CCL_API FCCLDecisionCandidate
{
	FGuid OpportunityId;
	FGameplayTag Activity;
	FCCLTargetReference Target;
	FGuid SupportingLifeGoalId;
	TMap<FGameplayTag, float> TraitSignals;
	TMap<FGameplayTag, float> NeedRelief;
	float BaseUtility = 0;
	float GoalUtility = 0;
	float RelationshipUtility = 0;
	uint8 bFeasible = 1;
};

struct CCL_API FCCLScoreContribution
{
	FName Source;
	float Value = 0;
};

struct CCL_API FCCLDecisionTrace
{
	FGuid OpportunityId;
	FGameplayTag Activity;
	float Score = 0;
	TArray<FCCLScoreContribution> Contributions;
	uint8 bUrgent = 0;
};

struct CCL_API FCCLDecisionInput
{
	FCCLAgentTraits Traits;
	FCCLAgentNeeds Needs;
	FCCLPersistentIntent CurrentIntent;
	TArray<FCCLDecisionCandidate> Candidates;
	double Time = 0;
	double MinimumCommitment = 60;
	float SwitchMargin = 0.1f;
};

struct CCL_API FCCLDecisionResult
{
	FCCLPersistentIntent Intent;
	TArray<FCCLDecisionTrace> Traces;
	uint8 bSelected = 0;
};

namespace CCLDecision
{
// No Actor, UObject, random stream, world query or mutable shared state.
CCL_API FCCLDecisionResult Evaluate(const FCCLDecisionInput& Input);
}
