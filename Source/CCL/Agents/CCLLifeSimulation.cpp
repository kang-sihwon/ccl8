#include "CCLLifeSimulation.h"

#include "CCLAgentTags.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"

namespace
{
template <class T> const T* Feature(const FCCLAgentRecord& Agent, FGameplayTag Tag)
{
	const auto* State = Agent.Features.Find(Tag);
	return State ? State->Data.GetPtr<T>() : nullptr;
}

template <class T> T* MutableFeature(FCCLAgentRecord& Agent, FGameplayTag Tag)
{
	auto* State = Agent.Features.Find(Tag);
	return State ? State->Data.GetMutablePtr<T>() : nullptr;
}


}

FCCLLifeSimulation::FCCLLifeSimulation()
{
	CCLAgentFeatures::Register(Registry);
	const TArray<FGameplayTag> Tags = {CCLAgentTags::Goal_Living, CCLAgentTags::Goal_Debt,
		CCLAgentTags::Goal_Support, CCLAgentTags::Goal_Business, CCLAgentTags::Goal_Mastery};
	const ECCLGoalMetric Metrics[] = {ECCLGoalMetric::Balance, ECCLGoalMetric::RepaidDebt,
		ECCLGoalMetric::DeliveredResources, ECCLGoalMetric::FacilityLevel, ECCLGoalMetric::SkillExperience};
	for (int32 Index = 0; Index < Tags.Num(); ++Index)
	{
		auto* Definition = NewObject<UCCLLifeGoalDefinition>();
		Definition->GoalTag = Tags[Index];
		Definition->ProgressEvaluator = NewObject<UCCLGoalProgressEvaluator>(Definition);
		Definition->ProgressEvaluator->Metric = Metrics[Index];
		Definition->ProgressEvaluator->Skill = CCLAgentTags::Work;
		Definition->CompletionPolicy = NewObject<UCCLGoalCompletionPolicy>(Definition);
		Definition->CompletionPolicy->bMaintenance = Index == 0;
		Definition->bMatchBeneficiary = Index == 2;
		Definition->SupportingActivities.AddTag(Index == 2 ? CCLAgentTags::Help : CCLAgentTags::Work);
		if (Index == 1 || Index == 3)
		{
			Definition->SupportingActivities.AddTag(Index == 1 ? CCLAgentTags::Repay : CCLAgentTags::Improve);
		}
		RegisterGoal(Definition);
	}
}

bool FCCLLifeSimulation::OpenAccount(FGuid Id, int64 InitialBalance)
{
	if (!Id.IsValid() || InitialBalance < 0 || InitialBalance > 1000000)
	{
		return false;
	}

	if (!Economy.Accounts.Contains(Id))
	{
		FCCLAccountRecord Account;
		Account.Id = Id;
		Account.OwnerId = Id;
		Account.Balance = InitialBalance;
		Account.OpeningBalance = InitialBalance;
		Account.bHasOpeningBalance = 1;
		Economy.Accounts.Add(Id, Account);
	}

	return true;
}

bool FCCLLifeSimulation::ImportAccountBalance(FGuid Id, int64 Balance)
{
	if (!OpenAccount(Id, Balance))
	{
		return false;
	}

	Economy.Accounts[Id].OpeningBalance += Balance - Economy.Accounts[Id].Balance;
	Economy.Accounts[Id].Balance = Balance;
	return true;
}

