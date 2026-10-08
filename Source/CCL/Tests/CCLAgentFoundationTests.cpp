#include "Agents/CCLAgentFeatures.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Agents/CCLAgentSnapshot.h"
#include "Agents/CCLAgentTags.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLAgentLifetimeTest, "CCL.Agent.LifetimeAndMigration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLAgentLifetimeTest::RunTest(const FString& Parameters)
{
	FCCLFeatureRegistry Registry;
	CCLAgentFeatures::Register(Registry);
	FCCLAgentRecord Record;
	Record.Id = FGuid(1, 2, 3, 4);
	Record.DefinitionId = FPrimaryAssetId(TEXT("Agent"), TEXT("Merchant"));
	FCCLAgentTraits Traits;
	Traits.Axes.Add(CCLAgentTags::RiskTolerance, 0.8f);
	Record.Features.Add(CCLAgentTags::Feature_Traits, {1, FInstancedStruct::Make(Traits)});
	FCCLAgentStore Store;
	FString Error;
	const auto Handle = Store.Add(Record, Registry, Error);
	TestTrue(TEXT("registered optional feature creates record"), Handle.Id.IsValid());
	const FGuid ActorWriter(2, 2, 2, 2);
	const FGuid ReducedWriter(3, 3, 3, 3);
	const auto Lease = Store.Acquire(Handle, ActorWriter);
	TestTrue(TEXT("actor obtains sole lease"), Lease.Writer == ActorWriter);
	TestFalse(TEXT("second writer cannot race actor"), Store.Acquire(Handle, ReducedWriter).Writer.IsValid());
	TArray<uint8> Bytes;
	TestFalse(TEXT("save refuses uncollected actor state"), UCCLAgentSnapshot::Encode(Store, Bytes));
	Record.LastSimulatedTime = 300;
	TestTrue(TEXT("collected actor state commits"), Store.Commit(Lease, Record, Registry, Error));
	TestTrue(TEXT("actor releases channels"), Store.Release(Lease));
	const auto ReducedLease = Store.Acquire(Handle, ReducedWriter);
	TestFalse(TEXT("late actor result rejected after handoff"), Store.Commit(Lease, Record, Registry, Error));
	Record.LastSimulatedTime = 600;
	TestTrue(TEXT("reduced simulation progresses time"), Store.Commit(ReducedLease, Record, Registry, Error));
	Store.Release(ReducedLease);
	TestTrue(TEXT("quiescent store saves"), UCCLAgentSnapshot::Encode(Store, Bytes));
	TestTrue(TEXT("snapshot restores"), UCCLAgentSnapshot::Decode(Bytes, Store, Registry, Error));
	TestNull(TEXT("old generation invalid after restore"), Store.Find(Handle));
	const auto NewHandle = Store.GetHandle(Record.Id);
	const auto* Restored = Store.Find(NewHandle);
	TestTrue(TEXT("time and typed traits persist"), Restored && Restored->LastSimulatedTime == 600 &&
		Restored->Features[CCLAgentTags::Feature_Traits].Data.Get<FCCLAgentTraits>().Axes[CCLAgentTags::RiskTolerance] == 0.8f);
	Bytes[Bytes.Num() / 2] ^= 1;
	TestFalse(TEXT("corrupt snapshot rejected"), UCCLAgentSnapshot::Decode(Bytes, Store, Registry, Error));
	TestNotNull(TEXT("failed restore retains previous store"), Store.Find(NewHandle));
	Record.Features[CCLAgentTags::Feature_Traits].Version = 2;
	TestFalse(TEXT("future feature rejected atomically"), Store.Replace({Record}, Registry, Error));
	TestNotNull(TEXT("failed migration retains handle"), Store.Find(NewHandle));

	FCCLFeatureRegistry Migrating;
	FCCLFeatureRegistration Registration;
	Registration.Tag = CCLAgentTags::Feature_Traits;
	Registration.Type = FCCLAgentTraits::StaticStruct();
	Registration.Version = 2;
	Registration.Migrate = [](int32 Version, FInstancedStruct& Data)
	{
		if (Version != 1 || !Data.GetPtr<FCCLAgentTraits>())
		{
			return false;
		}

		Data.GetMutable<FCCLAgentTraits>().Axes.Add(CCLAgentTags::Duty, 0.75f);
		return true;
	};
	Registration.Validate = [](const FInstancedStruct& Data) { return Data.Get<FCCLAgentTraits>().Axes.Contains(CCLAgentTags::Duty); };
	TestTrue(TEXT("migration registered"), Migrating.Register(MoveTemp(Registration)));
	Record.Features[CCLAgentTags::Feature_Traits].Version = 1;
	TestTrue(TEXT("feature migration executes"), Migrating.UpgradeAndValidate(Record, Error));
	TestEqual(TEXT("feature schema advances"), Record.Features[CCLAgentTags::Feature_Traits].Version, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLAgentObservationTest, "CCL.Agent.ObservationAndNeeds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLAgentObservationTest::RunTest(const FString& Parameters)
{
	FCCLAgentExperience Experience;
	FCCLObservation O;
	O.EventId = FGuid(1, 1, 1, 1);
	O.EvidenceId = O.EventId;
	O.EventType = CCLAgentTags::Aid;
	O.PerceivedSubject.Kind = CCLAgentTags::Unknown;
	O.PerceivedSubject.Id = FGuid(2, 2, 2, 2);
	TestFalse(TEXT("unknown observation cannot leak actual identity"), CCLAgentFeatures::Observe(Experience, O));
	O.PerceivedSubject.Kind = CCLAgentTags::Agent;
	TestTrue(TEXT("observed aid recorded"), CCLAgentFeatures::Observe(Experience, O));
	const float Trust = Experience.Relationships[O.PerceivedSubject.Id].Trust;
	Experience.Memories.Reset();
	TestFalse(TEXT("repeated evidence remains deduplicated after forgetting memory"), CCLAgentFeatures::Observe(Experience, O));
	TestEqual(TEXT("rumor does not repeatedly increase trust"), Experience.Relationships[O.PerceivedSubject.Id].Trust, Trust);
	FCCLAgentNeeds Needs;
	Needs.Urgency.Add(CCLAgentTags::Hunger, 0);
	FCCLEmotionState Emotion;
	Emotion.Intensity = 1;
	Emotion.HalfLifeSeconds = 300;
	Needs.Emotions.Add(CCLAgentTags::Fear, Emotion);
	CCLAgentFeatures::AdvanceNeeds(Needs, 300);
	TestEqual(TEXT("emotion decays using elapsed game time"), Needs.Emotions[CCLAgentTags::Fear].Intensity, 0.5f);
	TestTrue(TEXT("hunger rises only for opted-in need"), Needs.Urgency[CCLAgentTags::Hunger] > 0 && Needs.Urgency.Num() == 1);
	return true;
}
#endif
