#include "CCLEnvironmentInputs.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/Crc.h"

namespace
{
	FCCLEnvironmentInputs SavedFixture()
	{
		FCCLEnvironmentInputs Inputs = FCCLEnvironmentInputsCodec::MakeDefault(123);
		Inputs.Revision = 7;
		Inputs.SurfaceRevision = 5;
		Inputs.Observer.LatitudeDegrees = -35.;
		Inputs.Observer.LongitudeDegrees = 127.;
		Inputs.Observer.AltitudeMeters = 45.;
		FCCLSurfacePatch Roof;
		Roof.SurfaceId = FGuid(1, 2, 3, 4);
		Roof.BodyId = TEXT("World");
		Roof.MaterialId = TEXT("Glass");
		Roof.CenterMeters = FVector3d(5., 7., 10.);
		Roof.HalfExtentsMeters = FVector2d(10., 10.);
		Roof.Transmission.Sun = 0.7;
		Inputs.Surfaces.Add(Roof);
		FCCLSurfaceOpening Skylight;
		Skylight.SurfaceId = Roof.SurfaceId;
		Skylight.OpeningId = FGuid(5, 6, 7, 8);
		Skylight.OpenFraction = 0.5;
		Skylight.SpaceA = FGuid(9, 10, 11, 12);
		Inputs.Openings.Add(Skylight);
		return Inputs;
	}