bool FCCLLifeSimulation::Initialize(const FCCLSimulationSnapshot& InputSnapshot, FString& Error)
{
	if (!CCLEconomy::Validate(InputSnapshot.Economy, Error))
	{
		return false;
	}
	FCCLSimulationSnapshot Snapshot = InputSnapshot;
	// Optional field migration for snapshots produced before account-opening audit metadata.
	for (auto& Pair : Snapshot.Economy.Accounts)
	{
		auto& Account = Pair.Value;
		if (!Account.bHasOpeningBalance)
		{
			Account.OpeningBalance = Account.Balance;
			for (const auto& Receipt : Snapshot.Economy.Journal)
			{
				if (Receipt.bSucceeded)
				{
					Account.OpeningBalance += Receipt.Request.Buyer == Account.Id ? Receipt.Request.Price : 0;
					Account.OpeningBalance -= Receipt.Request.Seller == Account.Id ? Receipt.Request.Price : 0;
				}
			}
			Account.bHasOpeningBalance = 1;
		}
	}
	if (Snapshot.Version != 1 || !FMath::IsFinite(Snapshot.Time) || Snapshot.Time < 0 || Snapshot.Sequence < 0 ||
		Snapshot.Agents.Num() > 10000 || Snapshot.Opportunities.Num() > 10000 || !CCLEconomy::Validate(Snapshot.Economy, Error))
	{
		Error = TEXT("Invalid simulation snapshot.");
		return false;
	}

	TSet<FGuid> AgentIds;
	for (const auto& Agent : Snapshot.Agents)
	{
		if (AgentIds.Contains(Agent.Id) || Agent.LastSimulatedTime > Snapshot.Time)
		{
			Error = TEXT("Duplicate agent or future simulation time.");
			return false;
		}

		AgentIds.Add(Agent.Id);
	}

	TSet<FGuid> PlaceIds;
	for (const auto& Pair : Snapshot.Economy.Ownerships)
	{
		const auto& Ownership = Pair.Value;
		if (!Pair.Key.IsValid() || Pair.Key != Ownership.Id || !Ownership.PlaceId.IsValid() ||
			!AgentIds.Contains(Ownership.OwnerId) || Ownership.ImprovementLevel < 0 || Ownership.ImprovementLevel > 3)
		{
			Error = TEXT("Invalid ownership.");
			return false;
		}

		for (FGuid User : Ownership.AuthorizedUsers)
		{
			if (!AgentIds.Contains(User))
			{
				Error = TEXT("Unknown authorized user.");
				return false;
			}
		}

		PlaceIds.Add(Ownership.PlaceId);
	}

	TSet<FGuid> OpportunityIds;
	for (const auto& Opportunity : Snapshot.Opportunities)
	{
		if (!Opportunity.OpportunityId.IsValid() || OpportunityIds.Contains(Opportunity.OpportunityId) ||
			!AgentIds.Contains(Opportunity.ProviderId) || !Opportunity.Activity.IsValid() ||
			!PlaceIds.Contains(Opportunity.Location.PlaceId) || Opportunity.Location.Position.ContainsNaN() ||
			!FMath::IsFinite(Opportunity.ExpireTime) || Opportunity.ExpireTime < 0 || Opportunity.Price < 0 ||
			Opportunity.Price > 1000000000 || Opportunity.Quantity < 1 || Opportunity.Quantity > 1000 || Opportunity.Revision < 1)
		{
			Error = TEXT("Invalid world opportunity.");
			return false;
		}

		OpportunityIds.Add(Opportunity.OpportunityId);
	}

	for (const auto& Agent : Snapshot.Agents)
	{
		const auto* Life = Feature<FCCLLifeState>(Agent, CCLAgentTags::Feature_Life);
		const auto* Resources = Feature<FCCLAgentResourceLinks>(Agent, CCLAgentTags::Feature_Resources);
		const auto* Experience = Feature<FCCLAgentExperience>(Agent, CCLAgentTags::Feature_Experience);
		if ((Agent.Intent.OpportunityId.IsValid() && !OpportunityIds.Contains(Agent.Intent.OpportunityId)) ||
			(Agent.Intent.Target.Kind == CCLAgentTags::Agent && !AgentIds.Contains(Agent.Intent.Target.Id)) ||
			(Agent.Intent.SupportingLifeGoalId.IsValid() && (!Life || !Life->LifeGoals.ContainsByPredicate(
				[&](const FCCLLifeGoalState& Goal) { return Goal.GoalId == Agent.Intent.SupportingLifeGoalId; }))))
		{
			Error = TEXT("Unresolved persistent intent reference.");
			return false;
		}
		if (Life)
		{
			if (!PlaceIds.Contains(Life->Residence.PlaceId) || !PlaceIds.Contains(Life->Workplace.PlaceId))
			{
				Error = TEXT("Unknown home or workplace.");
				return false;
			}

			for (const auto& Goal : Life->LifeGoals)
			{
				const auto* Definition = GoalDefinitions.Find(Goal.GoalTag);
				const auto* Parameters = Goal.Parameters.GetPtr<FCCLLifeGoalParameters>();
				if (!Definition || Goal.DefinitionId != Definition->Get()->GetPrimaryAssetId() || !Parameters ||
					(Goal.Beneficiary.Kind == CCLAgentTags::Agent && !AgentIds.Contains(Goal.Beneficiary.Id)) ||
					(Goal.GoalTag == CCLAgentTags::Goal_Debt && !Snapshot.Economy.Obligations.Contains(Parameters->SubjectId)) ||
					(Goal.GoalTag == CCLAgentTags::Goal_Business && !Snapshot.Economy.Ownerships.Contains(Parameters->SubjectId)))
				{
					Error = TEXT("Unknown goal definition, beneficiary, or subject.");
					return false;
				}
			}

			for (const auto& Link : Life->SocialLinks)
			{
				if (!AgentIds.Contains(Link.OtherAgentId) || Link.OtherAgentId == Agent.Id)
				{
					Error = TEXT("Unknown or self social link.");
					return false;
				}
			}
		}

		if (Resources)
		{
			const auto* Account = Snapshot.Economy.Accounts.Find(Resources->Account);
			const auto* Inventory = Snapshot.Economy.Inventories.Find(Resources->Inventory);
			if (!Account || !Inventory || Account->OwnerId != Agent.Id || Inventory->OwnerId != Agent.Id)
			{
				Error = TEXT("Agent resource links do not own their accounts and inventories.");
				return false;
			}

			for (FGuid OwnershipId : Resources->Ownerships)
			{
				const auto* Ownership = Snapshot.Economy.Ownerships.Find(OwnershipId);
				if (!Ownership || Ownership->OwnerId != Agent.Id)
				{
					Error = TEXT("Invalid ownership link.");
					return false;
				}
			}

			for (FGuid DebtId : Resources->ObligationIds)
			{
				const auto* Debt = Snapshot.Economy.Obligations.Find(DebtId);
				if (!Debt || !AgentIds.Contains(Debt->DebtorId) || !AgentIds.Contains(Debt->CreditorId) ||
					(Debt->DebtorId != Agent.Id && Debt->CreditorId != Agent.Id))
				{
					Error = TEXT("Invalid obligation link.");
					return false;
				}
			}
		}

		if (Experience)
		{
			for (FGuid Known : Experience->KnownOpportunities)
			{
				if (!OpportunityIds.Contains(Known))
				{
					Error = TEXT("Unknown opportunity reference.");
					return false;
				}
			}

			for (const auto& Pair : Experience->Relationships)
			{
				if (!AgentIds.Contains(Pair.Key))
				{
					Error = TEXT("Unknown relationship target.");
					return false;
				}
			}
		}
	}

	TSet<FGuid> EventIds;
	for (const auto& Event : Snapshot.Events)
	{
		if (!Event.EventId.IsValid() || EventIds.Contains(Event.EventId) || !FMath::IsFinite(Event.Time) || Event.Time < 0 ||
			(Event.Kind != CCLAgentTags::Eat && Event.Kind != CCLAgentTags::Help && Event.Kind != CCLAgentTags::Failure) ||
			(Event.Kind == CCLAgentTags::Eat && (!Snapshot.Economy.Inventories.Contains(Event.InventoryId) ||
				!Event.Resource.IsValid() || Event.Quantity <= 0)) ||
			(Event.Kind != CCLAgentTags::Eat && !OpportunityIds.Contains(Event.OpportunityId)))
		{
			Error = TEXT("Invalid scheduled event.");
			return false;
		}
		EventIds.Add(Event.EventId);
	}

	for (const auto& Opportunity : Snapshot.Opportunities)
	{
		if ((Opportunity.OwnershipId.IsValid() && !Snapshot.Economy.Ownerships.Contains(Opportunity.OwnershipId)) ||
			(Opportunity.ObligationId.IsValid() && !Snapshot.Economy.Obligations.Contains(Opportunity.ObligationId)) ||
			(Opportunity.DiscoveredOpportunityId.IsValid() && !OpportunityIds.Contains(Opportunity.DiscoveredOpportunityId)) ||
			!FMath::IsFinite(Opportunity.BaseUtility))
		{
			Error = TEXT("Invalid opportunity references.");
			return false;
		}
		for (const auto& Pair : Opportunity.TraitSignals)
		{
			if (!Pair.Key.IsValid() || !FMath::IsFinite(Pair.Value) || FMath::Abs(Pair.Value) > 10)
			{
				Error = TEXT("Invalid opportunity evaluation signal.");
				return false;
			}
		}
		for (const auto& Pair : Opportunity.NeedRelief)
		{
			if (!Pair.Key.IsValid() || !FMath::IsFinite(Pair.Value) || FMath::Abs(Pair.Value) > 1)
			{
				Error = TEXT("Invalid opportunity outcome.");
				return false;
			}
		}
	}

	TSet<FGuid> ResultIds;
	for (const auto& Result : Snapshot.Results)
	{
		if (!Result.RequestId.IsValid() || ResultIds.Contains(Result.RequestId) || !AgentIds.Contains(Result.AgentId) ||
			!FMath::IsFinite(Result.Time) || Result.Time < 0 || Result.Time > Snapshot.Time || Result.DecisionExplanation.Len() > 8192)
		{
			Error = TEXT("Invalid execution receipts.");
			return false;
		}

		ResultIds.Add(Result.RequestId);
	}

	if (!Agents.Replace(Snapshot.Agents, Registry, Error))
	{
		return false;
	}

	Economy = Snapshot.Economy;
	Opportunities = Snapshot.Opportunities;
	Events = Snapshot.Events;
	Events.Sort([](const FCCLScheduledWorldEvent& A, const FCCLScheduledWorldEvent& B)
	{
		return A.Time == B.Time ? A.EventId < B.EventId : A.Time < B.Time;
	});
	Results = Snapshot.Results;
	Seed = Snapshot.Seed;
	Sequence = Snapshot.Sequence;
	Time = Snapshot.Time;
	Traces.Reset();
	ActiveActors.Reset();
	Reservations.Reset();
	return true;
}

bool FCCLLifeSimulation::Capture(FCCLSimulationSnapshot& Snapshot) const
{
	FCCLSimulationSnapshot Candidate;
	if (!Agents.Snapshot(Candidate.Agents))
	{
		return false;
	}

	Candidate.Seed = Seed;
	Candidate.Time = Time;
	Candidate.Sequence = Sequence;
	Candidate.Economy = Economy;
	Candidate.Opportunities = Opportunities;
	Candidate.Events = Events;
	Candidate.Results = Results;
	Snapshot = MoveTemp(Candidate);
	return true;
}

bool FCCLLifeSimulation::Save(TArray<uint8>& Bytes) const
{
	auto* Save = NewObject<UCCLLifeSave>();
	if (!Capture(Save->State))
	{
		return false;
	}

	TArray<uint8> Candidate;
	if (!UGameplayStatics::SaveGameToMemory(Save, Candidate))
	{
		return false;
	}

	const uint32 CRC = FCrc::MemCrc32(Candidate.GetData(), Candidate.Num());
	Candidate.Append(reinterpret_cast<const uint8*>(&CRC), sizeof(CRC));
	Bytes = MoveTemp(Candidate);
	return true;
}

