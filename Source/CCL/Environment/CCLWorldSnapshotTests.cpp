#include "CCLWorldSnapshot.h"

#include "CCLWorldAdvance.h"
#include "Agents/CCLLifeSimulation.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Engine/GameInstance.h"
#include "Session/CCLSessionRecord.h"
#include "Misc/AutomationTest.h"
#include "Misc/Base64.h"
#include "Misc/Crc.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWorldSaveRoundTripTest, "CCL.Environment.Save.PendingAndRates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLWorldSaveRoundTripTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLLifeSimulation Life;
	FCCLWorldClock Clock;
	TestTrue(TEXT("initial life"), Life.Initialize(FCCLLifeSimulation::MerchantScenario(42), Error));
	TestTrue(TEXT("old rate input"), Clock.QueueGameTime(120, false, Error));
	TestTrue(TEXT("partially commit old rate"), CCLWorldAdvance::Advance(Clock, Life, 60, 3600, Error));
	TestTrue(TEXT("new rate"), Clock.ChangeTimeScale(10, Error));
	TestTrue(TEXT("new rate input"), Clock.QueueGameTime(5, false, Error));
	FCCLWorldIdentity Identity;
	Identity.WorldId = FGuid::NewGuid();
	Identity.Generation = 19;
	FCCLWorldSnapshot Saved;
	TestTrue(TEXT("capture pending intervals"), FCCLWorldSnapshotCodec::Capture(Identity, Clock, Life, Saved, Error));
	TArray<uint8> Bytes;
	TestTrue(TEXT("encode"), FCCLWorldSnapshotCodec::Encode(Saved, Bytes, Error));
	FCCLWorldSnapshot Decoded;
	TestTrue(TEXT("decode"), FCCLWorldSnapshotCodec::Decode(Bytes, Decoded, Error));
	TestEqual(TEXT("same world"), Decoded.Identity.WorldId, Identity.WorldId);
	TestEqual(TEXT("same generation"), Decoded.Identity.Generation, uint64(19));
	TestEqual(TEXT("pending old and new rate intervals"), Decoded.Clock.Pending.Num(), 2);
	TestEqual(TEXT("rate history retained"), Decoded.Clock.Rates.Num(), 2);
	FCCLWorldClock ResumedClock;
	FCCLLifeSimulation ResumedLife;
	TestTrue(TEXT("restore"), FCCLWorldSnapshotCodec::Restore(Decoded, Identity.Domain, ResumedClock, ResumedLife, Error));
	TestEqual(TEXT("restore does not add offline time"), ResumedLife.GetTime(), 3600.);
	for (int32 Step = 0; Step < 2; ++Step)
	{
		TestTrue(TEXT("resume pending interval"), CCLWorldAdvance::Advance(ResumedClock, ResumedLife, 1000, 100000, Error));
		TestTrue(TEXT("reference pending interval"), CCLWorldAdvance::Advance(Clock, Life, 1000, 100000, Error));
	}

	TestEqual(TEXT("both rates integrated exactly once"), ResumedClock.GetWorldSeconds(), 7250.);
	TestEqual(TEXT("game time conserved"), ResumedClock.GetGameSeconds(), 125.);
	TestEqual(TEXT("economy and decisions match uninterrupted run"), ResumedLife.DailyReport(), Life.DailyReport());
	TestFalse(TEXT("all input consumed"), ResumedClock.HasPendingTime());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWorldSaveRejectTest, "CCL.Environment.Save.RejectWithoutMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLWorldSaveRejectTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLLifeSimulation Life;
	FCCLWorldClock Clock;
	const auto Initial = FCCLLifeSimulation::MerchantScenario(42);
	TestTrue(TEXT("initialize"), Life.Initialize(Initial, Error));
	FCCLWorldIdentity Identity;
	Identity.WorldId = FGuid::NewGuid();
	FCCLWorldSnapshot Saved;
	TestTrue(TEXT("capture"), FCCLWorldSnapshotCodec::Capture(Identity, Clock, Life, Saved, Error));
	TArray<uint8> Bytes;
	TestTrue(TEXT("encode"), FCCLWorldSnapshotCodec::Encode(Saved, Bytes, Error));
	FCCLWorldSnapshot Output = Saved;
	Output.Identity.Generation = 900;
	TArray<uint8> Corrupt = Bytes;
	Corrupt[40] ^= 1;
	TestFalse(TEXT("bad CRC rejected"), FCCLWorldSnapshotCodec::Decode(Corrupt, Output, Error));
	TestEqual(TEXT("failed decode preserves output"), Output.Identity.Generation, uint64(900));
	// Pending count begins after the fixed 89-byte header; valid CRC cannot bypass allocation bounds.
	Corrupt = Bytes;
	const int32 HugeCount = MAX_int32;
	FMemory::Memcpy(Corrupt.GetData() + 89, &HugeCount, 4);
	uint32 CRC = FCrc::MemCrc32(Corrupt.GetData(), Corrupt.Num() - 4);
	FMemory::Memcpy(Corrupt.GetData() + Corrupt.Num() - 4, &CRC, 4);
	TestFalse(TEXT("oversized array with valid CRC rejected"), FCCLWorldSnapshotCodec::Decode(Corrupt, Output, Error));
	for (int32 Length : {0, 4, 64, 128, Bytes.Num() - 1})
	{
		Corrupt = Bytes;
		Corrupt.SetNum(Length);
		if (Length >= 128)
		{
			CRC = FCrc::MemCrc32(Corrupt.GetData(), Length - 4);
			FMemory::Memcpy(Corrupt.GetData() + Length - 4, &CRC, 4);
		}

		TestFalse(TEXT("truncation rejected"), FCCLWorldSnapshotCodec::Decode(Corrupt, Output, Error));
	}

	TestTrue(TEXT("advance live state"), Clock.QueueGameTime(60, false, Error) && CCLWorldAdvance::Advance(Clock, Life, 60, 3600, Error));
	TArray<uint8> Before;
	TestTrue(TEXT("save live state before rejection"), Life.Save(Before));
	TestFalse(TEXT("experiment cannot replace campaign"), FCCLWorldSnapshotCodec::Restore(Saved, ECCLWorldDomain::Scenario, Clock, Life, Error));
	FCCLWorldSnapshot Mismatch = Saved;
	Mismatch.Clock.WorldSeconds = 100;
	TestFalse(TEXT("different completed times rejected"), FCCLWorldSnapshotCodec::Restore(Mismatch, Identity.Domain, Clock, Life, Error));
	Mismatch = Saved;
	Mismatch.Identity.BaseWorldVersion = 2;
	TestFalse(TEXT("unknown base world rejected"), FCCLWorldSnapshotCodec::Restore(Mismatch, Identity.Domain, Clock, Life, Error));
	const auto Handle = Life.GetAgents().GetHandle(Initial.Agents[0].Id);
	const auto Lease = Life.GetAgents().Acquire(Handle, FGuid::NewGuid());
	TestTrue(TEXT("writer acquired"), Lease.Writer.IsValid());
	TestFalse(TEXT("live writer blocks restore"), FCCLWorldSnapshotCodec::Restore(Saved, Identity.Domain, Clock, Life, Error));
	TestTrue(TEXT("failed restore preserves writer"), Life.GetAgents().Release(Lease));
	TArray<uint8> After;
	TestTrue(TEXT("capture after failures"), Life.Save(After));
	TestTrue(TEXT("all life data unchanged"), Before == After);
	TestEqual(TEXT("clock unchanged"), Clock.GetWorldSeconds(), 3600.);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWorldSaveMigrationTest, "CCL.Environment.Save.LegacySessions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLWorldSaveMigrationTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLLifeSimulation Life;
	TestTrue(TEXT("initialize legacy life"), Life.Initialize(FCCLLifeSimulation::MerchantScenario(42), Error));
	TestTrue(TEXT("legacy completion time"), Life.TryAdvanceTo(7200, Error));
	TArray<uint8> Legacy;
	TestTrue(TEXT("legacy serialization"), Life.Save(Legacy));
	for (int32 Version = 1; Version <= 5; ++Version)
	{
		FCCLSessionRecord Record;
		Record.Version = Version;
		if (Version == 5)
		{
			Record.AgentSimulation = FBase64::Encode(Legacy);
		}

		TArray<uint8> Bytes;
		FCCLSessionRecord Upgraded;
		TestTrue(TEXT("encode legacy session"), FCCLSessionCodec::Encode(Record, Bytes));
		TestTrue(TEXT("migrate session"), FCCLSessionCodec::Decode(Bytes, Upgraded));
		TestEqual(TEXT("current schema"), Upgraded.Version, 6);
		if (Version == 5)
		{
			TArray<uint8> WorldBytes;
			FCCLWorldSnapshot Snapshot;
			TestTrue(TEXT("upgraded world envelope"), FBase64::Decode(Upgraded.AgentSimulation, WorldBytes) &&
				FCCLWorldSnapshotCodec::Decode(WorldBytes, Snapshot, Error));
			TestEqual(TEXT("life time is new clock origin"), Snapshot.Clock.WorldSeconds, 7200.);
			TestEqual(TEXT("no invented elapsed game time"), Snapshot.Clock.GameSeconds, 0.);
			TestTrue(TEXT("no offline backlog"), Snapshot.Clock.Pending.IsEmpty());
			TestTrue(TEXT("world ID assigned"), Snapshot.Identity.WorldId.IsValid());
			TestTrue(TEXT("resave current session"), FCCLSessionCodec::Encode(Upgraded, Bytes));
			FCCLSessionRecord Again;
			TestTrue(TEXT("reload current session"), FCCLSessionCodec::Decode(Bytes, Again));
			TestEqual(TEXT("subsequent loads preserve world identity and bytes"), Again.AgentSimulation, Upgraded.AgentSimulation);
		}
		else
		{
			TestTrue(TEXT("older session retains fresh-world initialization"), Upgraded.AgentSimulation.IsEmpty());
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWorldDomainStoreTest, "CCL.Environment.Save.DomainIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLWorldDomainStoreTest::RunTest(const FString& Parameters)
{
	auto* GameInstance = NewObject<UGameInstance>();
	auto* Store = NewObject<UCCLAgentSessionStore>(GameInstance);
	Store->SnapshotFor(ECCLWorldDomain::Campaign) = {1};
	Store->SnapshotFor(ECCLWorldDomain::Playground) = {2};
	Store->SnapshotFor(ECCLWorldDomain::Scenario) = {3};
	const uint64 ExperimentSession = Store->SessionFor(ECCLWorldDomain::Playground);
	Store->ResetSession();
	TestTrue(TEXT("new campaign clears campaign state"), Store->SnapshotFor(ECCLWorldDomain::Campaign).IsEmpty());
	TestTrue(TEXT("campaign reset preserves experiments"), Store->SnapshotFor(ECCLWorldDomain::Playground) == TArray<uint8>{2} &&
		Store->SnapshotFor(ECCLWorldDomain::Scenario) == TArray<uint8>{3});
	TestEqual(TEXT("campaign reset does not invalidate experiment travel"), Store->SessionFor(ECCLWorldDomain::Playground), ExperimentSession);
	Store->SnapshotFor(ECCLWorldDomain::Campaign) = {4};
	const uint64 CampaignSession = Store->SessionFor(ECCLWorldDomain::Campaign);
	Store->ResetDomain(ECCLWorldDomain::Playground);
	TestTrue(TEXT("selected experiment reset"), Store->SnapshotFor(ECCLWorldDomain::Playground).IsEmpty());
	TestTrue(TEXT("other world bytes preserved"), Store->SnapshotFor(ECCLWorldDomain::Campaign) == TArray<uint8>{4} &&
		Store->SnapshotFor(ECCLWorldDomain::Scenario) == TArray<uint8>{3});
	TestTrue(TEXT("old experiment cannot overwrite reset session"), Store->SessionFor(ECCLWorldDomain::Playground) > ExperimentSession);
	TestEqual(TEXT("campaign travel generation unchanged"), Store->SessionFor(ECCLWorldDomain::Campaign), CampaignSession);
	return true;
}

#endif
