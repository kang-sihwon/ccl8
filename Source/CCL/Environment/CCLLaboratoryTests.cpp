#include "CCLExperimentDirector.h"
#include "CCLEcologyExperiment.h"
#include "CCLWorldEnvironmentConfig.h"
#include "Items/CCLFoodState.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLLaboratoryGrowthTest, "CCL.Environment.LaboratoryGrowthContinuity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLLaboratoryGrowthTest::RunTest(const FString& Parameters)
{
	FString Error;
	for (int32 Kind = 0; Kind < 9; ++Kind)
	{
		FKSISurfaceGrid Grid;
		if (!ACCLExperimentDirector::VegetationGrid(0., Grid, Error, Kind, 1))
		{
			AddError(Error);
			return false;
		}
		const FGuid Id = Grid.Vegetation.Cells[0].Id;
		const double Biomass = Grid.Vegetation.Cells[0].LivingBiomassKg;
		Grid.Forcing.TemperatureCelsius = -5.;
		if (!KSISurfaceVegetation::AdvanceCandidate(Grid, 0., 60., 1000., 12., Error))
		{
			AddError(Error);
			return false;
		}
		TestEqual(TEXT("Cold dormancy retains living biomass"), Grid.Vegetation.Cells[0].LivingBiomassKg, Biomass);
		Grid.Forcing.TemperatureCelsius = 20.;
		for (int32 Second = 60; Second < 3600; Second += 60)
		{
			if (!KSISurfaceVegetation::AdvanceCandidate(Grid, Second, 60., 1000., 12., Error))
			{
				AddError(Error);
				return false;
			}
		}
		const auto& Plant = Grid.Vegetation.Cells[0];
		TestTrue(TEXT("One minute at 60x does not restart the life cycle"), Plant.State != EKSIVegetationState::Dead && Plant.RegrowthCount == 0 && Plant.CycleStartedWorldSeconds == 0.);
		TestTrue(TEXT("Warmth preserves the same individual and biomass"), Plant.Id == Id && Plant.LivingBiomassKg >= Biomass);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLLaboratorySelectionTest, "CCL.Environment.LaboratorySelection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLLaboratorySelectionTest::RunTest(const FString& Parameters)
{
	FString Error;
	TestEqual(TEXT("Clock and celestial use one map"), ACCLWorldEnvironmentConfig::LaboratoryMap(0), ACCLWorldEnvironmentConfig::LaboratoryMap(1));
	for (int32 Medium = 0; Medium < 4; ++Medium)
	{
		for (int32 Count : {1, 3, 16})
		{
			FKSIEcologySimulation E;
			if (!TestTrue(TEXT("Selected ecology initializes"), ACCLEcologyExperiment::MakeInitial(FGuid::NewGuid(), 0., E, Error, Medium, Count)))
			{
				AddError(Error);
				return false;
			}
			TestEqual(TEXT("Only one population"), E.GetPopulations().Num(), 1);
			TestEqual(TEXT("Requested living count"), E.LivingCount(ACCLEcologyExperiment::Id(30 + Medium)), int64(Count));
			TestEqual(TEXT("Every requested individual represented"), E.GetIndividuals().Num(), Count);
			TestTrue(TEXT("Conservation"), E.Validate(Error));
		}
	}
	for (int32 Kind = 0; Kind < 9; ++Kind)
	{
		FKSISurfaceGrid Grid;
		TestTrue(TEXT("Selected vegetation initializes"), ACCLExperimentDirector::VegetationGrid(0., Grid, Error, Kind, 16));
		TestEqual(TEXT("Requested patches"), Grid.Vegetation.Cells.Num(), 16);
		for (const auto& C : Grid.Vegetation.Cells)
		{
			TestEqual(TEXT("Only selected kind seeded"), C.SpeciesIndex, Kind);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLLaboratoryRainTest, "CCL.Environment.LaboratoryRainOnset", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLLaboratoryRainTest::RunTest(const FString& Parameters)
{
	FString Error;
	for (double Spacing : {0.125, 0.5, 1.5})
	{
		for (double Scale : {60., 120., 600.})
		{
			for (double RainRate : {0.000005, 0.00002})
			{
				FKSISurfaceGrid Grid;
				Grid.RegionId = FGuid::NewGuid();
				Grid.Size = FIntPoint(8, 8);
				Grid.SpacingMeters = Spacing;
				Grid.Cells.SetNum(64);
				Grid.Forcing.RainMetersPerWorldSecond = RainRate;
				Grid.Forcing.InfiltrationMetersPerWorldSecond = 0.;
				Grid.Forcing.DrainageMetersPerWorldSecond = 0.;
				Grid.Forcing.PhaseMetersPerDegreeWorldSecond = 0.;
				for (auto& Cell : Grid.Cells)
				{
					Cell.BedMeters = 0.2;
				}
				FKSISurfaceSimulation Model;
				if (!Model.Initialize(FGuid::NewGuid(), 0., 0., 0, Error) || !Model.AddRegion(Grid, Error))
				{
					AddError(Error);
					return false;
				}
				FKSIWorldStep Step;
				Step.StepId = 1;
				Step.GameToSeconds = 0.25;
				Step.WorldToSeconds = 0.25 * Scale;
				if (!TestTrue(TEXT("First rain step"), Model.Advance(Step, Error)))
				{
					AddError(Error);
					return false;
				}
				const auto* Wet = Model.FindRegion(Grid.RegionId);
				TestTrue(TEXT("Onset depth equals elapsed world time times rainfall rate"), FMath::IsNearlyEqual(Wet->WaterLevelMeters(0) - 0.2, RainRate * Step.WorldToSeconds, 1.e-9));
				TestTrue(TEXT("Rain conserves mass"), FMath::Abs(Wet->BalanceErrorCubicMeters()) < 1.e-9);
				for (int32 I = 1; I < 120; ++I)
				{
					Step.StepId++;
					Step.GameFromSeconds = Step.GameToSeconds;
					Step.WorldFromSeconds = Step.WorldToSeconds;
					Step.GameToSeconds += 0.25;
					Step.WorldToSeconds += 0.25 * Scale;
					if (!Model.Advance(Step, Error))
					{
						AddError(Error);
						return false;
					}
				}
				Wet = Model.FindRegion(Grid.RegionId);
				TestTrue(TEXT("Accumulation at high scale remains proportional"), FMath::IsNearlyEqual(Wet->WaterLevelMeters(0) - 0.2, RainRate * Step.WorldToSeconds, 1.e-8));
				TestTrue(TEXT("Accumulation conserves mass"), FMath::Abs(Wet->BalanceErrorCubicMeters()) < 1.e-8);
				TestTrue(TEXT("Wind can be controlled without changing rain"), Model.ChangeWind(Grid.RegionId, 8., Error));
				TestEqual(TEXT("Wind observation"), Model.FindRegion(Grid.RegionId)->Weather.WindMetersPerSecond, 8.);
				TestFalse(TEXT("Invalid wind is rejected"), Model.ChangeWind(Grid.RegionId, -1., Error));
				TestEqual(TEXT("Rejected wind preserves state"), Model.FindRegion(Grid.RegionId)->Weather.WindMetersPerSecond, 8.);
			}
		}
	}
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLLaboratoryDecayTest, "CCL.Environment.LaboratoryDecayProgress", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLLaboratoryDecayTest::RunTest(const FString& Parameters)
{
	FString Error;
	FCCLFoodPolicy Policy;
	Policy.ShelfLifeSeconds = 300.;
	Policy.MoldDelaySeconds = 120.;
	Policy.MoldGrowthSeconds = 180.;
	FCCLFoodStorage Humid;
	Humid.TemperatureCelsius = 35.;
	Humid.RelativeHumidity = 1.;
	FCCLFoodStorage Cold = Humid;
	Cold.TemperatureCelsius = -5.;
	FCCLFoodStorage Dry = Humid;
	Dry.TemperatureCelsius = 20.;
	Dry.RelativeHumidity = 0.2;
	FCCLFoodState Food;
	FCCLFoodQuality Before, After;
	if (!CCLFood::Create(FGuid::NewGuid(), FGuid::NewGuid(), 3, 0., Policy, Humid, Food, Error) ||
		!CCLFood::Evaluate(Food, 60., Before, Error) ||
		!CCLFood::ChangeStorage(Food, Cold, 60., Error) ||
		!CCLFood::Evaluate(Food, 60., After, Error))
	{
		AddError(Error);
		return false;
	}
	TestTrue(TEXT("Humid warmth gradually reduces freshness"), Before.Freshness > 0. && Before.Freshness < 1.);
	TestTrue(TEXT("Humid warmth grows mold over time"), Before.Mold > 0. && Before.Mold < 1.);
	TestEqual(TEXT("Changing conditions at paused time preserves freshness"), After.Freshness, Before.Freshness);
	TestEqual(TEXT("Changing conditions at paused time preserves mold"), After.Mold, Before.Mold);
	if (!CCLFood::Evaluate(Food, 360., After, Error))
	{
		AddError(Error);
		return false;
	}
	TestEqual(TEXT("Freezing stops further spoilage without restoration"), After.Freshness, Before.Freshness);
	TestEqual(TEXT("Freezing stops further mold without removal"), After.Mold, Before.Mold);
	if (!CCLFood::ChangeStorage(Food, Dry, 360., Error) || !CCLFood::Evaluate(Food, 420., After, Error))
	{
		AddError(Error);
		return false;
	}
	TestTrue(TEXT("Dry warmth resumes spoilage"), After.Freshness < Before.Freshness);
	TestEqual(TEXT("Dry storage prevents further mold"), After.Mold, Before.Mold);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLLaboratoryVegetationFireTest, "CCL.Environment.LaboratoryVegetationFire", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLLaboratoryVegetationFireTest::RunTest(const FString& Parameters)
{
	FString Error;
	FKSISurfaceGrid Grid;
	FKSISurfaceSimulation Model;
	FKSICelestialSystem Celestial;
	FKSICelestialObserver Observer;
	Observer.BodyId = TEXT("World");
	if (!ACCLExperimentDirector::VegetationGrid(0., Grid, Error, 0, 3)
		|| !Celestial.Initialize(FKSICelestialSystem::MakeDefaultDefinition(42), Error)
		|| !Model.Initialize(FGuid::NewGuid(), 0., 0., 0, Error) || !Model.AddRegion(Grid, Error))
	{
		AddError(Error);
		return false;
	}
	auto Forcing = Grid.Forcing;
	Forcing.EvaporationMetersPerWorldSecond = 0.0001;
	if (!Model.ChangeForcing(Grid.RegionId, Forcing, Error))
	{
		AddError(Error);
		return false;
	}
	FKSIWorldStep Step;
	for (int32 I = 0; I < 60; ++I)
	{
		Step.StepId = Model.GetStepId() + 1;
		Step.GameFromSeconds = Model.GetGameSeconds();
		Step.GameToSeconds = Step.GameFromSeconds + 0.25;
		Step.WorldFromSeconds = Model.GetWorldSeconds();
		Step.WorldToSeconds = Step.WorldFromSeconds + 15.;
		if (!Model.Advance(Step, Error, &Celestial, &Observer))
		{
			AddError(Error);
			return false;
		}
	}
	const auto* Dry = Model.FindRegion(Grid.RegionId);
	TestTrue(TEXT("Drought dries the first vegetation sample"), Dry->Cells[0].SoilCubicMeters < 1.e-9);
	if (!Model.Ignite(Grid.RegionId, 0, 1200000., Error))
	{
		AddError(Error);
		return false;
	}
	Step.StepId++;
	Step.GameFromSeconds = Step.GameToSeconds;
	Step.GameToSeconds += 1.;
	Step.WorldFromSeconds = Step.WorldToSeconds;
	if (!Model.Advance(Step, Error, &Celestial, &Observer))
	{
		AddError(Error);
		return false;
	}
	const auto* Burning = Model.FindRegion(Grid.RegionId);
	TestTrue(TEXT("Dry sample burns while world time is paused"), Burning->Fire.Cells[0].bBurning != 0 && Burning->Fire.Cells[0].BurnedKg > 0.);
	TestTrue(TEXT("Burning conserves heat"), FMath::Abs(Burning->Fire.Cells[0].HeatBalanceError()) < 1.e-6);
	return true;
}
#endif
