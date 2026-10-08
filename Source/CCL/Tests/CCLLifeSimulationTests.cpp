#include "Agents/CCLLifeSimulation.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Agents/CCLAgentTags.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLLifeTransactionTest, "CCL.Agent.AtomicTransactions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLLifeTransactionTest::RunTest(const FString& Parameters)
{
	auto State = FCCLLifeSimulation::MerchantScenario(42);
	auto& Economy = State.Economy;
	const auto& Buyer = State.Agents[0].Features[CCLAgentTags::Feature_Resources].Data.Get<FCCLAgentResourceLinks>();
	const auto& Seller = State.Agents[1].Features[CCLAgentTags::Feature_Resources].Data.Get<FCCLAgentResourceLinks>();
	FCCLTransactionRequest Request;
	Request.RequestId = FGuid(90, 1, 1, 1);
	Request.Buyer = Buyer.Account;
	Request.Seller = Seller.Account;
	Request.Price = 5;
	Request.Reason = CCLAgentTags::Trade;
	FCCLItemTransfer Transfer;
	Transfer.Source = Seller.Inventory;
	Transfer.Destination = Buyer.Inventory;
	Transfer.Resource = CCLAgentTags::Food;
	Transfer.Quantity = 1;
	Request.Items.Add(Transfer);
	const int64 Money = Economy.Accounts[Buyer.Account].Balance;
	const int64 Food = CCLEconomy::Quantity(Economy, Buyer.Inventory, CCLAgentTags::Food);
	Economy.Inventories[Buyer.Inventory].Capacity = CCLEconomy::UsedCapacity(Economy.Inventories[Buyer.Inventory]);
	TestFalse(TEXT("full inventory rejects entire purchase"), CCLEconomy::Execute(Economy, Request).bSucceeded != 0);
	TestEqual(TEXT("rejected purchase retains money"), Economy.Accounts[Buyer.Account].Balance, Money);
	TestEqual(TEXT("rejected purchase retains stock"), CCLEconomy::Quantity(Economy, Buyer.Inventory, CCLAgentTags::Food), Food);
	Economy.Inventories[Buyer.Inventory].Capacity += 10;
	TestFalse(TEXT("failed request stays failed on retry"), CCLEconomy::Execute(Economy, Request).bSucceeded != 0);
	Request.RequestId.D = 2;
	TestTrue(TEXT("new request transfers atomically"), CCLEconomy::Execute(Economy, Request).bSucceeded != 0);
	TestTrue(TEXT("successful retry returns original receipt"), CCLEconomy::Execute(Economy, Request).bSucceeded != 0);
	TestEqual(TEXT("retry cannot duplicate money"), Economy.Accounts[Buyer.Account].Balance, Money - 5);
	TestEqual(TEXT("retry cannot duplicate food"), CCLEconomy::Quantity(Economy, Buyer.Inventory, CCLAgentTags::Food), Food + 1);
	Request.Price = 1;
	TestFalse(TEXT("same request with changed terms rejected"), CCLEconomy::Execute(Economy, Request).bSucceeded != 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLLifeAblationTest, "CCL.Agent.DecisionAblations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLLifeAblationTest::RunTest(const FString& Parameters)
{
	FCCLDecisionInput Input;
	FCCLDecisionCandidate Work;
	Work.OpportunityId = FGuid(1, 0, 0, 1);
	Work.Activity = CCLAgentTags::Work;
	Work.TraitSignals.Add(CCLAgentTags::MaterialGain, 1);
	FCCLDecisionCandidate Help;
	Help.OpportunityId = FGuid(1, 0, 0, 2);
	Help.Activity = CCLAgentTags::Help;
	Help.TraitSignals.Add(CCLAgentTags::OthersWelfare, 1);
	Input.Candidates = {Work, Help};
	Input.Traits.Axes.Add(CCLAgentTags::MaterialGain, 0.9f);
	Input.Traits.Axes.Add(CCLAgentTags::OthersWelfare, 0.1f);
	TestTrue(TEXT("material value prefers paid work"), CCLDecision::Evaluate(Input).Intent.Activity == CCLAgentTags::Work);
	Input.Traits.Axes[CCLAgentTags::MaterialGain] = 0.1f;
	Input.Traits.Axes[CCLAgentTags::OthersWelfare] = 0.9f;
	const auto Caring = CCLDecision::Evaluate(Input);
	TestTrue(TEXT("only values changed: help wins"), Caring.Intent.Activity == CCLAgentTags::Help);
	float Explained = 0;
	for (const auto& Contribution : Caring.Traces[1].Contributions)
	{
		Explained += Contribution.Value;
	}

	TestEqual(TEXT("score fully explained by contributions"), Explained, Caring.Traces[1].Score);
	Input.CurrentIntent = Caring.Intent;
	Input.CurrentIntent.ExpireTime = 100;
	Input.Needs.Urgency.Add(CCLAgentTags::Hunger, 1);
	Input.Candidates[0].NeedRelief.Add(CCLAgentTags::Hunger, 1);
	TestTrue(TEXT("urgent need interrupts commitment without deleting life goals"), CCLDecision::Evaluate(Input).Intent.Activity == CCLAgentTags::Work);

	auto Scenario = FCCLLifeSimulation::MerchantScenario(42);
	auto& Agent = Scenario.Agents[0];
	auto& Experience = Agent.Features[CCLAgentTags::Feature_Experience].Data.GetMutable<FCCLAgentExperience>();
	const auto First = Scenario.Opportunities[1];
	const auto Second = Scenario.Opportunities[17];
	Experience.KnownOpportunities = {First.OpportunityId, Second.OpportunityId};
	Experience.Relationships.FindOrAdd(First.ProviderId).Trust = 0.9f;
	Experience.Relationships.FindOrAdd(Second.ProviderId).Trust = 0.1f;
	FCCLLifeSimulation Simulation;
	FString Error;
	TestTrue(TEXT("scenario initializes"), Simulation.Initialize(Scenario, Error));
	TestTrue(TEXT("trusted supplier selected"), Simulation.Decide(Agent.Id).Intent.OpportunityId == First.OpportunityId);
	Experience.Relationships[First.ProviderId].Trust = 0.1f;
	Experience.Relationships[Second.ProviderId].Trust = 0.9f;
	TestTrue(TEXT("same traits with different relationships initializes"), Simulation.Initialize(Scenario, Error));
	TestTrue(TEXT("only relationship changed: other supplier selected"), Simulation.Decide(Agent.Id).Intent.OpportunityId == Second.OpportunityId);
	const auto& Resources = Agent.Features[CCLAgentTags::Feature_Resources].Data.Get<FCCLAgentResourceLinks>();
	Scenario.Economy.Accounts[Resources.Account].Balance = 0;
	TestTrue(TEXT("poor agent initializes"), Simulation.Initialize(Scenario, Error));
	TestFalse(TEXT("only balance changed: purchases infeasible"), Simulation.Decide(Agent.Id).bSelected != 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLLifeThirtyDayTest, "CCL.Agent.ThirtyDays",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLLifeThirtyDayTest::RunTest(const FString& Parameters)
{
	const auto Initial = FCCLLifeSimulation::MerchantScenario(42);
	FCCLLifeSimulation Simulation;
	FString Error;
	if (!TestTrue(TEXT("eight authored merchants validate"), Simulation.Initialize(Initial, Error)))
	{
		AddError(Error);
		return false;
	}

	FString Daily;
	TArray<uint8> Checkpoint;
	const double Started = FPlatformTime::Seconds();
	for (int32 Day = 1; Day <= 30; ++Day)
	{
		Simulation.AdvanceTo(Day * 86400.);
		Daily += Simulation.DailyReport();
		if (Day == 15)
		{
			TestTrue(TEXT("midpoint save"), Simulation.Save(Checkpoint));
		}
	}

	FCCLSimulationSnapshot Final;
	TestTrue(TEXT("final consistent capture"), Simulation.Capture(Final));
	TestTrue(TEXT("final snapshot still validates"), CCLEconomy::Validate(Final.Economy, Error));
	int64 InitialMoney = 0;
	int64 FinalMoney = 0;
	for (const auto& Pair : Initial.Economy.Accounts)
	{
		InitialMoney += Pair.Value.Balance;
		FinalMoney += Final.Economy.Accounts[Pair.Key].Balance;
	}

	TestEqual(TEXT("currency conserved across work, trading and debt payments"), FinalMoney, InitialMoney);
	for (const auto Resource : {FGameplayTag(CCLAgentTags::Food), FGameplayTag(CCLAgentTags::Material)})
	{
		int64 Expected = 0;
		int64 Actual = 0;
		for (const auto& Pair : Initial.Economy.Inventories)
		{
			Expected += Pair.Value.Resources.FindRef(Resource);
			Actual += Final.Economy.Inventories[Pair.Key].Resources.FindRef(Resource);
		}

		for (const auto& Receipt : Final.Economy.Journal)
		{
			if (Receipt.bSucceeded)
			{
				for (const auto& Transfer : Receipt.Request.Items)
				{
					if (Transfer.Resource == Resource)
					{
						Expected += !Transfer.Source.IsValid() ? Transfer.Quantity : 0;
						Expected -= !Transfer.Destination.IsValid() ? Transfer.Quantity : 0;
					}
				}
			}
		}

		TestEqual(FString::Printf(TEXT("%s explained by production and consumption"), *Resource.ToString()), Actual, Expected);
	}

	FCCLLifeSimulation Resumed;
	TestTrue(TEXT("midpoint restore validates all links"), Resumed.Load(Checkpoint, Error));
	Resumed.AdvanceTo(30 * 86400.);
	TestEqual(TEXT("saved resume reproduces decisions and life outcomes"), Resumed.DailyReport(), Simulation.DailyReport());
	FCCLSimulationSnapshot ResumedState;
	Resumed.Capture(ResumedState);
	TestEqual(TEXT("resume retains idempotency sequence"), ResumedState.Sequence, Final.Sequence);
	TestEqual(TEXT("resume retains every transaction"), ResumedState.Economy.Journal.Num(), Final.Economy.Journal.Num());
	TestTrue(TEXT("scheduled events executed"), Final.Events.ContainsByPredicate([](const FCCLScheduledWorldEvent& E) { return E.bApplied != 0; }));
	const FString Directory = FPaths::ProjectSavedDir() / TEXT("Tests/LifeSimulation");
	IFileManager::Get().MakeDirectory(*Directory, true);
	TestTrue(TEXT("daily report written"), FFileHelper::SaveStringToFile(Daily, *(Directory / TEXT("merchants-30-days.txt"))));
	AddInfo(FString::Printf(TEXT("30 days, 8 agents, %d receipts, %.3f seconds; daily report in Saved/Tests/LifeSimulation"),
		Final.Economy.Journal.Num(), FPlatformTime::Seconds() - Started));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLLifeOutcomeTest, "CCL.Agent.ExecutionFeedback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLLifeOutcomeTest::RunTest(const FString& Parameters)
{
    auto Initial = FCCLLifeSimulation::MerchantScenario(42);
    const FGuid Id = Initial.Agents[0].Id;
    const auto Purchase = Initial.Opportunities[1];
    const auto Aid = Initial.Opportunities[4];
    const auto& Seller = Initial.Agents[1].Features[CCLAgentTags::Feature_Resources].Data.Get<FCCLAgentResourceLinks>();
    Initial.Economy.Inventories[Seller.Inventory].Resources[CCLAgentTags::Food] = 0;
    FCCLLifeSimulation Simulation;
    FString Error;
    TestTrue(TEXT("initialize depleted supplier"), Simulation.Initialize(Initial, Error));
    const auto NeedsBefore = Simulation.Find(Id)->Features[CCLAgentTags::Feature_Needs].Data.Get<FCCLAgentNeeds>();
    const auto& Goal = Initial.Agents[0].Features[CCLAgentTags::Feature_Life].Data.Get<FCCLLifeState>().LifeGoals[0];
    const float ProgressBefore = Simulation.GoalProgress(*Simulation.Find(Id), Goal);
    TestFalse(TEXT("selected purchase really fails"), Simulation.Execute(Id, Purchase.OpportunityId, Purchase.Revision, FGuid(90, 5, 1, 1)).bSucceeded != 0);
    TestEqual(TEXT("failure does not relieve hunger"), Simulation.Find(Id)->Features[CCLAgentTags::Feature_Needs].Data.Get<FCCLAgentNeeds>().Urgency.FindRef(CCLAgentTags::Hunger),
        NeedsBefore.Urgency.FindRef(CCLAgentTags::Hunger));
    TestEqual(TEXT("failure cannot progress goal"), Simulation.GoalProgress(*Simulation.Find(Id), Goal), ProgressBefore);
    TestTrue(TEXT("failed supplier remembered"), Simulation.Find(Id)->Features[CCLAgentTags::Feature_Experience].Data.Get<FCCLAgentExperience>().Beliefs.ContainsByPredicate(
        [&](const FCCLBelief& B) { return B.Subject.Id == Purchase.ProviderId && B.Predicate == CCLAgentTags::Failure; }));
    const float TrustBefore = Simulation.Find(Aid.ProviderId)->Features[CCLAgentTags::Feature_Experience].Data.Get<FCCLAgentExperience>().Relationships.FindRef(Id).Trust;
    TestTrue(TEXT("help transfers actual food"), Simulation.Execute(Id, Aid.OpportunityId, Aid.Revision, FGuid(90, 5, 1, 2)).bSucceeded != 0);
    TestTrue(TEXT("recipient remembers helper"), Simulation.Find(Aid.ProviderId)->Features[CCLAgentTags::Feature_Experience].Data.Get<FCCLAgentExperience>().Relationships.FindRef(Id).Trust > TrustBefore);
    const auto Lease = Simulation.GetAgents().Acquire(Simulation.GetAgents().GetHandle(Aid.ProviderId), FGuid(90, 5, 9, 1));
    const int64 Food = CCLEconomy::Quantity(Simulation.GetEconomy(), Seller.Inventory, CCLAgentTags::Food);
    TestFalse(TEXT("recipient writer prevents partial multi-agent publication"), Simulation.Execute(Id, Aid.OpportunityId, Aid.Revision, FGuid(90, 5, 1, 3)).bSucceeded != 0);
    TestEqual(TEXT("lease conflict leaves resources unchanged"), CCLEconomy::Quantity(Simulation.GetEconomy(), Seller.Inventory, CCLAgentTags::Food), Food);
    Simulation.GetAgents().Release(Lease);
    auto Invalid = Initial;
    Invalid.Agents[0].Features[CCLAgentTags::Feature_Life].Data.GetMutable<FCCLLifeState>().LifeGoals[0].Beneficiary.Id = FGuid(99, 99, 99, 99);
    TestFalse(TEXT("unresolved beneficiary rejected"), Simulation.Initialize(Invalid, Error));
    TestTrue(TEXT("failed restore retains live state"), Simulation.Find(Id) != nullptr);
    return true;
}
#endif
