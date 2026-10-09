#include "CCLWorldEnvironmentState.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLEnvironmentViewWireTest, "CCL.Environment.View.AtomicWireRoundtrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLEnvironmentViewWireTest::RunTest(const FString& Parameters)
{
	FCCLReplicatedWorldTime Sent;
	Sent.WorldId = FGuid::NewGuid();
	Sent.Epoch = FGuid::NewGuid();
	Sent.WorldSeconds = 12345.;
	Sent.GameSeconds = 205.75;
	Sent.Environment.bValid = 1;
	Sent.Environment.DefinitionId = FGuid::NewGuid();
	Sent.Environment.DefinitionVersion = 2;
	Sent.Environment.InputRevision = 17;
	Sent.Environment.SurfaceRevision = 8;
	Sent.Environment.SurfaceEpoch = FGuid::NewGuid();
	Sent.Environment.Observer.BodyId = TEXT("World");
	Sent.Environment.Observer.LatitudeDegrees = -45.;
	FCCLCelestialSourceView Star;
	Star.BodyId = TEXT("Star");
	Star.SolarHours = 14.25;
	Star.LocalDirection = FVector(0.6, 0., 0.8);
	Sent.Environment.Stars.Add(Star);
	FCCLSurfaceOpening Opening;
	Opening.OpeningId = FGuid::NewGuid();
	Opening.OpenFraction = 0.5;
	Sent.Environment.Openings.Add(Opening);
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes);
	FObjectAndNameAsStringProxyArchive Save(Writer, false);
	bool bSuccess = false;
	TestTrue(TEXT("native serializer handled"), Sent.NetSerialize(Save, nullptr, bSuccess));
	TestTrue(TEXT("serialize succeeded"), bSuccess);
	FCCLReplicatedWorldTime Received;
	FMemoryReader Reader(Bytes);
	FObjectAndNameAsStringProxyArchive Load(Reader, false);
	Received.NetSerialize(Load, nullptr, bSuccess);
	TestTrue(TEXT("deserialize succeeded"), bSuccess);
	TestEqual(TEXT("one message contains exact time"), Received.WorldSeconds, Sent.WorldSeconds);
	TestEqual(TEXT("one message contains matching input revision"), Received.Environment.InputRevision, uint64(17));
	TestEqual(TEXT("execution epoch retained"), Received.Environment.SurfaceEpoch, Sent.Environment.SurfaceEpoch);
	if (TestEqual(TEXT("star retained"), Received.Environment.Stars.Num(), 1))
	{
		TestTrue(TEXT("direction retained"), Received.Environment.Stars[0].LocalDirection == Star.LocalDirection);
		TestEqual(TEXT("local solar time retained"), Received.Environment.Stars[0].SolarHours, 14.25);
	}

	TestEqual(TEXT("opening state retained"), Received.Environment.Openings[0].OpenFraction, 0.5);
	for (int32 Failure = 0; Failure < 2; ++Failure)
	{
		TArray<uint8> Bad = Bytes;
		if (Failure == 0)
		{
			Bad.SetNum(Bad.Num() - 1);
		}
		else
		{
			Bad[0] = 99;
		}

		Received.WorldSeconds = 777.;
		Received.Environment.InputRevision = 999;
		FMemoryReader BadReader(Bad);
		FObjectAndNameAsStringProxyArchive BadLoad(BadReader, false);
		Received.NetSerialize(BadLoad, nullptr, bSuccess);
		TestFalse(TEXT("truncated or unsupported packet rejected"), bSuccess);
		TestEqual(TEXT("time does not partially update"), Received.WorldSeconds, 777.);
		TestEqual(TEXT("environment does not partially update"), Received.Environment.InputRevision, uint64(999));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLEnvironmentViewBudgetTest, "CCL.Environment.View.InterestBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLEnvironmentViewBudgetTest::RunTest(const FString& Parameters)
{
	FCCLReplicatedWorldTime Value;
	Value.Environment.Stars.SetNum(65);
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes);
	FObjectAndNameAsStringProxyArchive Save(Writer, false);
	bool bSuccess = true;
	Value.NetSerialize(Save, nullptr, bSuccess);
	TestFalse(TEXT("oversized interest set is rejected rather than truncated"), bSuccess);
	return true;
}
#endif