bool FCCLLifeSimulation::Load(const TArray<uint8>& Bytes, FString& Error)
{
	if (Bytes.Num() < 32 || Bytes.Num() > 64 * 1024 * 1024)
	{
		return false;
	}

	const int32 Size = Bytes.Num() - sizeof(uint32);
	uint32 CRC;
	FMemory::Memcpy(&CRC, Bytes.GetData() + Size, sizeof(CRC));
	if (CRC != FCrc::MemCrc32(Bytes.GetData(), Size))
	{
		return false;
	}

	TArray<uint8> Payload;
	Payload.Append(Bytes.GetData(), Size);
	const auto* Save = Cast<UCCLLifeSave>(UGameplayStatics::LoadGameFromMemory(Payload));
	return Save && Initialize(Save->State, Error);
}

const FCCLWorldOpportunity* FCCLLifeSimulation::FindOpportunity(FGuid Id) const
{
	return Opportunities.FindByPredicate([Id](const FCCLWorldOpportunity& O) { return O.OpportunityId == Id; });
}

const TArray<FCCLDecisionTrace>& FCCLLifeSimulation::GetLastTrace(FGuid Id) const
{
	static const TArray<FCCLDecisionTrace> Empty;
	const auto* Result = Traces.Find(Id);
	return Result ? *Result : Empty;
}

FGuid FCCLLifeSimulation::NextId()
{
	++Sequence;
	return FGuid(0xCC180001, static_cast<uint32>(Seed), static_cast<uint32>(Sequence >> 32), static_cast<uint32>(Sequence));
}

bool FCCLLifeSimulation::BuildTransaction(const FCCLAgentRecord& Agent, const FCCLWorldOpportunity& O,
	FGuid RequestId, FCCLTransactionRequest& Request, FString& Error) const
{
	const auto* CurrentNeeds = Feature<FCCLAgentNeeds>(Agent, CCLAgentTags::Feature_Needs);
	if (CurrentNeeds && CurrentNeeds->Urgency.FindRef(CCLAgentTags::Injury) >= 1)
	{
		Error = TEXT("Incapacitated agent cannot execute activities.");
		return false;
	}
	const auto* Resources = Feature<FCCLAgentResourceLinks>(Agent, CCLAgentTags::Feature_Resources);
	const auto* Provider = Find(O.ProviderId);
	const auto* ProviderResources = Provider ? Feature<FCCLAgentResourceLinks>(*Provider, CCLAgentTags::Feature_Resources) : nullptr;
	const auto* Knowledge = Feature<FCCLAgentExperience>(Agent, CCLAgentTags::Feature_Experience);
	const bool bNeedsResources = O.Activity == CCLAgentTags::Trade || O.Activity == CCLAgentTags::Work ||
		O.Activity == CCLAgentTags::Eat || O.Activity == CCLAgentTags::Help || O.Activity == CCLAgentTags::Repay || O.Activity == CCLAgentTags::Improve;
	if ((bNeedsResources && (!Resources || !ProviderResources)) || !Knowledge || !Knowledge->KnownOpportunities.Contains(O.OpportunityId) ||
		!O.bAvailable || (O.ExpireTime > 0 && O.ExpireTime <= Time))
	{
		Error = TEXT("Opportunity is unknown, expired or unavailable.");
		return false;
	}

	if (O.OwnershipId.IsValid())
	{
		const auto* Ownership = Economy.Ownerships.Find(O.OwnershipId);
		if (!Ownership || (Ownership->OwnerId != Agent.Id && !Ownership->AuthorizedUsers.Contains(Agent.Id)))
		{
			Error = TEXT("Agent has no permission to use this place.");
			return false;
		}
	}

	Request.RequestId = RequestId;
	Request.Reason = O.Activity;
	Request.Time = Time;
	Request.Buyer = Resources ? Resources->Account : FGuid();
	Request.Seller = ProviderResources ? ProviderResources->Account : FGuid();
	Request.Price = O.Price;
	auto Transfer = [&](FGuid Source, FGuid Destination, FGameplayTag Resource, int64 Quantity)
	{
		FCCLItemTransfer Item;
		Item.Source = Source;
		Item.Destination = Destination;
		Item.Resource = Resource;
		Item.Quantity = Quantity;
		Request.Items.Add(Item);
	};

	if (O.Activity == CCLAgentTags::Trade)
	{
		Transfer(ProviderResources->Inventory, Resources->Inventory, O.Resource, O.Quantity);
	}
	else if (O.Activity == CCLAgentTags::Work)
	{
		Request.Buyer = ProviderResources->Account;
		Request.Seller = Resources->Account;
		Transfer(ProviderResources->Inventory, {}, CCLAgentTags::Material, 1);
		const auto* Life = Feature<FCCLLifeState>(Agent, CCLAgentTags::Feature_Life);
		const auto* Skill = Life ? Life->Skills.Find(CCLAgentTags::Work) : nullptr;
		Transfer({}, ProviderResources->Inventory, CCLAgentTags::Food,
			O.Quantity + (Skill && Skill->Proficiency >= 0.5f ? 1 : 0));
	}
	else if (O.Activity == CCLAgentTags::Eat)
	{
		Transfer(Resources->Inventory, {}, CCLAgentTags::Food, O.Quantity);
	}
	else if (O.Activity == CCLAgentTags::Help)
	{
		Transfer(Resources->Inventory, ProviderResources->Inventory, CCLAgentTags::Food, O.Quantity);
	}
	else if (O.Activity == CCLAgentTags::Repay)
	{
		const auto* Debt = Economy.Obligations.Find(O.ObligationId);
		if (!Debt || Debt->RemainingAmount <= 0)
		{
			Error = TEXT("No outstanding debt.");
			return false;
		}

		Request.ObligationId = O.ObligationId;
		Request.Price = FMath::Min(O.Price, Debt->RemainingAmount);
	}
	else if (O.Activity == CCLAgentTags::Improve)
	{
		const auto* Ownership = Economy.Ownerships.Find(O.OwnershipId);
		if (!Ownership || Ownership->OwnerId != Agent.Id || Ownership->ImprovementLevel >= 3)
		{
			Error = TEXT("No available facility improvement.");
			return false;
		}

		Transfer(Resources->Inventory, {}, CCLAgentTags::Material, O.Quantity);
	}
	else if (O.Activity == CCLAgentTags::Discover)
	{
		if (!FindOpportunity(O.DiscoveredOpportunityId) || Knowledge->KnownOpportunities.Contains(O.DiscoveredOpportunityId))
		{
			Error = TEXT("No new opportunity to discover.");
			return false;
		}
	}
	else if (O.Activity != CCLAgentTags::Rest)
	{
		const auto* Processor = ActivityProcessors.Find(O.Activity);
		if (Processor && Processor->Build)
		{
			return Processor->Build(Agent, O, Economy, Request, Error);
		}
		Error = TEXT("No executor registered for this activity.");
		return false;
	}

	return true;
}

