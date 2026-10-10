#include "CCLSurfaceSimulation.h"
#include "CCLSurfaceQuery.h"
#include "CCLCelestialSystem.h"
#include "CCLEnvironmentInputs.h"
#include "CCLWorldAdvance.h"
#include "CCLWorldSnapshot.h"
#include "Agents/CCLLifeSimulation.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	FCCLVegetationSpecies PlantSpecies()
	{
		FCCLVegetationSpecies S;
		S.Id = FGuid(960, 1, 1, 1);
		S.GrowthKgPerWorldSecond = 0.01;
		S.WaterCubicMetersPerKg = 0.001;
		S.NutrientKgPerBiomassKg = 0.1;
		S.MinimumWetness = 0.01;
		S.OptimalWetness = 0.5;
		S.FullGrowthIrradiance = 1000.;
		S.LifespanSeconds = 10000.;
		S.DecayPerWorldSecond = 0.;
		return S;
	}

	FCCLSurfaceGrid PlantGrid(int32 Count = 1)
	{
		FCCLSurfaceGrid G;
		G.RegionId = FGuid(960, 2, 1, 1);
		G.Size = FIntPoint(Count, 1);
		G.SpacingMeters = 1.;
		G.Cells.SetNum(Count);
		G.Forcing.TemperatureCelsius = 20.;
		G.Forcing.FlowConductance = 0.;
		G.Forcing.InfiltrationMetersPerWorldSecond = 0.;
		G.Forcing.DrainageMetersPerWorldSecond = 0.;
		G.Forcing.PhaseMetersPerDegreeWorldSecond = 0.;
		G.Fire.SpreadWattsPerKelvin = 0.;
		G.Fire.Material.CoolingWattsPerKelvin = 0.;
		G.Fire.Material.SurfaceWattsPerKelvin = 0.;
		for (auto& C : G.Cells)
		{
			C.SoilCubicMeters = 0.05;
		}
		G.Ledger.InitialCubicMeters = G.TotalCubicMeters();
		return G;
	}

	TArray<FCCLVegetationSeed> PlantSeeds(int32 Count)
	{
		TArray<FCCLVegetationSeed> Seeds;
		for (int32 I = 0; I < Count; ++I)
		{
			FCCLVegetationSeed S;
			S.Id = FGuid(960, 3, 1, I + 1);
			S.SpeciesIndex = 0;
			S.BiomassKg = 0.1;
			S.SoilNutrientsKg = 1.;
			S.SoilOrganicKg = 0.2;
			Seeds.Add(S);
		}
		return Seeds;
	}

	FCCLWorldStep PlantStep(const FCCLSurfaceSimulation& S, double WorldDelta, double GameDelta = 0.25)
	{
		FCCLWorldStep Step;
		Step.StepId = S.GetStepId() + 1;
		Step.GameFromSeconds = S.GetGameSeconds();
		Step.GameToSeconds = Step.GameFromSeconds + GameDelta;
		Step.WorldFromSeconds = S.GetWorldSeconds();
		Step.WorldToSeconds = Step.WorldFromSeconds + WorldDelta;
		return Step;
	}

	TArray<uint8> PlantBytes(const FCCLSurfaceSimulation& S)
	{
		TArray<uint8> Bytes;
		FString Error;
		S.Capture(Bytes, Error);
		return Bytes;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLVegetationResourcesTest, "CCL.Environment.Vegetation.ResourcesLightAndFungi",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLVegetationResourcesTest::RunTest(const FString& Parameters)
{
	FString Error;
	auto G = PlantGrid(5);
	auto Fungus = PlantSpecies();
	Fungus.Id.D = 2;
	Fungus.Kind = ECCLVegetationKind::Fungus;
	Fungus.MinimumDaylightHours = 0.;
	auto Seeds = PlantSeeds(5);
	Seeds[2].SpeciesIndex = Seeds[3].SpeciesIndex = 1;
	Seeds[3].SoilOrganicKg = 0.;
	Seeds[4].SoilNutrientsKg = 0.;
	G.Cells[1].SunExposure = G.Cells[2].SunExposure = G.Cells[3].SunExposure = 0.;
	if (!TestTrue(TEXT("seed patches"), CCLSurfaceVegetation::Initialize(G, {PlantSpecies(), Fungus}, Seeds, 0., Error))) return false;
	TestTrue(TEXT("advance biological resources"), CCLSurfaceVegetation::AdvanceCandidate(G, 0., 100., 1000., 12., Error));
	const auto& C = G.Vegetation.Cells;
	TestTrue(TEXT("lit plant grows"), C[0].GrownBiomassKg > 0.9);
	TestEqual(TEXT("shade blocks photosynthesis"), C[1].GrownBiomassKg, 0.);
	TestTrue(TEXT("fungus consumes finite organic matter"), FMath::IsNearlyEqual(C[2].GrownBiomassKg, 0.2, 1.e-10) && C[2].SoilOrganicKg == 0.);
	TestEqual(TEXT("fungus cannot grow without substrate"), C[3].GrownBiomassKg, 0.);
	TestEqual(TEXT("plant cannot invent nutrients"), C[4].GrownBiomassKg, 0.);
	TestTrue(TEXT("water consumed rather than a visual-only growth"), G.Cells[0].SoilCubicMeters < 0.05);
	TestTrue(TEXT("whole region water and fuel balance"), FCCLSurfaceSimulation::ValidateRegion(G, Error));
	TestTrue(TEXT("transpiration leaves water ledger once"), FMath::Abs(G.BalanceErrorCubicMeters()) < 1.e-10);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLVegetationLifecycleTest, "CCL.Environment.Vegetation.SeasonDroughtDeathAndRegrowth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLVegetationLifecycleTest::RunTest(const FString& Parameters)
{
	FString Error;
	auto G = PlantGrid();
	auto S = PlantSpecies();
	S.LifespanSeconds = 10.;
	S.RegrowthDelaySeconds = 5.;
	TestTrue(TEXT("initialize life"), CCLSurfaceVegetation::Initialize(G, {S}, PlantSeeds(1), 0., Error));
	TestTrue(TEXT("short day dormancy"), CCLSurfaceVegetation::AdvanceCandidate(G, 0., 5., 1000., 4., Error));
	TestTrue(TEXT("dormant survives with unchanged biomass"), G.Vegetation.Cells[0].State == ECCLVegetationState::Dormant && G.Vegetation.Cells[0].GrownBiomassKg == 0.);
	TestTrue(TEXT("reach exact natural death"), CCLSurfaceVegetation::AdvanceCandidate(G, 5., 5., 1000., 12., Error));
	TestEqual(TEXT("death timestamp"), G.Vegetation.Cells[0].DeathWorldSeconds, 10.);
	TestEqual(TEXT("no living biomass at death"), G.Vegetation.Cells[0].LivingBiomassKg, 0.);
	TestTrue(TEXT("wait until regeneration boundary"), CCLSurfaceVegetation::AdvanceCandidate(G, 10., 5., 1000., 12., Error));
	TestEqual(TEXT("waiting does not refill"), G.Vegetation.Cells[0].LivingBiomassKg, 0.);
	TestTrue(TEXT("regrow using resources"), CCLSurfaceVegetation::AdvanceCandidate(G, 15., 5., 1000., 12., Error));
	TestEqual(TEXT("one new cycle"), G.Vegetation.Cells[0].RegrowthCount, uint32(1));
	TestTrue(TEXT("regrowth excludes original seed biomass"), G.Vegetation.Cells[0].LivingBiomassKg > 0. && G.Vegetation.Cells[0].LivingBiomassKg < 0.051);
	TestTrue(TEXT("all old biomass retained as litter"), G.Vegetation.Cells[0].DeadBiomassKg >= 0.149);
	auto Dry = PlantGrid();
	Dry.Cells[0].SoilCubicMeters = Dry.Ledger.InitialCubicMeters = 0.;
	S.StressDeathSeconds = 3.;
	TestTrue(TEXT("dry seed fixture"), CCLSurfaceVegetation::Initialize(Dry, {S}, PlantSeeds(1), 0., Error));
	TestTrue(TEXT("drought beyond death"), CCLSurfaceVegetation::AdvanceCandidate(Dry, 0., 20., 1000., 12., Error));
	TestEqual(TEXT("drought exact death"), Dry.Vegetation.Cells[0].DeathWorldSeconds, 3.);
	TestEqual(TEXT("no regeneration without water"), Dry.Vegetation.Cells[0].RegrowthCount, uint32(0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLVegetationFireTest, "CCL.Environment.Vegetation.FireAndDecayConservation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLVegetationFireTest::RunTest(const FString& Parameters)
{
	FString Error;
	auto G = PlantGrid();
	G.Cells[0].SoilCubicMeters = G.Ledger.InitialCubicMeters = 0.;
	TestTrue(TEXT("seed fuel"), CCLSurfaceVegetation::Initialize(G, {PlantSpecies()}, PlantSeeds(1), 0., Error));
	FCCLSurfaceSimulation Surface;
	TestTrue(TEXT("world"), Surface.Initialize(FGuid(960, 4, 1, 1), 0., 0., 0, Error) && Surface.AddRegion(G, Error));
	TestTrue(TEXT("severe heat"), Surface.Ignite(G.RegionId, 0, 800000., Error));
	TestTrue(TEXT("actual fire while biological time frozen"), Surface.Advance(PlantStep(Surface, 0., 1.), Error));
	const auto& C = Surface.FindRegion(G.RegionId)->Vegetation.Cells[0];
	TestTrue(TEXT("fire kills roots"), C.State == ECCLVegetationState::Dead && !C.bRootsViable);
	TestTrue(TEXT("fuel actually burned"), C.BurnedBiomassKg > 0.);
	TestEqual(TEXT("frozen world time cannot grow fuel"), C.GrownBiomassKg, 0.);
	TestTrue(TEXT("burned nutrients returned"), C.SoilNutrientsKg > 1.);
	TestTrue(TEXT("surface balances after fire"), Surface.Validate(Error));
	auto Litter = PlantGrid();
	auto S = PlantSpecies();
	S.LifespanSeconds = 1.;
	S.bRegrows = 0;
	S.DecayPerWorldSecond = 0.1;
	TestTrue(TEXT("litter fixture"), CCLSurfaceVegetation::Initialize(Litter, {S}, PlantSeeds(1), 0., Error));
	TestTrue(TEXT("death and decomposition"), CCLSurfaceVegetation::AdvanceCandidate(Litter, 0., 20., 0., 12., Error));
	TestTrue(TEXT("organic matter and nutrients return"), Litter.Vegetation.Cells[0].SoilOrganicKg > 0.28 && Litter.Vegetation.Cells[0].SoilNutrientsKg > 1.008);
	TestTrue(TEXT("decay fuel accounting"), FCCLSurfaceSimulation::ValidateRegion(Litter, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLVegetationRestoreTest, "CCL.Environment.Vegetation.CommonClockSaveResumeAndRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLVegetationRestoreTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLWorldIdentity Identity;
	Identity.WorldId = FGuid(960, 4, 1, 1);
	FCCLWorldClock Clock;
	FCCLLifeSimulation Life;
	FCCLSurfaceSimulation Surface, Restored;
	FCCLCelestialSystem Celestial;
	FCCLCelestialObserver Observer;
	Observer.BodyId = TEXT("World");
	auto G = PlantGrid();
	if (!TestTrue(TEXT("initialize owners"), Celestial.Initialize(FCCLCelestialSystem::MakeDefaultDefinition(42), Error)
		&& Life.Initialize(FCCLLifeSimulation::MerchantScenario(42), Error)
		&& Surface.Initialize(Identity.WorldId, 0., 0., 0, Error)
		&& CCLSurfaceVegetation::Initialize(G, {PlantSpecies()}, PlantSeeds(1), 0., Error)
		&& Surface.AddRegion(G, Error))) return false;
	const auto Before = PlantBytes(Surface);
	TestFalse(TEXT("missing solar input is rejected"), Surface.Advance(PlantStep(Surface, 60.), Error));
	TestTrue(TEXT("missing context keeps prior state"), Before == PlantBytes(Surface));
	TestTrue(TEXT("queue world time"), Clock.QueueGameTime(120., false, Error));
	TestFalse(TEXT("Agent budget rejects prepared plant progression"), CCLWorldAdvance::Advance(Clock, Life, 120., 7200., Error, 1, &Surface, &Celestial, &Observer));
	TestTrue(TEXT("whole surface rolled back"), Before == PlantBytes(Surface));
	TestEqual(TEXT("clock input retained"), Clock.GetPendingGameSeconds(), 120.);
	if (!TestTrue(TEXT("retry world interval"), CCLWorldAdvance::Advance(Clock, Life, 120., 7200., Error, 256, &Surface, &Celestial, &Observer)))
	{
		AddError(Error);
		return false;
	}
	FCCLWorldSnapshot Snapshot;
	TestTrue(TEXT("capture world vegetation"), FCCLWorldSnapshotCodec::Capture(Identity, Clock, Life, Snapshot, Error, nullptr, &Surface));
	TestTrue(TEXT("independent restore"), Restored.Restore(Snapshot.Surface, Error));
	TestTrue(TEXT("same serialized generation"), PlantBytes(Surface) == PlantBytes(Restored));
	TestTrue(TEXT("advance uninterrupted and resumed"), Surface.Advance(PlantStep(Surface, 60.), Error, &Celestial, &Observer)
		&& Restored.Advance(PlantStep(Restored, 60.), Error, &Celestial, &Observer));
	TestTrue(TEXT("resume produces identical future"), PlantBytes(Surface) == PlantBytes(Restored));
	auto Bad = Snapshot.Surface;
	Bad[Bad.Num() / 2] ^= 1;
	const auto Committed = PlantBytes(Restored);
	TestFalse(TEXT("corrupt vegetation snapshot rejected"), Restored.Restore(Bad, Error));
	TestTrue(TEXT("corrupt restore keeps previous generation"), Committed == PlantBytes(Restored));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLVegetationTerrainTest, "CCL.Environment.Vegetation.TerrainSupportAtomicity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLVegetationTerrainTest::RunTest(const FString& Parameters)
{
	FString Error;
	auto G = PlantGrid();
	G.TerrainId = FGuid(960, 5, 1, 1);
	G.TerrainRevision = 1;
	G.Cells[0].WaterCubicMeters = 1.;
	G.Ledger.InitialCubicMeters = G.TotalCubicMeters();
	TestTrue(TEXT("seed terrain"), CCLSurfaceVegetation::Initialize(G, {PlantSpecies()}, PlantSeeds(1), 0., Error));
	FCCLSurfaceSimulation Surface;
	TestTrue(TEXT("world and terrain"), Surface.Initialize(FGuid(960, 4, 1, 1), 0., 0., 0, Error) && Surface.AddRegion(G, Error));
	const auto Before = PlantBytes(Surface);
	TestFalse(TEXT("blocked displacement rejects changed support"), Surface.RebaseTerrain(G.RegionId, {31.5}, 2, Error));
	TestTrue(TEXT("rejected terrain retains living plants"), Before == PlantBytes(Surface));
	TestTrue(TEXT("dig removes support"), Surface.RebaseTerrain(G.RegionId, {-1.}, 2, Error));
	const auto& C = Surface.FindRegion(G.RegionId)->Vegetation.Cells[0];
	TestTrue(TEXT("support loss kills roots"), C.State == ECCLVegetationState::Dead && !C.bRootsViable);
	TestEqual(TEXT("terrain edit preserves biomass as litter"), C.DeadBiomassKg, 0.1);
	TestTrue(TEXT("changed terrain and biology agree"), Surface.Validate(Error));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLVegetationIndoorTest, "CCL.Environment.Vegetation.IndoorTranspirationConservesWaterAndHeat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLVegetationIndoorTest::RunTest(const FString& Parameters)
{
	FString Error;
	auto G = PlantGrid();
	auto S = PlantSpecies();
	S.Kind = ECCLVegetationKind::Fungus;
	S.MinimumDaylightHours = 0.;
	G.Cells[0].SunExposure = 0.;
	TestTrue(TEXT("seed indoor fungus"), CCLSurfaceVegetation::Initialize(G, {S}, PlantSeeds(1), 0., Error));
	FCCLAirSpace Room;
	Room.SpaceId = FGuid(960, 6, 1, 1);
	Room.MinimumMeters = FVector(-1.);
	Room.MaximumMeters = FVector(2.);
	Room.VolumeCubicMeters = 27.;
	TestTrue(TEXT("room initial water and heat"), Room.Initialize(20., 0.004, Error));
	FCCLSurfaceSimulation Surface;
	FCCLCelestialSystem Celestial;
	FCCLCelestialObserver Observer;
	Observer.BodyId = TEXT("World");
	TArray<FCCLSurfaceOpening> Openings;
	TestTrue(TEXT("coupled owners"), Surface.Initialize(FGuid(960, 4, 1, 1), 0., 0., 0, Error)
		&& Surface.AddRegion(G, Error) && Surface.AddAirSpace(Room, Error)
		&& Celestial.Initialize(FCCLCelestialSystem::MakeDefaultDefinition(42), Error));
	if (!TestTrue(TEXT("indoor growth"), Surface.Advance(PlantStep(Surface, 60.), Error, &Celestial, &Observer, nullptr, &Openings)))
	{
		AddError(Error);
		return false;
	}
	const auto& R = *Surface.FindRegion(G.RegionId);
	const auto& Air = *Surface.GetAirSimulation().FindSpace(Room.SpaceId);
	TestTrue(TEXT("fungus actually grows"), R.Vegetation.Cells[0].GrownBiomassKg > 0.);
	TestTrue(TEXT("all removed water enters closed room"), FMath::IsNearlyEqual(Air.Contents.VaporKg - Room.Initial.VaporKg,
		R.Ledger.EvaporatedCubicMeters * 1000., 1.e-9));
	TestTrue(TEXT("room supplies latent heat"), FMath::IsNearlyEqual(Air.Contents.HeatJoules + R.AirHeatToSurfaceJoules,
		Room.Initial.HeatJoules, 1.e-5));
	TestTrue(TEXT("indoor state validates"), Surface.Validate(Error));
	return true;
}
#endif
