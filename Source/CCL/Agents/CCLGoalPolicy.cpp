#include "CCLGoalPolicy.h"
#include "CCLAgentTags.h"
namespace
{
template<class T> const T* Feature(const FCCLAgentRecord& Agent, FGameplayTag Tag)
{
    const auto* State = Agent.Features.Find(Tag);
    return State ? State->Data.GetPtr<T>() : nullptr;
}
}

float UCCLGoalProgressEvaluator::Evaluate(const FCCLAgentRecord& Agent, const FCCLLifeGoalState& Goal,
    const FCCLEconomyState& Economy) const
{
    return CCLGoalPolicy::Evaluate(Metric, Skill, Agent, Goal, Economy);
}

float CCLGoalPolicy::Evaluate(ECCLGoalMetric Metric, FGameplayTag SkillTag, const FCCLAgentRecord& Agent,
    const FCCLLifeGoalState& Goal, const FCCLEconomyState& Economy)
{
	const auto* Parameters = Goal.Parameters.GetPtr<FCCLLifeGoalParameters>();
	const auto* Resources = Feature<FCCLAgentResourceLinks>(Agent, CCLAgentTags::Feature_Resources);
	if (!Parameters || !Resources || Parameters->TargetAmount <= 0)
	{
		return 0;
	}

	double Progress = 0;
	if (Metric == ECCLGoalMetric::Balance)
	{
		const auto* Account = Economy.Accounts.Find(Resources->Account);
		Progress = Account ? double(Account->Balance) / Parameters->TargetAmount : 0;
	}
	else if (Metric == ECCLGoalMetric::RepaidDebt)
	{
		const auto* Debt = Economy.Obligations.Find(Parameters->SubjectId);
		Progress = Debt && Debt->OriginalAmount > 0 ? 1 - double(Debt->RemainingAmount) / Debt->OriginalAmount : 0;
	}
	else if (Metric == ECCLGoalMetric::FacilityLevel)
	{
		const auto* Ownership = Economy.Ownerships.Find(Parameters->SubjectId);
		Progress = Ownership ? double(Ownership->ImprovementLevel) / Parameters->TargetAmount : 0;
	}
	else if (Metric == ECCLGoalMetric::SkillExperience)
	{
		const auto* Life = Feature<FCCLLifeState>(Agent, CCLAgentTags::Feature_Life);
		const auto* Skill = Life ? Life->Skills.Find(SkillTag) : nullptr;
		Progress = Skill ? Skill->Experience / Parameters->TargetAmount : 0;
	}
	else if (Metric == ECCLGoalMetric::DeliveredResources)
	{
		int64 Delivered = 0;
		for (const auto& Receipt : Economy.Journal)
		{
			if (Receipt.bSucceeded && Receipt.Request.Reason == CCLAgentTags::Help)
			{
				for (const auto& Item : Receipt.Request.Items)
				{
					const auto* Recipient = Economy.Inventories.Find(Item.Destination);
					if (Item.Source == Resources->Inventory && Recipient && Recipient->OwnerId == Goal.Beneficiary.Id &&
                        (!Parameters->Resource.IsValid() || Parameters->Resource == Item.Resource))
					{
						Delivered += Item.Quantity;
					}
				}
			}
		}

		Progress = double(Delivered) / Parameters->TargetAmount;
	}

	return FMath::Clamp(Progress, 0., 1.);
}