FCCLDecisionResult FCCLLifeSimulation::Decide(FGuid AgentId) const
{
	const auto* Agent = Find(AgentId);
	const auto* Knowledge = Agent ? Feature<FCCLAgentExperience>(*Agent, CCLAgentTags::Feature_Experience) : nullptr;
	if (!Agent || !Knowledge)
	{
		return {};
	}

	FCCLDecisionInput Input;
	Input.Time = Time;
	Input.CurrentIntent = Agent->Intent;
	if (const auto* Traits = Feature<FCCLAgentTraits>(*Agent, CCLAgentTags::Feature_Traits))
	{
		Input.Traits = *Traits;
	}

	if (const auto* Needs = Feature<FCCLAgentNeeds>(*Agent, CCLAgentTags::Feature_Needs))
	{
		Input.Needs = *Needs;
	}

	for (FGuid KnownId : Knowledge->KnownOpportunities)
	{
		const auto* O = FindOpportunity(KnownId);
		FCCLTransactionRequest Request;
		FString Error;
		if (!O || !BuildTransaction(*Agent, *O, FGuid(1, 1, 1, 1), Request, Error))
		{
			continue;
		}

		const auto* Resources = Feature<FCCLAgentResourceLinks>(*Agent, CCLAgentTags::Feature_Resources);
		const auto* Inventory = Resources ? Economy.Inventories.Find(Resources->Inventory) : nullptr;
		const auto* Account = Resources ? Economy.Accounts.Find(Resources->Account) : nullptr;
		if ((Request.Price > 0 && !Account) || (Account && Request.Buyer == Account->Id && Request.Price > Account->Balance))
		{
			continue;
		}

		// Own resources are known; the provider's private stock is checked only during execution.
		int64 SpaceNeeded = 0;
		bool bAffordable = true;
		TMap<FGameplayTag, int64> Outgoing;
		for (const auto& Item : Request.Items)
		{
			if (Inventory && Item.Source == Inventory->Id)
			{
				Outgoing.FindOrAdd(Item.Resource) += Item.Quantity;
				SpaceNeeded -= Item.Quantity;
			}

			if (Inventory && Item.Destination == Inventory->Id)
			{
				SpaceNeeded += Item.Quantity;
			}
		}

		for (const auto& Pair : Outgoing)
		{
			bAffordable &= Inventory->Resources.FindRef(Pair.Key) >= Pair.Value;
		}

		if (!bAffordable || (Inventory && CCLEconomy::UsedCapacity(*Inventory) + SpaceNeeded > Inventory->Capacity))
		{
			continue;
		}

		auto& Candidate = Input.Candidates.AddDefaulted_GetRef();
		Candidate.OpportunityId = O->OpportunityId;
		Candidate.Activity = O->Activity;
		Candidate.Target.Kind = CCLAgentTags::Agent;
		Candidate.Target.Id = O->ProviderId;
		Candidate.BaseUtility = O->BaseUtility;
		Candidate.TraitSignals = O->TraitSignals;
		Candidate.NeedRelief = O->NeedRelief;
		if (O->Activity == CCLAgentTags::Trade && O->Resource == CCLAgentTags::Food)
		{
			const int64 Stock = Inventory->Resources.FindRef(CCLAgentTags::Food);
			Candidate.BaseUtility -= 0.25f * Stock;
			if (Stock == 0)
			{
				Candidate.NeedRelief.Add(CCLAgentTags::Hunger, 0.7f);
			}
		}
		if (O->ProviderId != AgentId)
		{
			const auto* Relationship = Knowledge->Relationships.Find(O->ProviderId);
			Candidate.RelationshipUtility = Relationship ? Relationship->Trust - 0.5f +
				0.5f * Relationship->Affection - Relationship->Resentment - Relationship->Fear : 0;
			for (const auto& Belief : Knowledge->Beliefs)
			{
				if (Belief.Subject.Id == O->ProviderId && Belief.Predicate == CCLAgentTags::Failure)
				{
					Candidate.RelationshipUtility -= 0.25f * Belief.Confidence;
				}
			}
		}

		if (const auto* Life = Feature<FCCLLifeState>(*Agent, CCLAgentTags::Feature_Life))
		{
			for (const auto& Goal : Life->LifeGoals)
			{
				const auto* Policy = GoalDefinitions.Find(Goal.GoalTag);
				if (Goal.Status == CCLAgentTags::Active && Policy && Policy->Get()->SupportingActivities.HasTagExact(O->Activity) &&
					(!Policy->Get()->bMatchBeneficiary || Goal.Beneficiary.Id == O->ProviderId))
				{
					const float Utility = Goal.Commitment * (1 - GoalProgress(*Agent, Goal));
					if (Utility > Candidate.GoalUtility)
					{
						Candidate.GoalUtility = Utility;
						Candidate.SupportingLifeGoalId = Goal.GoalId;
					}
				}
			}
		}
	}

	return CCLDecision::Evaluate(Input);
}

void FCCLLifeSimulation::ApplyOutcome(FCCLAgentRecord& Agent, const FCCLWorldOpportunity& O, FGuid RequestId)
{
	if (auto* Needs = MutableFeature<FCCLAgentNeeds>(Agent, CCLAgentTags::Feature_Needs))
	{
		for (const auto& Pair : O.NeedRelief)
		{
			if (auto* Value = Needs->Urgency.Find(Pair.Key))
			{
				*Value = FMath::Clamp(*Value - Pair.Value, 0.f, 1.f);
			}
		}
	}

	if (auto* Life = MutableFeature<FCCLLifeState>(Agent, CCLAgentTags::Feature_Life); Life && O.Activity == CCLAgentTags::Work)
	{
		auto& Skill = Life->Skills.FindOrAdd(CCLAgentTags::Work);
		Skill.Experience += 1;
		Skill.Proficiency = FMath::Min(1.f, Skill.Experience / 100.f);
	}

	if (auto* Experience = MutableFeature<FCCLAgentExperience>(Agent, CCLAgentTags::Feature_Experience))
	{
		FCCLObservation Observation;
		Observation.EventId = RequestId;
		Observation.EvidenceId = RequestId;
		Observation.Time = Time;
		Observation.EventType = CCLAgentTags::Success;
		Observation.PerceivedSubject.Kind = O.ProviderId != Agent.Id ? CCLAgentTags::Agent : CCLAgentTags::Unknown;
		Observation.PerceivedSubject.Id = O.ProviderId != Agent.Id ? O.ProviderId : FGuid();
		CCLAgentFeatures::Observe(*Experience, Observation);
		if (O.Activity == CCLAgentTags::Discover)
		{
			Experience->KnownOpportunities.AddUnique(O.DiscoveredOpportunityId);
		}
	}
}