	void RepairChecksum(TArray<uint8>& Bytes)
	{
		const uint32 CRC = FCrc::MemCrc32(Bytes.GetData(), Bytes.Num() - 4);
		FMemory::Memcpy(Bytes.GetData() + Bytes.Num() - 4, &CRC, 4);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLEnvironmentInputsRoundtripTest, "CCL.Environment.Inputs.RoundtripAndReconstruction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLEnvironmentInputsRoundtripTest::RunTest(const FString& Parameters)
{
	const FCCLEnvironmentInputs Inputs = SavedFixture();
	FString Error;
	TArray<uint8> Bytes;
	TestTrue(TEXT("save input state"), FCCLEnvironmentInputsCodec::Encode(Inputs, Bytes, Error));
	FCCLEnvironmentInputs Restored;
	if (!TestTrue(TEXT("restore input state"), FCCLEnvironmentInputsCodec::Decode(Bytes, Restored, Error)))
	{
		return false;
	}

	TArray<uint8> Again;
	TestTrue(TEXT("encode restored data"), FCCLEnvironmentInputsCodec::Encode(Restored, Again, Error));
	TestTrue(TEXT("all persisted fields roundtrip exactly"), Bytes == Again);
	TestEqual(TEXT("observer latitude preserved"), Restored.Observer.LatitudeDegrees, -35.);
	TestEqual(TEXT("seed preserved"), Restored.Celestial.Seed, 123);
	TestEqual(TEXT("partial opening preserved"), Restored.Openings[0].OpenFraction, 0.5);
	FCCLCelestialSystem A;
	FCCLCelestialSystem B;
	FCCLSurfaceScene SA;
	FCCLSurfaceScene SB;
	FCCLCelestialObservation OA;
	FCCLCelestialObservation OB;
	TestTrue(TEXT("original candidates"), FCCLEnvironmentInputsCodec::Prepare(Inputs, 1234567., A, SA, OA, Error));
	TestTrue(TEXT("reconstructed candidates"), FCCLEnvironmentInputsCodec::Prepare(Restored, 1234567., B, SB, OB, Error));
	TestTrue(TEXT("same observation after rebuild"), OA.ObserverPositionKm == OB.ObserverPositionKm);
	TestEqual(TEXT("same local solar time"), OA.Stars[0].SolarHours, OB.Stars[0].SolarHours);
	TestEqual(TEXT("same incident radiation"), OA.TotalHorizontalIrradianceWattsPerM2, OB.TotalHorizontalIrradianceWattsPerM2);
	TestEqual(TEXT("surface revision preserved"), SB.GetRevision(), uint64(5));
	TestEqual(TEXT("opening area reconstructed"), SB.GetEffectiveOpeningAreaM2(Inputs.Openings[0].SpaceA), 1.);
	TestTrue(TEXT("expected definition accepted"), FCCLEnvironmentInputsCodec::CheckDefinition(Restored, Inputs.Celestial.DefinitionId, 1, Error));
	TestFalse(TEXT("other definition rejected"), FCCLEnvironmentInputsCodec::CheckDefinition(Restored, FGuid(1, 1, 1, 1), 1, Error));
	TestFalse(TEXT("other definition version rejected"), FCCLEnvironmentInputsCodec::CheckDefinition(Restored, Inputs.Celestial.DefinitionId, 2, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLEnvironmentInputsCorruptTest, "CCL.Environment.Inputs.MalformedStorageIsBounded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLEnvironmentInputsCorruptTest::RunTest(const FString& Parameters)
{
	FString Error;
	TArray<uint8> Valid;
	TestTrue(TEXT("valid fixture"), FCCLEnvironmentInputsCodec::Encode(SavedFixture(), Valid, Error));
	TArray<TArray<uint8>> Invalid;
	TArray<uint8> Bad = Valid;
	Bad[20] ^= 1;
	Invalid.Add(Bad);
	Bad = Valid;
	Bad.SetNum(Bad.Num() - 10);
	RepairChecksum(Bad);
	Invalid.Add(Bad);
	Bad = Valid;
	const int32 ExcessiveCount = MAX_int32;
	FMemory::Memcpy(Bad.GetData() + 48, &ExcessiveCount, sizeof(ExcessiveCount));
	RepairChecksum(Bad);
	Invalid.Add(Bad);
	Bad = Valid;
	FMemory::Memcpy(Bad.GetData() + 52, &ExcessiveCount, sizeof(ExcessiveCount));
	RepairChecksum(Bad);
	Invalid.Add(Bad);
	Bad = Valid;
	Bad[4] = 99;
	RepairChecksum(Bad);
	Invalid.Add(Bad);
	Bad = Valid;
	Bad.Insert(0, Bad.Num() - 4);
	RepairChecksum(Bad);
	Invalid.Add(Bad);
	FCCLEnvironmentInputs Output = SavedFixture();
	Output.Revision = 99;
	for (const TArray<uint8>& Candidate : Invalid)
	{
		TestFalse(TEXT("corrupt payload rejected including valid checksum attacks"), FCCLEnvironmentInputsCodec::Decode(Candidate, Output, Error));
		TestEqual(TEXT("failure leaves output untouched"), Output.Revision, uint64(99));
		TestFalse(TEXT("failure includes reason"), Error.IsEmpty());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLEnvironmentInputsAtomicTest, "CCL.Environment.Inputs.PrepareIsAtomicAcrossDomains",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLEnvironmentInputsAtomicTest::RunTest(const FString& Parameters)
{
	FCCLEnvironmentInputs Inputs = SavedFixture();
	FCCLCelestialSystem Celestial;
	FCCLSurfaceScene Surfaces;
	FCCLCelestialObservation Observation;
	FString Error;
	TestTrue(TEXT("initial state"), FCCLEnvironmentInputsCodec::Prepare(Inputs, 10., Celestial, Surfaces, Observation, Error));
	Inputs.SurfaceRevision = 6;
	Inputs.Celestial.Seed = 456;
	Inputs.Surfaces[0].BodyId = TEXT("MissingPlanet");
	TestFalse(TEXT("cross-domain invalid body rejected"), FCCLEnvironmentInputsCodec::Prepare(Inputs, 20., Celestial, Surfaces, Observation, Error));
	TestEqual(TEXT("celestial candidate not partially published"), Celestial.GetDefinition().Seed, 123);
	TestEqual(TEXT("surface candidate not partially published"), Surfaces.GetRevision(), uint64(5));
	TestEqual(TEXT("observation not partially published"), Observation.WorldSeconds, 10.);
	TArray<uint8> Bytes = { 1, 2, 3 };
	TestFalse(TEXT("invalid state cannot be saved"), FCCLEnvironmentInputsCodec::Encode(Inputs, Bytes, Error));
	TestTrue(TEXT("failed save preserves output"), Bytes == TArray<uint8>({ 1, 2, 3 }));
	Inputs = SavedFixture();
	Inputs.Observer.BodyId = TEXT("MissingPlanet");
	TestFalse(TEXT("invalid observer rejected"), FCCLEnvironmentInputsCodec::Prepare(Inputs, 20., Celestial, Surfaces, Observation, Error));
	Inputs = SavedFixture();
	Inputs.Openings[0].OpenFraction = -1.;
	TestFalse(TEXT("invalid opening rejected"), FCCLEnvironmentInputsCodec::Prepare(Inputs, 20., Celestial, Surfaces, Observation, Error));
	TestEqual(TEXT("still retains original celestial state"), Celestial.GetDefinition().Seed, 123);
	Inputs = SavedFixture();
	Inputs.Surfaces[0].MaterialId = FName(FString::ChrN(129, TEXT('A')));
	TestFalse(TEXT("runtime cannot accept a name that persistence cannot encode"),
		FCCLEnvironmentInputsCodec::Prepare(Inputs, 20., Celestial, Surfaces, Observation, Error));
	return true;
}
#endif
