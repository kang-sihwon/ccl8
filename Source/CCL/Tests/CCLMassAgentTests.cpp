#include "Agents/CCLMassAgentBridge.h"
#include "Agents/CCLLifeSimulation.h"
#include "Agents/CCLAgentTags.h"
#include "MassEntityManager.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLMassAgentTest, "CCL.Agent.MassHandoff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLMassAgentTest::RunTest(const FString& Parameters)
{
	FCCLLifeSimulation Simulation;
	FString Error;
	auto Initial = FCCLLifeSimulation::MerchantScenario(42);
	TestTrue(TEXT("scenario initialized"), Simulation.Initialize(Initial, Error));
	auto Manager = MakeShared<FMassEntityManager>();
	FMassEntityManagerStorageInitParams Init;
	Init.Set<FMassEntityManager_InitParams_Concurrent>({1024, 256});
	Manager->Initialize(Init);
	const FGuid Id = Initial.Agents[0].Id;
	const auto Entity = CCLMassAgentBridge::Enter(*Manager, Simulation.GetAgents(), Id);
	TestTrue(TEXT("actual Mass Entity created"), Manager->IsEntityValid(Entity));
	TestFalse(TEXT("Actor cannot acquire a concurrent writer"), Simulation.GetAgents().Acquire(
		Simulation.GetAgents().GetHandle(Id), FGuid::NewGuid()).Writer.IsValid());
	FCCLSimulationSnapshot Pending;
	TestFalse(TEXT("save waits for pending Mass results"), Simulation.Capture(Pending));
	FCCLDecisionInput Input;
	Input.Traits = Initial.Agents[0].Features[CCLAgentTags::Feature_Traits].Data.Get<FCCLAgentTraits>();
	FCCLDecisionCandidate Candidate;
	Candidate.OpportunityId = Initial.Opportunities[0].OpportunityId;
	Candidate.Activity = CCLAgentTags::Work;
	Candidate.TraitSignals.Add(CCLAgentTags::MaterialGain, 1);
	Input.Candidates.Add(Candidate);
	Candidate.OpportunityId = Initial.Opportunities[4].OpportunityId;
	Candidate.Activity = CCLAgentTags::Help;
	Candidate.TraitSignals.Reset();
	Candidate.TraitSignals.Add(CCLAgentTags::OthersWelfare, 1);
	Input.Candidates.Add(Candidate);
	Candidate.OpportunityId = Initial.Opportunities[3].OpportunityId;
	Candidate.Activity = CCLAgentTags::Rest;
	Candidate.TraitSignals.Reset();
	Candidate.NeedRelief.Add(CCLAgentTags::Fatigue, 1);
	Input.Candidates.Add(Candidate);
	const auto Expected = CCLDecision::Evaluate(Input);
	const auto Actual = CCLMassAgentBridge::Decide(*Manager, Entity, Input);
	TestEqual(TEXT("same input selects same action"), Actual.Intent.OpportunityId, Expected.Intent.OpportunityId);
	TestEqual(TEXT("same score contributions"), Actual.Traces[0].Score, Expected.Traces[0].Score);
	TestTrue(TEXT("Mass result collected before Actor can resume"), CCLMassAgentBridge::Leave(*Manager, Entity,
		Simulation.GetAgents(), Simulation.GetRegistry(), Error));
	TestFalse(TEXT("transient Entity retired"), Manager->IsEntityValid(Entity));
	TestEqual(TEXT("intent retained after handoff"), Simulation.Find(Id)->Intent.OpportunityId, Expected.Intent.OpportunityId);
	TestTrue(TEXT("quiescent state can save"), Simulation.Capture(Pending));
	TestEqual(TEXT("economic state remains single-owned"), Pending.Economy.Accounts.Num(), Initial.Economy.Accounts.Num());
	for (int32 Index = 1; Index < Initial.Agents.Num(); ++Index)
	{
		const auto& Agent = Initial.Agents[Index];
		const auto OtherEntity = CCLMassAgentBridge::Enter(*Manager, Simulation.GetAgents(), Agent.Id);
		Input.Traits = Agent.Features[CCLAgentTags::Feature_Traits].Data.Get<FCCLAgentTraits>();
		Input.Needs.Urgency.Add(CCLAgentTags::Fatigue, Index / 7.f);
		const auto Direct = CCLDecision::Evaluate(Input);
		const auto Mass = CCLMassAgentBridge::Decide(*Manager, OtherEntity, Input);
		TestEqual(TEXT("authored agents preserve choice across adapters"), Mass.Intent.OpportunityId, Direct.Intent.OpportunityId);
		for (int32 Trace = 0; Trace < Direct.Traces.Num(); ++Trace)
		{
			TestEqual(TEXT("every candidate score matches"), Mass.Traces[Trace].Score, Direct.Traces[Trace].Score);
		}
		TestTrue(TEXT("all pending results collected"), CCLMassAgentBridge::Leave(*Manager, OtherEntity,
			Simulation.GetAgents(), Simulation.GetRegistry(), Error));
	}
	Manager->Deinitialize();
	return true;
}
#endif