FCCLLifeExecutionResult FCCLLifeSimulation::Execute(FGuid AgentId, FGuid OpportunityId, int32 ExpectedRevision, FGuid RequestId, bool bReduced)
{
	FCCLLifeExecutionResult Result;
	Result.RequestId = RequestId;
	const auto* Old = Results.FindByPredicate([RequestId](const FCCLLifeExecutionReceipt& R) { return R.RequestId == RequestId; });
	if (Old)
	{
		Result.bSucceeded = Old->AgentId == AgentId && Old->OpportunityId == OpportunityId && Old->Revision == ExpectedRevision && Old->bSucceeded;
		Result.Failure = Result.bSucceeded ? FString() : TEXT("Request already processed or terms changed.");
		return Result;
	}

	FCCLLifeExecutionReceipt Receipt;
	Receipt.RequestId = RequestId;
	Receipt.AgentId = AgentId;
	Receipt.OpportunityId = OpportunityId;
	Receipt.Revision = ExpectedRevision;
	Receipt.Time = Time;
	for (const auto& Trace : GetLastTrace(AgentId))
	{
		if (Trace.OpportunityId == OpportunityId)
		{
			Receipt.DecisionExplanation = FString::Printf(TEXT("activity=%s score=%.4f"), *Trace.Activity.ToString(), Trace.Score);
			for (const auto& Contribution : Trace.Contributions)
			{
				Receipt.DecisionExplanation += FString::Printf(TEXT(" %s=%.4f"), *Contribution.Source.ToString(), Contribution.Value);
			}
			break;
		}
	}
	auto Finish = [&](const FString& Error)
	{
		Result.Failure = Error;
		Result.bSucceeded = Error.IsEmpty();
		Receipt.bSucceeded = Result.bSucceeded;
		Receipt.Failure = Error;
		if (RequestId.IsValid() && Find(AgentId))
		{
			Results.Add(Receipt);
		}

		return Result;
	};
	const auto* Agent = Find(AgentId);
	const auto* O = FindOpportunity(OpportunityId);
	FCCLTransactionRequest Request;
	FString Error;
	if (!RequestId.IsValid() || !Agent || !O || O->Revision != ExpectedRevision ||
		!BuildTransaction(*Agent, *O, RequestId, Request, Error))
	{
		return Finish(Error.IsEmpty() ? TEXT("Invalid or stale execution request.") : Error);
	}

	if (const auto* Reservation = Reservations.Find(OpportunityId); Reservation && Reservation->Until > Time && Reservation->AgentId != AgentId)
	{
		return Finish(TEXT("Opportunity is reserved by another executor."));
	}
	const auto* Processor = ActivityProcessors.Find(O->Activity);
	if (bReduced && Processor && !Processor->bSupportsReducedExecution)
	{
		return Finish(TEXT("This activity requires its Actor executor."));
	}
	FCCLAgentRecord Updated = *Agent;
	ApplyOutcome(Updated, *O, RequestId);
	if (Processor && Processor->Apply)
	{
		Processor->Apply(Updated, *O);
	}
	if (!Registry.UpgradeAndValidate(Updated, Error))
	{
		return Finish(Error);
	}

	FCCLEconomyState Candidate;
	Candidate.Accounts = Economy.Accounts;
	Candidate.Inventories = Economy.Inventories;
	Candidate.Obligations = Economy.Obligations;
	Candidate.Ownerships = Economy.Ownerships;
	const auto Transaction = CCLEconomy::Execute(Candidate, Request);
	if (!Transaction.bSucceeded)
	{
		FCCLAgentRecord Failed = *Agent;
		if (auto* Experience = MutableFeature<FCCLAgentExperience>(Failed, CCLAgentTags::Feature_Experience))
		{
			FCCLObservation Observation;
			Observation.EventId = RequestId;
			Observation.EvidenceId = RequestId;
			Observation.Time = Time;
			Observation.EventType = CCLAgentTags::Failure;
			Observation.PerceivedSubject.Kind = O->ProviderId == AgentId ? CCLAgentTags::Unknown : CCLAgentTags::Agent;
			Observation.PerceivedSubject.Id = O->ProviderId == AgentId ? FGuid() : O->ProviderId;
			CCLAgentFeatures::Observe(*Experience, Observation);
		}

		const auto FailureLease = Agents.Acquire(Agents.GetHandle(AgentId), RequestId);
		Agents.Commit(FailureLease, Failed, Registry, Error);
		Agents.Release(FailureLease);
		return Finish(Transaction.Failure);
	}

	const auto Lease = Agents.Acquire(Agents.GetHandle(AgentId), RequestId);
	if (!Lease.Writer.IsValid())
	{
		return Finish(TEXT("Agent state is owned by another executor."));
	}

	if (O->Activity == CCLAgentTags::Improve)
	{
		++Candidate.Ownerships[O->OwnershipId].ImprovementLevel;
		const auto* Resources = Feature<FCCLAgentResourceLinks>(Updated, CCLAgentTags::Feature_Resources);
		Candidate.Inventories[Resources->Inventory].Capacity += 10;
	}

	TArray<TPair<FCCLAgentLease, FCCLAgentRecord>> Updates;
	Updates.Emplace(Lease, Updated);
	FCCLAgentLease OtherLease;
	if (O->ProviderId != AgentId && (O->Activity == CCLAgentTags::Help || O->Activity == CCLAgentTags::Trade))
	{
		const auto* Other = Find(O->ProviderId);
		if (Other)
		{
			FCCLAgentRecord Recipient = *Other;
			if (auto* Experience = MutableFeature<FCCLAgentExperience>(Recipient, CCLAgentTags::Feature_Experience))
			{
				FCCLObservation Observation;
				Observation.EventId = Observation.EvidenceId = RequestId;
				Observation.Time = Time;
				Observation.EventType = O->Activity == CCLAgentTags::Help ? CCLAgentTags::Aid : CCLAgentTags::Success;
				Observation.PerceivedSubject.Kind = CCLAgentTags::Agent;
				Observation.PerceivedSubject.Id = AgentId;
				CCLAgentFeatures::Observe(*Experience, Observation);
				OtherLease = Agents.Acquire(Agents.GetHandle(Other->Id), RequestId);
				Updates.Emplace(OtherLease, MoveTemp(Recipient));
			}
		}
	}

	const bool bCommitted = Agents.CommitBatch(MoveTemp(Updates), Registry, Error);
	Agents.Release(Lease);
	if (OtherLease.Writer.IsValid())
	{
		Agents.Release(OtherLease);
	}
	if (!bCommitted)
	{
		return Finish(Error.IsEmpty() ? TEXT("Agent result rejected.") : Error);
	}

	Economy.Accounts = MoveTemp(Candidate.Accounts);
	Economy.Inventories = MoveTemp(Candidate.Inventories);
	Economy.Obligations = MoveTemp(Candidate.Obligations);
	Economy.Ownerships = MoveTemp(Candidate.Ownerships);
	Economy.Journal.Add(Transaction);
	return Finish({});
}

float FCCLLifeSimulation::GoalProgress(const FCCLAgentRecord& Agent, const FCCLLifeGoalState& Goal) const
{
	const auto* Definition = GoalDefinitions.Find(Goal.GoalTag);
	if (!Definition || !Definition->Get() || !Definition->Get()->ProgressEvaluator)
	{
		return 0;
	}

	return Definition->Get()->ProgressEvaluator->Evaluate(Agent, Goal, Economy);
}

bool FCCLLifeSimulation::RegisterGoal(UCCLLifeGoalDefinition* Definition)
{
	if (!Definition || !Definition->GoalTag.IsValid() || !Definition->ProgressEvaluator || !Definition->CompletionPolicy)
	{
		return false;
	}

	GoalDefinitions.Add(Definition->GoalTag, TStrongObjectPtr<UCCLLifeGoalDefinition>(Definition));
	return true;
}

bool FCCLLifeSimulation::ApplyEvents(FString& Error)
{
	for (auto& Event : Events)
	{
		if (Event.bApplied || Event.Time > Time)
		{
			continue;
		}

		if (Event.Kind == CCLAgentTags::Failure)
		{
			if (auto* O = Opportunities.FindByPredicate([&](const FCCLWorldOpportunity& Value) { return Value.OpportunityId == Event.OpportunityId; }))
			{
				O->bAvailable = 0;
				++O->Revision;
			}
		}
		else if (Event.Kind == CCLAgentTags::Help)
		{
			if (auto* O = Opportunities.FindByPredicate([&](const FCCLWorldOpportunity& Value) { return Value.OpportunityId == Event.OpportunityId; }))
			{
				O->BaseUtility += 0.75f;
				++O->Revision;
			}
		}
		else if (Event.Kind == CCLAgentTags::Eat)
		{
			FCCLTransactionRequest Request;
			Request.RequestId = Event.EventId;
			Request.Reason = Event.Kind;
			Request.Time = Time;
			FCCLItemTransfer Loss;
			Loss.Source = Event.InventoryId;
			Loss.Resource = Event.Resource;
			Loss.Quantity = FMath::Min(Event.Quantity, CCLEconomy::Quantity(Economy, Event.InventoryId, Event.Resource));
			if (Loss.Quantity > 0)
			{
				Request.Items.Add(Loss);
				const auto Result = CCLEconomy::Execute(Economy, Request);
				if (!Result.bSucceeded)
				{
					Error = Result.Failure;
					return false;
				}
			}
		}

		Event.bApplied = 1;
	}
	return true;
}

