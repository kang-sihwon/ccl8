#include "CCLAgentFeatures.h"

#include "CCLAgentTags.h"

namespace
{
bool Unit(float Value)
{
	return FMath::IsFinite(Value) && Value >= 0 && Value <= 1;
}

bool ValidTraits(const FInstancedStruct& Data)
{
	const auto& Traits = Data.Get<FCCLAgentTraits>();
	const TArray<FGameplayTag> Axes = {CCLAgentTags::RiskTolerance, CCLAgentTags::Aggressiveness,
		CCLAgentTags::Curiosity, CCLAgentTags::Sociability, CCLAgentTags::SelfControl, CCLAgentTags::Empathy,
		CCLAgentTags::MaterialGain, CCLAgentTags::Duty, CCLAgentTags::OthersWelfare};
	for (const auto& Pair : Traits.Axes)
	{
		if (!Axes.Contains(Pair.Key) || !Unit(Pair.Value))
		{
			return false;
		}
	}

	return true;
}

bool ValidLife(const FInstancedStruct& Data)
{
	const auto& Life = Data.Get<FCCLLifeState>();
	if (Life.LifeGoals.Num() > 32 || Life.SocialLinks.Num() > 128 || Life.Skills.Num() > 128 ||
		Life.Residence.Position.ContainsNaN() || Life.Workplace.Position.ContainsNaN())
	{
		return false;
	}

	TSet<FGuid> Ids;
	for (const auto& Goal : Life.LifeGoals)
	{
		const auto* Parameters = Goal.Parameters.GetPtr<FCCLLifeGoalParameters>();
		if (!Goal.GoalId.IsValid() || Ids.Contains(Goal.GoalId) || !Goal.GoalTag.IsValid() ||
			!Unit(Goal.Commitment) || !FMath::IsFinite(Goal.CreatedTime) || Goal.CreatedTime < 0 ||
			!FMath::IsFinite(Goal.Deadline) || (Goal.Deadline != 0 && Goal.Deadline < Goal.CreatedTime) ||
			!Parameters || Parameters->TargetAmount <= 0)
		{
			return false;
		}

		Ids.Add(Goal.GoalId);
	}

	for (const auto& Link : Life.SocialLinks)
	{
		if (!Link.OtherAgentId.IsValid() || !Link.LinkType.IsValid())
		{
			return false;
		}
	}

	for (const auto& Pair : Life.Skills)
	{
		if (!Pair.Key.IsValid() || !Unit(Pair.Value.Proficiency) ||
			!FMath::IsFinite(Pair.Value.Experience) || Pair.Value.Experience < 0)
		{
			return false;
		}
	}

	return true;
}

bool ValidExperience(const FInstancedStruct& Data)
{
	const auto& Experience = Data.Get<FCCLAgentExperience>();
	if (Experience.Memories.Num() > 256 || Experience.Beliefs.Num() > 256 ||
		Experience.Relationships.Num() > 256 || Experience.KnownOpportunities.Num() > 512)
	{
		return false;
	}

	for (const auto& Pair : Experience.Relationships)
	{
		const auto& R = Pair.Value;
		if (!Pair.Key.IsValid() || !Unit(R.Familiarity) || !Unit(R.Trust) || !Unit(R.Affection) ||
			!Unit(R.Fear) || !Unit(R.Resentment))
		{
			return false;
		}
	}

	TSet<FGuid> Evidence;
	for (const auto& Memory : Experience.Memories)
	{
		const auto& O = Memory.Observation;
		if (!Memory.MemoryId.IsValid() || !O.EventId.IsValid() || !O.EvidenceId.IsValid() ||
			Evidence.Contains(O.EvidenceId) || !Unit(Memory.Importance) || !Unit(O.Confidence) ||
			!FMath::IsFinite(O.Time) || O.Time < 0 ||
			(O.PerceivedSubject.Kind == CCLAgentTags::Unknown && O.PerceivedSubject.Id.IsValid()))
		{
			return false;
		}

		Evidence.Add(O.EvidenceId);
	}

	for (const auto& Belief : Experience.Beliefs)
	{
		if (!Belief.Predicate.IsValid() || !Unit(Belief.Confidence) || Belief.EvidenceIds.Num() > 256 ||
			(Belief.Subject.Kind == CCLAgentTags::Unknown && Belief.Subject.Id.IsValid()))
		{
			return false;
		}
	}

	return true;
}

bool ValidNeeds(const FInstancedStruct& Data)
{
	const auto& Needs = Data.Get<FCCLAgentNeeds>();
	for (const auto& Pair : Needs.Urgency)
	{
		if (!Pair.Key.IsValid() || !Unit(Pair.Value))
		{
			return false;
		}
	}

	for (const auto& Pair : Needs.Emotions)
	{
		if (!Pair.Key.IsValid() || !Unit(Pair.Value.Intensity) || !Unit(Pair.Value.Baseline) ||
			!FMath::IsFinite(Pair.Value.HalfLifeSeconds) || Pair.Value.HalfLifeSeconds <= 0)
		{
			return false;
		}
	}

	return Needs.Urgency.Num() <= 32 && Needs.Emotions.Num() <= 32;
}
}

