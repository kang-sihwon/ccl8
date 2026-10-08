#include "CCLDecision.h"
#include "CCLAgentTags.h"

FCCLDecisionResult CCLDecision::Evaluate(const FCCLDecisionInput& Input)
{
	FCCLDecisionResult Result;
	if (!FMath::IsFinite(Input.Time) || Input.Time < 0)
	{
		return Result;
	}

	TArray<FCCLDecisionCandidate> Candidates = Input.Candidates;
	Candidates.Sort([](const FCCLDecisionCandidate& A, const FCCLDecisionCandidate& B)
	{
		return A.OpportunityId < B.OpportunityId;
	});
	int32 Winner = INDEX_NONE;
	int32 Current = INDEX_NONE;
	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		const auto& Candidate = Candidates[Index];
		FCCLDecisionTrace Trace;
		Trace.OpportunityId = Candidate.OpportunityId;
		Trace.Activity = Candidate.Activity;
		auto Add = [&](FName Name, float Value)
		{
			Trace.Contributions.Add({Name, Value});
			Trace.Score += Value;
		};
		Add(TEXT("Base"), Candidate.BaseUtility);
		Add(TEXT("LifeGoal"), Candidate.GoalUtility);
		Add(TEXT("Relationship"), Candidate.RelationshipUtility);
		if (const auto* Fear = Input.Needs.Emotions.Find(CCLAgentTags::Fear))
		{
			Add(TEXT("Fear"), -Fear->Intensity * (1 - CCLAgentFeatures::Trait(Input.Traits, CCLAgentTags::RiskTolerance)) *
				FMath::Max(0.f, Candidate.TraitSignals.FindRef(CCLAgentTags::RiskTolerance)));
		}

		if (const auto* Anger = Input.Needs.Emotions.Find(CCLAgentTags::Anger))
		{
			Add(TEXT("Anger"), Anger->Intensity * (1 - CCLAgentFeatures::Trait(Input.Traits, CCLAgentTags::SelfControl)) *
				Candidate.TraitSignals.FindRef(CCLAgentTags::Aggressiveness));
		}
		TArray<FGameplayTag> Axes;
		Candidate.TraitSignals.GetKeys(Axes);
		Axes.Sort([](FGameplayTag A, FGameplayTag B) { return A.ToString() < B.ToString(); });
		for (const auto Axis : Axes)
		{
			Add(Axis.GetTagName(), CCLAgentFeatures::Trait(Input.Traits, Axis) * Candidate.TraitSignals[Axis]);
		}

		TArray<FGameplayTag> Needs;
		Candidate.NeedRelief.GetKeys(Needs);
		Needs.Sort([](FGameplayTag A, FGameplayTag B) { return A.ToString() < B.ToString(); });
		for (const auto Need : Needs)
		{
			const float Urgency = Input.Needs.Urgency.FindRef(Need);
			const float Relief = Candidate.NeedRelief[Need];
			Add(Need.GetTagName(), Urgency * Urgency * Relief * 4);
			Trace.bUrgent |= Urgency >= 0.9f && Relief > 0;
		}

		Result.Traces.Add(Trace);
		if (!Candidate.bFeasible || !Candidate.Activity.IsValid() || !Candidate.OpportunityId.IsValid() || !FMath::IsFinite(Trace.Score))
		{
			continue;
		}

		if (Winner == INDEX_NONE || Trace.bUrgent > Result.Traces[Winner].bUrgent ||
			(Trace.bUrgent == Result.Traces[Winner].bUrgent && Trace.Score > Result.Traces[Winner].Score))
		{
			Winner = Index;
		}

		if (Candidate.OpportunityId == Input.CurrentIntent.OpportunityId && Input.CurrentIntent.ExpireTime > Input.Time)
		{
			Current = Index;
		}
	}

	if (Winner == INDEX_NONE)
	{
		return Result;
	}

	if (Current != INDEX_NONE && !Result.Traces[Winner].bUrgent &&
		(Input.Time - Input.CurrentIntent.StartedTime < Input.MinimumCommitment ||
		 Result.Traces[Winner].Score - Result.Traces[Current].Score < Input.SwitchMargin))
	{
		Winner = Current;
	}

	const auto& Selected = Candidates[Winner];
	Result.Intent.Activity = Selected.Activity;
	Result.Intent.Target = Selected.Target;
	Result.Intent.OpportunityId = Selected.OpportunityId;
	Result.Intent.SupportingLifeGoalId = Selected.SupportingLifeGoalId;
	Result.Intent.StartedTime = Winner == Current ? Input.CurrentIntent.StartedTime : Input.Time;
	Result.Intent.ExpireTime = Input.Time + 3600;
	Result.bSelected = 1;
	return Result;
}