void FCCLLifeSimulation::AdvanceTo(double TargetTime)
{
	FString Error;
	TryAdvanceTo(TargetTime, Error);
}

bool FCCLLifeSimulation::TryAdvanceTo(double TargetTime, FString& Error, int32 MaxSlices)
{
	Error.Reset();
	if (!FMath::IsFinite(TargetTime) || TargetTime < Time || MaxSlices <= 0)
	{
		Error = TEXT("Invalid life time or slice budget.");
		return false;
	}

	if (TargetTime == Time)
	{
		return true;
	}

	// Copy preserves stable handles, reservations and Actor ownership. Policies are read-only.
	FCCLLifeSimulation Candidate(*this);
	if (!Candidate.AdvanceCandidate(TargetTime, MaxSlices, Error) || !CCLEconomy::Validate(Candidate.Economy, Error))
	{
		return false;
	}

	*this = MoveTemp(Candidate);
	return true;
}

bool FCCLLifeSimulation::AdvanceCandidate(double TargetTime, int32 MaxSlices, FString& Error)
{
	// Written reduced activities complete at hourly boundaries; scheduled events split a boundary.
	int32 Slices = 0;
	while (Time < TargetTime)
	{
		if (++Slices > MaxSlices)
		{
			Error = TEXT("Life advance exceeded its slice budget; no state was published.");
			return false;
		}

		const double HourBoundary = (FMath::FloorToDouble(Time / 3600) + 1) * 3600;
		double Next = FMath::Min(TargetTime, HourBoundary);
		for (const auto& Event : Events)
		{
			if (!Event.bApplied && Event.Time > Time)
			{
				Next = FMath::Min(Next, Event.Time);
			}
		}

		TArray<FCCLAgentRecord> Snapshot;
		if (!Agents.Snapshot(Snapshot))
		{
			Error = TEXT("An Agent writer still owns state; release it before advancing.");
			return false;
		}

		if (Next <= Time)
		{
			Error = TEXT("Life time cannot advance at this precision.");
			return false;
		}

		Time = Next;
		if (!ApplyEvents(Error))
		{
			return false;
		}

		for (auto& Agent : Snapshot)
		{
			// Earlier agents in this time slice may already have changed this recipient's experience.
			if (const auto* Latest = Find(Agent.Id))
			{
				Agent = *Latest;
			}
			if (auto* Needs = MutableFeature<FCCLAgentNeeds>(Agent, CCLAgentTags::Feature_Needs))
			{
				CCLAgentFeatures::AdvanceNeeds(*Needs, Time - Agent.LastSimulatedTime);
			}

			Agent.LastSimulatedTime = Time;
			if (auto* Life = MutableFeature<FCCLLifeState>(Agent, CCLAgentTags::Feature_Life))
			{
				for (auto& Goal : Life->LifeGoals)
				{
					const auto* Policy = GoalDefinitions.Find(Goal.GoalTag);
					if (Goal.Status == CCLAgentTags::Active && Policy &&
						Policy->Get()->CompletionPolicy->IsComplete(GoalProgress(Agent, Goal)))
					{
						Goal.Status = CCLAgentTags::Completed;
					}
				}
			}

			const auto Lease = Agents.Acquire(Agents.GetHandle(Agent.Id), FGuid(0xCC18, 0, 0, 1));
			const bool bCommitted = Agents.Commit(Lease, Agent, Registry, Error);
			Agents.Release(Lease);
			if (!bCommitted)
			{
				return false;
			}

			if (Time != HourBoundary || ActiveActors.Contains(Agent.Id))
			{
				continue;
			}

			const auto Decision = Decide(Agent.Id);
			Traces.Add(Agent.Id, Decision.Traces);
			if (Decision.bSelected)
			{
				const auto IntentLease = Agents.Acquire(Agents.GetHandle(Agent.Id), FGuid(0xCC18, 0, 0, 2));
				Agent.Intent = Decision.Intent;
				const bool bIntentCommitted = Agents.Commit(IntentLease, Agent, Registry, Error);
				Agents.Release(IntentLease);
				if (!bIntentCommitted)
				{
					return false;
				}
				const auto* O = FindOpportunity(Decision.Intent.OpportunityId);
				if (!O)
				{
					Error = TEXT("Selected opportunity disappeared.");
					return false;
				}
				Execute(Agent.Id, O->OpportunityId, O->Revision, NextId(), true);
			}
		}
	}
	return true;
}

FString FCCLLifeSimulation::DailyReport() const
{
	FString Report;
	TArray<FCCLAgentRecord> Snapshot;
	if (!Agents.Snapshot(Snapshot))
	{
		return Report;
	}

	for (const auto& Agent : Snapshot)
	{
		const auto* Resources = Feature<FCCLAgentResourceLinks>(Agent, CCLAgentTags::Feature_Resources);
		const auto* Life = Feature<FCCLLifeState>(Agent, CCLAgentTags::Feature_Life);
		const auto* Experience = Feature<FCCLAgentExperience>(Agent, CCLAgentTags::Feature_Experience);
		const auto* Account = Resources ? Economy.Accounts.Find(Resources->Account) : nullptr;
		int64 Debt = 0;
		for (const auto& Pair : Economy.Obligations)
		{
			if (Pair.Value.DebtorId == Agent.Id)
			{
				Debt += Pair.Value.RemainingAmount;
			}
		}

		Report += FString::Printf(TEXT("day=%.3f agent=%s balance=%lld food=%lld debt=%lld action=%s memories=%d\n"),
			Time / 86400, *Agent.Id.ToString(), Account ? Account->Balance : 0,
			Resources ? CCLEconomy::Quantity(Economy, Resources->Inventory, CCLAgentTags::Food) : 0,
			Debt, *Agent.Intent.Activity.ToString(), Experience ? Experience->Memories.Num() : 0);
		if (Resources)
		{
			const auto* Inventory = Economy.Inventories.Find(Resources->Inventory);
			TArray<FGameplayTag> ResourceTags;
			Inventory->Resources.GetKeys(ResourceTags);
			ResourceTags.Sort([](FGameplayTag A, FGameplayTag B) { return A.ToString() < B.ToString(); });
			for (FGameplayTag Resource : ResourceTags)
			{
				Report += FString::Printf(TEXT(" inventory=%s quantity=%lld\n"), *Resource.ToString(), Inventory->Resources[Resource]);
			}
		}
		if (Life)
		{
			for (const auto& Goal : Life->LifeGoals)
			{
				Report += FString::Printf(TEXT(" goal=%s progress=%.4f status=%s\n"), *Goal.GoalTag.ToString(), GoalProgress(Agent, Goal), *Goal.Status.ToString());
			}
		}

		if (Experience)
		{
			TArray<FGuid> Others;
			Experience->Relationships.GetKeys(Others);
			Others.Sort();
			for (FGuid Other : Others)
			{
				const auto& R = Experience->Relationships[Other];
				Report += FString::Printf(TEXT(" relation=%s trust=%.4f affection=%.4f resentment=%.4f\n"), *Other.ToString(), R.Trust, R.Affection, R.Resentment);
			}
		}

		for (const auto& Trace : GetLastTrace(Agent.Id))
		{
			Report += FString::Printf(TEXT(" candidate=%s opportunity=%s score=%.4f"), *Trace.Activity.ToString(), *Trace.OpportunityId.ToString(), Trace.Score);
			for (const auto& Contribution : Trace.Contributions)
			{
				Report += FString::Printf(TEXT(" %s=%.4f"), *Contribution.Source.ToString(), Contribution.Value);
			}

			Report += TEXT("\n");
		}
	}

	for (const auto& Event : Events)
	{
		if (Event.bApplied && Event.Time > Time - 86400 && Event.Time <= Time)
		{
			Report += FString::Printf(TEXT(" event=%s time=%.0f kind=%s quantity=%lld\n"),
				*Event.EventId.ToString(), Event.Time, *Event.Kind.ToString(), Event.Quantity);
		}
	}
	for (const auto& Result : Results)
	{
		if (Result.Time > Time - 86400 && Result.Time <= Time)
		{
			Report += FString::Printf(TEXT(" execution=%s time=%.0f agent=%s opportunity=%s success=%d failure=%s %s\n"),
				*Result.RequestId.ToString(), Result.Time, *Result.AgentId.ToString(), *Result.OpportunityId.ToString(),
				Result.bSucceeded, *Result.Failure, *Result.DecisionExplanation);
		}
	}
	return Report;
}

