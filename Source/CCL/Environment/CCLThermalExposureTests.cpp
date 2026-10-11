#include "CCLThermalExposure.h"
#include "CCLSurfaceSimulation.h"
#include "CCLSurfaceQuery.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
FCCLSurfaceGrid ThermalGrid(FGuid Id, double Temperature)
{
	FCCLSurfaceGrid G;
	G.RegionId = Id;
	G.Size = FIntPoint(1, 1);
	G.SpacingMeters = 1.;
	G.Cells.SetNum(1);
	G.Fire.Cells.SetNum(1);
	FString Error;
	FCCLCombustionModel::Initialize(G.Fire.Material, 1., Temperature, G.Fire.Cells[0], Error);
	return G;
}
} // namespace
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLThermalSamplingTest, "CCL.Environment.Thermal.ContactAirAndBounds",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLThermalSamplingTest::RunTest(const FString& Parameters)
{
	FCCLSurfaceSimulation S;
	FCCLThermalDamagePolicy P;
	FString E;
	TestTrue(TEXT("initialize"), S.Initialize(FGuid::NewGuid(), 0., 0., 0, E));
	TestTrue(TEXT("hot fuel"), S.AddRegion(ThermalGrid(FGuid(1, 1, 1, 1), 260.), E));
	double Rate = -1.;
	TestTrue(TEXT("sample contact"),
			 CCLThermalExposure::Sample(S, FVector(.5, .5, 1.), P, nullptr, NAME_None, Rate, E));
	TestEqual(TEXT("200 degrees above threshold"), Rate, 10.);
	TestTrue(TEXT("overlapping region"), S.AddRegion(ThermalGrid(FGuid(1, 1, 1, 2), 160.), E));
	CCLThermalExposure::Sample(S, FVector(.5, .5, 1.), P, nullptr, NAME_None, Rate, E);
	TestEqual(TEXT("overlap max, not double counting"), Rate, 10.);
	for (const FVector Pos : {FVector(1., .5, 1.), FVector(.5, .5, 2.01), FVector(.5, .5, -.01)})
	{
		CCLThermalExposure::Sample(S, Pos, P, nullptr, NAME_None, Rate, E);
		TestEqual(TEXT("outside horizontal or vertical contact"), Rate, 0.);
	}
	FCCLAirSpace Room;
	Room.SpaceId = FGuid(2, 2, 2, 2);
	Room.MaximumMeters = FVector(2., 2., 3.);
	Room.VolumeCubicMeters = 12.;
	TestTrue(TEXT("warm air"), Room.Initialize(100., .004, E) && S.AddAirSpace(Room, E));
	CCLThermalExposure::Sample(S, FVector(.5, .5, 1.), P, nullptr, NAME_None, Rate, E);
	TestEqual(TEXT("contact plus room"), Rate, 14.);
	CCLThermalExposure::Sample(S, FVector(1.5, 1.5, 1.), P, nullptr, NAME_None, Rate, E);
	TestEqual(TEXT("air outside fuel"), Rate, 4.);
	P.MaximumDamagePerSecond = 3.;
	CCLThermalExposure::Sample(S, FVector(.5, .5, 1.), P, nullptr, NAME_None, Rate, E);
	TestEqual(TEXT("combined cap"), Rate, 3.);
	P.ContactHeightMeters = -1.;
	TestFalse(TEXT("bad policy"), CCLThermalExposure::Sample(S, FVector(.5, .5, 1.), P, nullptr, NAME_None, Rate, E));
	TestEqual(TEXT("failed sample preserves output"), Rate, 3.);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLThermalBarrierTest, "CCL.Environment.Thermal.CommittedBarrierAndAbsoluteHeight",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLThermalBarrierTest::RunTest(const FString& Parameters)
{
	FCCLSurfaceSimulation S;
	FCCLSurfaceScene Scene;
	FCCLThermalDamagePolicy P;
	FString E;
	auto G = ThermalGrid(FGuid(3, 3, 3, 3), 260.);
	G.OriginMeters.Z = 20.;
	G.Cells[0].BedMeters = 5.;
	G.Cells[0].CeilingMeters = 8.;
	TestTrue(TEXT("initialize raised surface"), S.Initialize(FGuid::NewGuid(), 0., 0., 0, E) && S.AddRegion(G, E));
	double Rate = 0.;
	TestTrue(TEXT("empty scene"), Scene.Replace({}, {}, 1, E));
	TestTrue(TEXT("absolute bed contact"),
			 CCLThermalExposure::Sample(S, FVector(.5, .5, 6.), P, &Scene, TEXT("Earth"), Rate, E));
	TestEqual(TEXT("origin height not added to absolute bed"), Rate, 10.);
	FCCLSurfacePatch Roof;
	Roof.SurfaceId = FGuid(4, 4, 4, 4);
	Roof.BodyId = TEXT("Earth");
	Roof.MaterialId = TEXT("Stone");
	Roof.CenterMeters = FVector(.5, .5, 5.5);
	TestTrue(TEXT("barrier"), Scene.Replace({Roof}, {}, 2, E));
	CCLThermalExposure::Sample(S, FVector(.5, .5, 6.), P, &Scene, TEXT("Earth"), Rate, E);
	TestEqual(TEXT("floor blocks heat contact"), Rate, 0.);
	Roof.Transmission.Wind = .25;
	TestTrue(TEXT("partial barrier"), Scene.Replace({Roof}, {}, 3, E));
	CCLThermalExposure::Sample(S, FVector(.5, .5, 6.), P, &Scene, TEXT("Earth"), Rate, E);
	TestEqual(TEXT("transmitted contact"), Rate, 2.5);
	FCCLSurfaceScene Invalid;
	TestFalse(TEXT("uninitialized geometry rejected"),
			  CCLThermalExposure::Sample(S, FVector(.5, .5, 6.), P, &Invalid, TEXT("Earth"), Rate, E));
	TestEqual(TEXT("failure preserves output"), Rate, 2.5);
	return true;
}
#endif