void CCLAgentFeatures::Register(FCCLFeatureRegistry& Registry)
{
	Registry.Register({CCLAgentTags::Feature_Traits, FCCLAgentTraits::StaticStruct(), 1, {}, {}, ValidTraits});
	Registry.Register({CCLAgentTags::Feature_Life, FCCLLifeState::StaticStruct(), 1, {}, {}, ValidLife});
	Registry.Register({CCLAgentTags::Feature_Experience, FCCLAgentExperience::StaticStruct(), 1, {}, {}, ValidExperience});
	Registry.Register({CCLAgentTags::Feature_Needs, FCCLAgentNeeds::StaticStruct(), 1, {}, {}, ValidNeeds});
	Registry.Register({CCLAgentTags::Feature_Resources, FCCLAgentResourceLinks::StaticStruct(), 1, {}, {},
		[](const FInstancedStruct& Data)
		{
			const auto& Links = Data.Get<FCCLAgentResourceLinks>();
			return Links.Account.IsValid() && Links.Inventory.IsValid() &&
				Links.Ownerships.Num() <= 128 && Links.ObligationIds.Num() <= 128;
		}});
}

float CCLAgentFeatures::Trait(const FCCLAgentTraits& Traits, FGameplayTag Axis)
{
	const auto* Value = Traits.Axes.Find(Axis);
	return Value ? *Value : 0.5f;
}

bool CCLAgentFeatures::Observe(FCCLAgentExperience& Experience, const FCCLObservation& Observation)
{
	if (!Observation.EventId.IsValid() || !Observation.EvidenceId.IsValid() || !Observation.EventType.IsValid() ||
		!Unit(Observation.Confidence) || !FMath::IsFinite(Observation.Time) || Observation.Time < 0 ||
		(Observation.PerceivedSubject.Kind == CCLAgentTags::Unknown && Observation.PerceivedSubject.Id.IsValid()))
	{
		return false;
	}

	for (const auto& Belief : Experience.Beliefs)
	{
		if (Belief.EvidenceIds.Contains(Observation.EvidenceId))
		{
			return false;
		}
	}

	auto* Belief = Experience.Beliefs.FindByPredicate([&](const FCCLBelief& B)
	{
		return B.Subject.Kind == Observation.PerceivedSubject.Kind &&
			B.Subject.Id == Observation.PerceivedSubject.Id && B.Predicate == Observation.EventType;
	});
	if ((!Belief && Experience.Beliefs.Num() >= 256) || (Belief && Belief->EvidenceIds.Num() >= 256))
	{
		return false;
	}

	if (!Belief)
	{
		Belief = &Experience.Beliefs.AddDefaulted_GetRef();
		Belief->Subject = Observation.PerceivedSubject;
		Belief->Predicate = Observation.EventType;
	}

	Belief->EvidenceIds.Add(Observation.EvidenceId);
	Belief->Confidence = FMath::Max(Belief->Confidence, Observation.Confidence);
	if (Experience.Memories.Num() == 256)
	{
		Experience.Memories.RemoveAt(0);
	}

	auto& Memory = Experience.Memories.AddDefaulted_GetRef();
	Memory.MemoryId = Observation.EvidenceId;
	Memory.Observation = Observation;
	Memory.Importance = Observation.Confidence;
	if (Observation.PerceivedSubject.Kind == CCLAgentTags::Agent && Observation.PerceivedSubject.Id.IsValid())
	{
		if (!Experience.Relationships.Contains(Observation.PerceivedSubject.Id) && Experience.Relationships.Num() >= 256)
		{
			return true;
		}

		auto& Relationship = Experience.Relationships.FindOrAdd(Observation.PerceivedSubject.Id);
		const float Strength = 0.1f * Observation.Confidence;
		Relationship.Familiarity = FMath::Min(1.f, Relationship.Familiarity + Strength);
		if (Observation.EventType == CCLAgentTags::Success || Observation.EventType == CCLAgentTags::Aid)
		{
			Relationship.Trust = FMath::Min(1.f, Relationship.Trust + Strength);
			Relationship.Affection = FMath::Min(1.f, Relationship.Affection + Strength);
		}
		else if (Observation.EventType == CCLAgentTags::Failure)
		{
			Relationship.Trust = FMath::Max(0.f, Relationship.Trust - Strength);
			Relationship.Resentment = FMath::Min(1.f, Relationship.Resentment + Strength);
		}
	}

	return true;
}

void CCLAgentFeatures::AdvanceNeeds(FCCLAgentNeeds& Needs, double ElapsedSeconds)
{
	if (!FMath::IsFinite(ElapsedSeconds) || ElapsedSeconds <= 0)
	{
		return;
	}

	for (auto& Pair : Needs.Urgency)
	{
		const double Rate = Pair.Key == CCLAgentTags::Hunger ? 1. / 43200. :
			Pair.Key == CCLAgentTags::Fatigue ? 1. / 86400. : Pair.Key == CCLAgentTags::Social ? 1. / 172800. : 0;
		Pair.Value = FMath::Clamp(Pair.Value + Rate * ElapsedSeconds, 0., 1.);
	}

	for (auto& Pair : Needs.Emotions)
	{
		auto& Emotion = Pair.Value;
		Emotion.Intensity = Emotion.Baseline + (Emotion.Intensity - Emotion.Baseline) *
			FMath::Pow(0.5, ElapsedSeconds / Emotion.HalfLifeSeconds);
	}
}