FCCLSimulationSnapshot FCCLLifeSimulation::MerchantScenario(int32 InSeed)
{
	FCCLSimulationSnapshot S;
	S.Seed = InSeed;
	auto Id = [](int32 Group, int32 Index) { return FGuid(0xCC180000, 0, Group, Index + 1); };
	FRandomStream Random(InSeed);
	const TArray<FGameplayTag> Axes = {CCLAgentTags::RiskTolerance, CCLAgentTags::Aggressiveness, CCLAgentTags::Curiosity,
		CCLAgentTags::Sociability, CCLAgentTags::SelfControl, CCLAgentTags::Empathy, CCLAgentTags::MaterialGain,
		CCLAgentTags::Duty, CCLAgentTags::OthersWelfare};
	for (int32 Index = 0; Index < 8; ++Index)
	{
		FCCLAgentRecord Agent;
		Agent.Id = Id(1, Index);
		Agent.DefinitionId = FPrimaryAssetId(TEXT("Agent"), TEXT("Merchant"));
		Agent.Location.PlaceId = Id(4, Index);
		Agent.Location.Position = FVector(-1300 + (Index % 4) * 180, 400 + (Index / 4) * 250, 100);
		FCCLAgentTraits Traits;
		for (int32 Axis = 0; Axis < Axes.Num(); ++Axis)
		{
			Traits.Axes.Add(Axes[Axis], FMath::Clamp(((Index * 3 + Axis * 5) % 9 + 1) / 10.f + Random.FRandRange(-0.025f, 0.025f), 0.f, 1.f));
		}

		Agent.Features.Add(CCLAgentTags::Feature_Traits, {1, FInstancedStruct::Make(Traits)});
		FCCLAgentResourceLinks Links;
		Links.Account = Id(2, Index);
		Links.Inventory = Id(3, Index);
		Links.Ownerships.Add(Id(5, Index));
		Links.ObligationIds.Add(Id(6, Index));
		Agent.Features.Add(CCLAgentTags::Feature_Resources, {1, FInstancedStruct::Make(Links)});
		FCCLAccountRecord Account;
		Account.Id = Links.Account;
		Account.OwnerId = Agent.Id;
		Account.Balance = 40 + Index * 15;
		S.Economy.Accounts.Add(Account.Id, Account);
		FCCLResourceInventory Inventory;
		Inventory.Id = Links.Inventory;
		Inventory.OwnerId = Agent.Id;
		Inventory.Capacity = 200;
		Inventory.Resources.Add(CCLAgentTags::Food, 5 + Index);
		Inventory.Resources.Add(CCLAgentTags::Material, 40);
		S.Economy.Inventories.Add(Inventory.Id, Inventory);
		FCCLOwnershipRecord Ownership;
		Ownership.Id = Id(5, Index);
		Ownership.PlaceId = Id(4, Index);
		Ownership.OwnerId = Agent.Id;
		for (int32 Other = 0; Other < 8; ++Other)
		{
			Ownership.AuthorizedUsers.Add(Id(1, Other));
		}

		S.Economy.Ownerships.Add(Ownership.Id, Ownership);
		FCCLObligationRecord Debt;
		Debt.ObligationId = Id(6, Index);
		Debt.DebtorId = Agent.Id;
		Debt.CreditorId = Id(1, (Index + 1) % 8);
		Debt.OriginalAmount = Debt.RemainingAmount = 10 + Index * 5;
		Debt.DueTime = 20 * 86400;
		Debt.Status = CCLAgentTags::Active;
		S.Economy.Obligations.Add(Debt.ObligationId, Debt);
		FCCLLifeState Life;
		Life.Occupation = CCLAgentTags::Merchant;
		Life.Residence = Agent.Location;
		Life.Workplace = Agent.Location;
		FCCLSocialLink Family;
		Family.OtherAgentId = Id(1, (Index + 1) % 8);
		Family.LinkType = CCLAgentTags::Family;
		Life.SocialLinks.Add(Family);
		const TArray<FGameplayTag> Goals = {CCLAgentTags::Goal_Living, CCLAgentTags::Goal_Debt, CCLAgentTags::Goal_Support,
			CCLAgentTags::Goal_Business, CCLAgentTags::Goal_Mastery};
		for (int32 G = 0; G < Goals.Num(); ++G)
		{
			FCCLLifeGoalState Goal;
			Goal.GoalId = Id(20 + Index, G);
			Goal.GoalTag = Goals[G];
			Goal.DefinitionId = FPrimaryAssetId(TEXT("LifeGoal"), Goals[G].GetTagName());
			Goal.Beneficiary.Kind = CCLAgentTags::Agent;
			Goal.Beneficiary.Id = G == 2 ? Family.OtherAgentId : Agent.Id;
			Goal.Commitment = 0.2f + ((Index + G) % 4) * 0.2f;
			Goal.Status = CCLAgentTags::Active;
			FCCLLifeGoalParameters Parameters;
			Parameters.TargetAmount = G == 0 ? 160 : G == 2 ? 8 : G == 3 ? 3 : 100;
			Parameters.SubjectId = G == 1 ? Debt.ObligationId : Ownership.Id;
			Goal.Parameters = FInstancedStruct::Make(Parameters);
			Life.LifeGoals.Add(Goal);
		}

		Agent.Features.Add(CCLAgentTags::Feature_Life, {1, FInstancedStruct::Make(Life)});
		FCCLAgentNeeds Needs;
		Needs.Urgency.Add(CCLAgentTags::Hunger, 0.2f);
		Needs.Urgency.Add(CCLAgentTags::Fatigue, 0.1f);
		Needs.Urgency.Add(CCLAgentTags::Social, 0.2f);
		Agent.Features.Add(CCLAgentTags::Feature_Needs, {1, FInstancedStruct::Make(Needs)});
		FCCLAgentExperience Experience;
		FCCLRelationship Relationship;
		Relationship.Trust = 0.2f + Index * 0.1f;
		Experience.Relationships.Add(Family.OtherAgentId, Relationship);
		Agent.Features.Add(CCLAgentTags::Feature_Experience, {1, FInstancedStruct::Make(Experience)});
		S.Agents.Add(Agent);
	}

	const TArray<FGameplayTag> Activities = {CCLAgentTags::Work, CCLAgentTags::Trade, CCLAgentTags::Eat, CCLAgentTags::Rest,
		CCLAgentTags::Help, CCLAgentTags::Repay, CCLAgentTags::Improve, CCLAgentTags::Discover};
	for (int32 Index = 0; Index < 8; ++Index)
	{
		auto& Experience = S.Agents[Index].Features[CCLAgentTags::Feature_Experience].Data.GetMutable<FCCLAgentExperience>();
		for (int32 Activity = 0; Activity < Activities.Num(); ++Activity)
		{
			FCCLWorldOpportunity O;
			O.OpportunityId = Id(40 + Index, Activity);
			O.Activity = Activities[Activity];
			const int32 Provider = Activity == 2 || Activity == 3 || Activity == 7 ? Index : (Index + 1) % 8;
			O.ProviderId = Id(1, Provider);
			O.Location = S.Agents[Provider].Location;
			O.BaseUtility = 0.05f;
			O.Resource = CCLAgentTags::Food;
			if (Activity == 0)
			{
				O.Price = 5;
				O.Quantity = 3;
				O.OwnershipId = Id(5, Provider);
				O.TraitSignals.Add(CCLAgentTags::MaterialGain, 0.7f);
				O.TraitSignals.Add(CCLAgentTags::SelfControl, 0.4f);
				O.TraitSignals.Add(CCLAgentTags::RiskTolerance, 0.1f);
				O.NeedRelief.Add(CCLAgentTags::Fatigue, -0.08f);
			}
			else if (Activity == 1)
			{
				O.Price = 3;
				O.TraitSignals.Add(CCLAgentTags::MaterialGain, -0.2f);
			}
			else if (Activity == 2)
			{
				O.NeedRelief.Add(CCLAgentTags::Hunger, 0.7f);
			}
			else if (Activity == 3)
			{
				O.NeedRelief.Add(CCLAgentTags::Fatigue, 0.6f);
			}
			else if (Activity == 4)
			{
				O.TraitSignals.Add(CCLAgentTags::OthersWelfare, 0.8f);
				O.TraitSignals.Add(CCLAgentTags::Empathy, 0.3f);
				O.TraitSignals.Add(CCLAgentTags::Sociability, 0.3f);
				O.TraitSignals.Add(CCLAgentTags::Aggressiveness, -0.15f);
				O.NeedRelief.Add(CCLAgentTags::Social, 0.35f);
			}
			else if (Activity == 5)
			{
				O.Price = 5;
				O.ObligationId = Id(6, Index);
				O.TraitSignals.Add(CCLAgentTags::Duty, 0.8f);
			}
			else if (Activity == 6)
			{
				O.Price = 20;
				O.Quantity = 3;
				O.OwnershipId = Id(5, Index);
				O.TraitSignals.Add(CCLAgentTags::MaterialGain, 0.5f);
			}
			else
			{
				O.DiscoveredOpportunityId = Id(40 + (Index + 2) % 8, 1);
				O.TraitSignals.Add(CCLAgentTags::Curiosity, 1.1f);
			}

			Experience.KnownOpportunities.Add(O.OpportunityId);
			S.Opportunities.Add(O);
		}
	}

	FCCLScheduledWorldEvent Shortage;
	Shortage.EventId = Id(80, 0);
	Shortage.Time = 7 * 86400;
	Shortage.Kind = CCLAgentTags::Eat;
	Shortage.InventoryId = Id(3, 1);
	Shortage.Resource = CCLAgentTags::Food;
	Shortage.Quantity = 20;
	S.Events.Add(Shortage);
	FCCLScheduledWorldEvent Help;
	Help.EventId = Id(80, 1);
	Help.Time = 10 * 86400;
	Help.Kind = CCLAgentTags::Help;
	Help.OpportunityId = Id(40, 4);
	S.Events.Add(Help);
	FCCLScheduledWorldEvent Failure;
	Failure.EventId = Id(80, 2);
	Failure.Time = 15 * 86400;
	Failure.Kind = CCLAgentTags::Failure;
	Failure.OpportunityId = Id(41, 1);
	S.Events.Add(Failure);
	return S;
}

