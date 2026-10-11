#include "Agents/CCLAgentBiology.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Agents/CCLAgentTags.h"
#include "Agents/CCLLifeSimulation.h"
#include "Agents/CCLMassAgentBridge.h"
#include "Environment/CCLWorldAdvance.h"
#include "KSIWorldClock.h"
#include "Environment/CCLWorldSnapshot.h"
#include "KSIEcologySimulation.h"
#include "MassEntityManager.h"
#include "Misc/AutomationTest.h"

namespace
{
	FGuid BiologyId(uint32 N)
	{
		return FGuid(940, 941, 942, N);
	}

	FCCLSimulationSnapshot BiologyScenario()
	{
		FCCLSimulationSnapshot S;
		for (uint32 I = 1; I <= 2; ++I)
		{
			FKSIAgentRecord R;
			R.Id = BiologyId(I);
			R.DefinitionId = FPrimaryAssetId(TEXT("Agent"), TEXT("AgeTest"));
			FCCLAgentBiology B;
			B.BirthWorldSeconds = -20.;
			B.MaturitySeconds = 10.;
			B.LifespanSeconds = 100.;
			B.bProtectNaturalAging = I == 2;
			R.Features.Add(CCLAgentTags::Feature_Biology, {1, FInstancedStruct::Make(B)});
			R.Features.Add(CCLAgentTags::Feature_Needs, {1, FInstancedStruct::Make(FCCLAgentNeeds())});
			S.Agents.Add(R);
		}

		return S;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLAgentBiologyTimeTest, "CCL.Environment.AgentBiology.WorldTimeLifetimeAndProtection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLAgentBiologyTimeTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLLifeSimulation Life;
	if (!TestTrue(TEXT("initialize"), Life.Initialize(BiologyScenario(), Error)))
	{
		return false;
	}

	FKSIWorldClock Clock;
	TestTrue(TEXT("freeze clock"), Clock.ChangeTimeScale(0., Error));
	TestTrue(TEXT("queue real interval while frozen"), Clock.QueueGameTime(10., false, Error));
	TestTrue(TEXT("frozen commit"), CCLWorldAdvance::Advance(Clock, Life, 10., 100., Error));
	TestEqual(TEXT("frozen age"), CCLAgentBiology::AgeSeconds(*CCLAgentBiology::Find(*Life.Find(BiologyId(1))), Life.GetTime()), 20.);
	TestTrue(TEXT("accelerate"), Clock.ChangeTimeScale(10., Error));
	TestTrue(TEXT("queue accelerated interval"), Clock.QueueGameTime(10., false, Error));
	TestTrue(TEXT("world time advance"), CCLWorldAdvance::Advance(Clock, Life, 10., 100., Error));
	TestFalse(TEXT("ordinary dies at lifespan"), Life.IsAlive(BiologyId(1)));
	const auto* B = CCLAgentBiology::Find(*Life.Find(BiologyId(1)));
	TestEqual(TEXT("exact death timestamp"), B->DeathWorldSeconds, 80.);
	TestEqual(TEXT("dead age stops"), CCLAgentBiology::AgeSeconds(*B, Life.GetTime()), 100.);
	TestTrue(TEXT("quest protection survives natural expiry"), Life.IsAlive(BiologyId(2)));
	TestEqual(TEXT("protected age still increases"), CCLAgentBiology::AgeSeconds(*CCLAgentBiology::Find(*Life.Find(BiologyId(2))), Life.GetTime()), 120.);
	TestFalse(TEXT("protected natural death rejected"), Life.EndLife(BiologyId(2), ECCLAgentDeathCause::NaturalAge, Error));
	TestTrue(TEXT("protection does not block external death"), Life.EndLife(BiologyId(2), ECCLAgentDeathCause::External, Error));
	TestTrue(TEXT("same terminal result is idempotent"), Life.EndLife(BiologyId(2), ECCLAgentDeathCause::External, Error));
	TestFalse(TEXT("conflicting terminal cause rejected"), Life.EndLife(BiologyId(2), ECCLAgentDeathCause::NaturalAge, Error));
	TestFalse(TEXT("dead intent rejected"), Life.SelectIntent(BiologyId(1), {}));
	TestFalse(TEXT("dead movement rejected"), Life.UpdateLocation(BiologyId(1), FVector(100.)));
	auto Manager = MakeShared<FMassEntityManager>();
	FMassEntityManagerStorageInitParams Init;
	Init.Set<FMassEntityManager_InitParams_Concurrent>({1024, 256});
	Manager->Initialize(Init);
	TestFalse(TEXT("dead Mass entry rejected"), CCLMassAgentBridge::Enter(*Manager, Life.GetAgents(), BiologyId(1)).IsValid());
	Manager->Deinitialize();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLAgentBiologySaveTest, "CCL.Environment.AgentBiology.SaveSplitAndFailureAtomicity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLAgentBiologySaveTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLLifeSimulation Whole, Split, Resumed;
	TestTrue(TEXT("whole init"), Whole.Initialize(BiologyScenario(), Error));
	TestTrue(TEXT("split init"), Split.Initialize(BiologyScenario(), Error));
	TArray<uint8> Before, After, Saved;
	Whole.Save(Before);
	TestFalse(TEXT("budget fails after candidate death"), Whole.TryAdvanceTo(100., Error, 1));
	Whole.Save(After);
	TestTrue(TEXT("candidate death and time rolled back"), Before == After);
	TestTrue(TEXT("whole advance"), Whole.TryAdvanceTo(100., Error));
	TestTrue(TEXT("split before boundary"), Split.TryAdvanceTo(79., Error));
	TestTrue(TEXT("save live state"), Split.Save(Saved));
	TestTrue(TEXT("resume"), Resumed.Load(Saved, Error));
	TestTrue(TEXT("advance resumed"), Resumed.TryAdvanceTo(100., Error));
	for (uint32 I = 1; I <= 2; ++I)
	{
		const auto* A = CCLAgentBiology::Find(*Whole.Find(BiologyId(I)));
		const auto* B = CCLAgentBiology::Find(*Resumed.Find(BiologyId(I)));
		TestTrue(TEXT("same terminal cause"), A->DeathCause == B->DeathCause);
		TestEqual(TEXT("same birth"), A->BirthWorldSeconds, B->BirthWorldSeconds);
		TestEqual(TEXT("same death"), A->DeathWorldSeconds, B->DeathWorldSeconds);
	}

	TestTrue(TEXT("save terminal state"), Whole.Save(Saved));
	TestTrue(TEXT("restore terminal state"), Split.Load(Saved, Error));
	TestFalse(TEXT("restoring dead does not resurrect"), Split.IsAlive(BiologyId(1)));
	const auto Lease = Split.GetAgents().Acquire(Split.GetAgents().GetHandle(BiologyId(2)), BiologyId(10));
	TestFalse(TEXT("writer blocks age/time advance"), Split.TryAdvanceTo(200., Error));
	TestEqual(TEXT("writer failure retains time"), Split.GetTime(), 100.);
	Split.GetAgents().Release(Lease);
	TestTrue(TEXT("advance after release"), Split.TryAdvanceTo(200., Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLAgentBiologyValidationTest, "CCL.Environment.AgentBiology.LegacyAndInvalidState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLAgentBiologyValidationTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLLifeSimulation Life;
	auto S = BiologyScenario();
	S.Agents[0].Features.Remove(CCLAgentTags::Feature_Biology);
	TestTrue(TEXT("legacy missing birth remains accepted"), Life.Initialize(S, Error));
	TestTrue(TEXT("legacy advances"), Life.TryAdvanceTo(100., Error));
	TestTrue(TEXT("legacy is not assigned invented age"), CCLAgentBiology::Find(*Life.Find(BiologyId(1))) == nullptr);
	TArray<uint8> Before, After;
	Life.Save(Before);
	S = BiologyScenario();
	auto& B = S.Agents[0].Features[CCLAgentTags::Feature_Biology].Data.GetMutable<FCCLAgentBiology>();
	B.BirthWorldSeconds = 1.;
	TestFalse(TEXT("future birth rejected"), Life.Initialize(S, Error));
	B.BirthWorldSeconds = -100.;
	TestFalse(TEXT("overdue live record rejected"), Life.Initialize(S, Error));
	B.DeathCause = ECCLAgentDeathCause::NaturalAge;
	B.DeathWorldSeconds = 0.;
	B.bProtectNaturalAging = true;
	TestFalse(TEXT("protected natural death inconsistent"), Life.Initialize(S, Error));
	Life.Save(After);
	TestTrue(TEXT("failed initialize leaves prior world intact"), Before == After);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLAgentBiologyTradeTest, "CCL.Environment.AgentBiology.DeadProviderAndDuplicateOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLAgentBiologyTradeTest::RunTest(const FString& Parameters)
{
	FString Error;
	auto S = FCCLLifeSimulation::MerchantScenario(42);
	const FGuid ProviderId = S.Opportunities[0].ProviderId;
	const auto* Provider = S.Agents.FindByPredicate([&](const FKSIAgentRecord& R) { return R.Id == ProviderId; });
	const FGuid BuyerId = S.Agents[0].Id;
	FCCLLifeSimulation Life;
	TestTrue(TEXT("merchant init"), Life.Initialize(S, Error));
	TestTrue(TEXT("live provider baseline transaction"), Life.Execute(BuyerId, S.Opportunities[0].OpportunityId,
		S.Opportunities[0].Revision, BiologyId(29)).bSucceeded != 0);
	const auto Token = Life.ReserveOpportunity(BuyerId, S.Opportunities[0].OpportunityId);
	TestTrue(TEXT("live provider baseline reservation"), Token.IsValid());
	TestTrue(TEXT("provider external death"), Life.EndLife(Provider->Id, ECCLAgentDeathCause::External, Error));
	TestFalse(TEXT("death releases provider reservation"), Life.HasReservation(BuyerId, S.Opportunities[0].OpportunityId, Token));
	TestFalse(TEXT("dead provider cannot execute transaction"), Life.Execute(BuyerId, S.Opportunities[0].OpportunityId,
		S.Opportunities[0].Revision, BiologyId(30)).bSucceeded != 0);
	TestFalse(TEXT("dead provider cannot be reserved"), Life.ReserveOpportunity(BuyerId, S.Opportunities[0].OpportunityId).IsValid());

	FKSIEcologySimulation Ecology;
	FKSIWorldClock Clock;
	TestTrue(TEXT("ecology init"), Ecology.Initialize(BiologyId(90), 0., Error));
	FKSIEcologySpecies Species;
	Species.Id = BiologyId(91);
	FKSIEcologyHabitat Habitat;
	Habitat.Id = BiologyId(92);
	FKSIEcologyPopulation P;
	P.Id = BiologyId(93);
	P.SpeciesId = Species.Id;
	P.HabitatId = Habitat.Id;
	P.Cohorts.Add(BiologyId(94), {BiologyId(94), 0., 5});
	TestTrue(TEXT("ecology fixtures"), Ecology.AddSpecies(Species, Error) && Ecology.AddHabitat(Habitat, Error) && Ecology.AddPopulation(P, Error));
	TestTrue(TEXT("materialize colliding identity"), Ecology.Materialize(BiologyId(95), P.Id, BiologyId(94), BuyerId, true, Error));
	FCCLWorldIdentity Identity;
	Identity.WorldId = BiologyId(90);
	FCCLWorldSnapshot Snapshot;
	TestFalse(TEXT("save rejects duplicate biological ownership"), FCCLWorldSnapshotCodec::Capture(Identity, Clock, Life, Snapshot, Error,
		nullptr, nullptr, &Ecology));
	TestTrue(TEXT("queue"), Clock.QueueGameTime(1., false, Error));
	TestFalse(TEXT("two biology owners rejected"), CCLWorldAdvance::Advance(Clock, Life, 1., 60., Error,
		4096, nullptr, nullptr, nullptr, nullptr, nullptr, &Ecology));
	TestEqual(TEXT("rejected dual ownership retains time"), Life.GetTime(), 0.);
	TestEqual(TEXT("rejected dual ownership retains input"), Clock.GetPendingGameSeconds(), 1.);
	return true;
}
#endif
