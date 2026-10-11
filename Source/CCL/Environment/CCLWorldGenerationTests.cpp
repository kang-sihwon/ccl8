#include "CCLWorldGenerationStore.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Agents/CCLLifeSimulation.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "KSISurfaceSimulation.h"
#include "KSIEcologySimulation.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLWorldGenerationTest, "CCL.Environment.Save.GenerationRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLWorldGenerationTest::RunTest(const FString& Parameters)
{
    FString LegacyDirectory;
    if (FParse::Value(FCommandLine::Get(), TEXT("CCLLegacyGeneration="), LegacyDirectory))
    {
        FCCLWorldGenerationBundle Legacy;
        FString LegacyError;
        if (!TestTrue(TEXT("recover pre-extraction generation"), FCCLWorldGenerationStore::Recover(
            LegacyDirectory, ECCLWorldDomain::Scenario, Legacy, LegacyError)))
        {
            AddError(LegacyError);
            return false;
        }
        FCCLWorldSnapshot Snapshot;
        FKSIWorldClock LegacyClock;
        FCCLLifeSimulation LegacyLife;
        FKSISurfaceSimulation LegacySurface;
        FKSIEcologySimulation LegacyEcology;
        TestTrue(TEXT("decode pre-extraction world"), FCCLWorldSnapshotCodec::Decode(Legacy.World, Snapshot, LegacyError));
        TestTrue(TEXT("restore pre-extraction time life surface ecology"), FCCLWorldSnapshotCodec::Restore(
            Snapshot, ECCLWorldDomain::Scenario, LegacyClock, LegacyLife, LegacyError, &LegacySurface, &LegacyEcology));
        TestEqual(TEXT("legacy completed world time"), LegacyClock.GetWorldSeconds(), Legacy.Context.WorldSeconds);
        for (const auto& Part : Legacy.Terrain)
        {
            FKSITerrainSnapshot Region;
            FKSITerrainSaveContext Context;
            TestTrue(TEXT("legacy terrain decoded"), FKSITerrainCodec::Decode(Part.Value, Region, Context, LegacyError));
            FKSITerrainStore Store;
            TestTrue(TEXT("legacy terrain restored"), Store.Initialize(Region.Definition, LegacyError)
                && Store.Restore(Part.Value, Legacy.Context, LegacyError));
            TArray<uint8> RoundTrip;
            TestTrue(TEXT("legacy terrain bytes unchanged"), Store.Capture(Legacy.Context, RoundTrip, LegacyError) && RoundTrip == Part.Value);
        }
        AddInfo(FString::Printf(TEXT("CCL_LEGACY_GENERATION restored generation=%llu world=%.9f regions=%d"),
            Legacy.Context.WorldGeneration, LegacyClock.GetWorldSeconds(), Legacy.Terrain.Num()));
    }
    const FString Root = FPaths::ProjectSavedDir() / TEXT("Tests/Generation") / FGuid::NewGuid().ToString(EGuidFormats::Digits);
	FCCLLifeSimulation Life;
	FKSIWorldClock Clock;
	FCCLWorldIdentity Identity;
	Identity.WorldId = FGuid::NewGuid();
	Identity.Domain = ECCLWorldDomain::Scenario;
	FString Error;
	if (!TestTrue(TEXT("life init"), Life.Initialize(FCCLLifeSimulation::MerchantScenario(42), Error)))
	{
		return false;
	}

	FKSITerrainDefinition Definition;
	Definition.WorldId = Identity.WorldId;
	Definition.RegionId = FGuid::NewGuid();
	Definition.DefinitionId = FGuid::NewGuid();
	FKSITerrainStore Terrain;
	TestTrue(TEXT("terrain init"), Terrain.Initialize(Definition, Error));
	TArray<FString> Directories;
	TArray<TArray<uint8>> WorldBytes;
	TMap<FGuid, TArray<uint8>> Regions;
	for (int32 Index = 1; Index <= 3; ++Index)
	{
		Identity.Generation = Index;
		FCCLWorldSnapshot World;
		TestTrue(TEXT("world capture"), FCCLWorldSnapshotCodec::Capture(Identity, Clock, Life, World, Error));
		TArray<uint8> Bytes;
		TestTrue(TEXT("world encode"), FCCLWorldSnapshotCodec::Encode(World, Bytes, Error));
		TestTrue(TEXT("terrain same generation"), Terrain.Capture(FCCLWorldGenerationStore::ContextFor(World), Regions.FindOrAdd(Definition.RegionId), Error));
		FString Directory;
		if (!TestTrue(TEXT("write complete generation"), FCCLWorldGenerationStore::Write(Root, Bytes, Regions, Directory, Error)))
		{
			return false;
		}

		Directories.Add(Directory);
		WorldBytes.Add(Bytes);
	}

	FCCLWorldGenerationBundle Recovered;
	TestTrue(TEXT("recover latest"), FCCLWorldGenerationStore::Recover(Root, Identity.Domain, Recovered, Error));
	TestEqual(TEXT("latest generation"), Recovered.Context.WorldGeneration, uint64(3));
	TestTrue(TEXT("same world bytes"), Recovered.World == WorldBytes[2]);
	TestTrue(TEXT("all regions included"), Recovered.Terrain.Contains(Definition.RegionId));
	const FString LatestRegion = Directories[2] / (Definition.RegionId.ToString(EGuidFormats::Digits) + TEXT(".bin"));
	TArray<uint8> Damaged = Regions.FindChecked(Definition.RegionId);
	Damaged[20] ^= 1;
	TestTrue(TEXT("inject interrupted corrupt region"), FFileHelper::SaveArrayToFile(Damaged, *LatestRegion));
	TestTrue(TEXT("recover preceding generation"), FCCLWorldGenerationStore::Recover(Root, Identity.Domain, Recovered, Error));
	TestEqual(TEXT("corruption falls back to two"), Recovered.Context.WorldGeneration, uint64(2));
	TestTrue(TEXT("remove completion marker"), IFileManager::Get().Delete(*(Directories[1] / TEXT("complete.bin"))));
	TestTrue(TEXT("recover after unfinished publication"), FCCLWorldGenerationStore::Recover(Root, Identity.Domain, Recovered, Error));
	TestEqual(TEXT("uncommitted bundle falls back to one"), Recovered.Context.WorldGeneration, uint64(1));
	TestFalse(TEXT("cross-domain restore rejected"), FCCLWorldGenerationStore::Recover(Root, ECCLWorldDomain::Campaign, Recovered, Error));
	TestEqual(TEXT("failed recovery preserves output"), Recovered.Context.WorldGeneration, uint64(1));
	FString Unwritten;
	TestFalse(TEXT("mixed generation rejected before writing"), FCCLWorldGenerationStore::Write(Root, WorldBytes[0], Regions, Unwritten, Error));
	TestTrue(TEXT("remove last world payload"), IFileManager::Get().Delete(*(Directories[0] / TEXT("world.bin"))));
	TestFalse(TEXT("no valid generation"), FCCLWorldGenerationStore::Recover(Root, Identity.Domain, Recovered, Error));
	TestEqual(TEXT("failure still preserves prior result"), Recovered.Context.WorldGeneration, uint64(1));
	return true;
}
#endif