void FCCLLifeSimulation::SetActorActive(FGuid Id, bool bActive)
{
	if (bActive)
	{
		ActiveActors.Add(Id);
	}
	else
	{
		ActiveActors.Remove(Id);
	}
}

bool FCCLLifeSimulation::SelectIntent(FGuid Id, const FCCLPersistentIntent& Intent, const TArray<FCCLDecisionTrace>* DecisionTraces)
{
	const auto* Record = Find(Id);
	if (!Record)
	{
		return false;
	}

	FCCLAgentRecord Updated = *Record;
	Updated.Intent = Intent;
	const auto Lease = Agents.Acquire(Agents.GetHandle(Id), FGuid(0xCC18, 0, 1, 1));
	FString Error;
	const bool bCommitted = Agents.Commit(Lease, Updated, Registry, Error);
	Agents.Release(Lease);
	if (bCommitted && DecisionTraces)
	{
		Traces.Add(Id, *DecisionTraces);
	}
	return bCommitted;
}

bool FCCLLifeSimulation::UpdateLocation(FGuid Id, FVector Position)
{
	const auto* Record = Find(Id);
	if (!Record || Position.ContainsNaN())
	{
		return false;
	}

	FCCLAgentRecord Updated = *Record;
	Updated.Location.Position = Position;
	const auto Lease = Agents.Acquire(Agents.GetHandle(Id), FGuid(0xCC18, 0, 1, 2));
	FString Error;
	const bool bCommitted = Agents.Commit(Lease, Updated, Registry, Error);
	Agents.Release(Lease);
	return bCommitted;
}

bool FCCLLifeSimulation::RegisterActivity(FGameplayTag Activity, FCCLActivityProcessor Processor)
{
	const TArray<FGameplayTag> Native = {CCLAgentTags::Work, CCLAgentTags::Trade, CCLAgentTags::Eat, CCLAgentTags::Rest,
		CCLAgentTags::Help, CCLAgentTags::Repay, CCLAgentTags::Improve, CCLAgentTags::Discover};
	if (!Activity.IsValid() || Native.Contains(Activity) || ActivityProcessors.Contains(Activity) || !Processor.Build)
	{
		return false;
	}
	ActivityProcessors.Add(Activity, MoveTemp(Processor));
	return true;
}

FGuid FCCLLifeSimulation::ReserveOpportunity(FGuid AgentId, FGuid OpportunityId)
{
	const auto* Record = Find(AgentId);
	const auto* Opportunity = FindOpportunity(OpportunityId);
	const auto* Knowledge = Record ? Feature<FCCLAgentExperience>(*Record, CCLAgentTags::Feature_Experience) : nullptr;
	if (!Opportunity || !Knowledge || !Knowledge->KnownOpportunities.Contains(OpportunityId) || !Opportunity->bAvailable ||
		(Opportunity->ExpireTime > 0 && Opportunity->ExpireTime <= Time))
	{
		return {};
	}
	if (const auto* Existing = Reservations.Find(OpportunityId); Existing && Existing->Until > Time)
	{
		return Existing->AgentId == AgentId ? Existing->Token : FGuid();
	}
	FReservation Reservation;
	Reservation.AgentId = AgentId;
	Reservation.Token = FGuid::NewGuid();
	Reservation.Until = Time + 7200;
	Reservations.Add(OpportunityId, Reservation);
	return Reservation.Token;
}
void FCCLLifeSimulation::ReleaseOpportunity(FGuid Token)
{
	for (auto It = Reservations.CreateIterator(); It; ++It)
	{
		if (It.Value().Token == Token)
		{
			It.RemoveCurrent();
			return;
		}
	}
}
bool FCCLLifeSimulation::HasReservation(FGuid AgentId, FGuid OpportunityId, FGuid Token) const
{
	const auto* Reservation = Reservations.Find(OpportunityId);
	return Reservation && Token.IsValid() && Reservation->AgentId == AgentId && Reservation->Token == Token && Reservation->Until > Time;
}
