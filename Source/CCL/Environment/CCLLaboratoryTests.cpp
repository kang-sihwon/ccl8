#include "CCLExperimentDirector.h"
#include "CCLEcologyExperiment.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLLaboratorySelectionTest, "CCL.Environment.LaboratorySelection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLLaboratorySelectionTest::RunTest(const FString& Parameters)
{
	FString Error;
	for (int32 Medium = 0; Medium < 4; ++Medium)
	{
		for (int32 Count : {1, 3, 16})
		{
			FKSIEcologySimulation E;
			if (!TestTrue(TEXT("Selected ecology initializes"), ACCLEcologyExperiment::MakeInitial(FGuid::NewGuid(), 0., E, Error, Medium, Count))) { AddError(Error); return false; }
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
		for (const auto& C : Grid.Vegetation.Cells) { TestEqual(TEXT("Only selected kind seeded"), C.SpeciesIndex, Kind); }
	}
	return true;
}
#endif
