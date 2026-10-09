#include "CCLSurfaceQuery.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include <limits>

namespace
{
	FCCLSurfacePatch Floor(int32 Id, double Height)
	{
		FCCLSurfacePatch Patch;
		Patch.SurfaceId = FGuid(1, 0, 0, Id);
		Patch.BodyId = TEXT("World");
		Patch.CenterMeters.Z = Height;
		Patch.HalfExtentsMeters = FVector2d(10., 10.);
		Patch.MaterialId = TEXT("Stone");
		return Patch;
	}

	FCCLSurfacePatch Wall()
	{
		FCCLSurfacePatch Patch = Floor(3, 2.);
		Patch.Normal = FVector3d::XAxisVector;
		Patch.TangentU = FVector3d::YAxisVector;
		Patch.TangentV = FVector3d::ZAxisVector;
		Patch.HalfExtentsMeters = FVector2d(4., 2.);
		return Patch;
	}

	FCCLSurfaceOpening Door()
	{
		FCCLSurfaceOpening Opening;
		Opening.OpeningId = FGuid(2, 0, 0, 1);
		Opening.SurfaceId = FGuid(1, 0, 0, 3);
		Opening.HalfExtentsMeters = FVector2d(1., 2.);
		Opening.SpaceA = FGuid(3, 0, 0, 1);
		return Opening;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLSurfaceLayersTest, "CCL.Environment.Surface.MultipleLayersAndFilters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLSurfaceLayersTest::RunTest(const FString& Parameters)
{
	FCCLSurfacePatch Ground = Floor(1, 0.);
	Ground.Kind = ECCLSurfaceKind::Terrain;
	Ground.MaterialId = TEXT("Soil");
	FCCLSurfacePatch RoofTop = Floor(2, 10.);
	FCCLSurfacePatch RoofBottom = Floor(3, 9.5);
	RoofBottom.Normal = -FVector3d::ZAxisVector;
	RoofBottom.TangentV = -FVector3d::YAxisVector;
	FCCLSurfacePatch OtherPlanet = Floor(4, 15.);
	OtherPlanet.BodyId = TEXT("Other");
	FCCLSurfaceScene Scene;
	FString Error;
	TestTrue(TEXT("layered scene"), Scene.Replace({ Ground, RoofBottom, RoofTop, OtherPlanet }, {}, 1, Error));
	FCCLSurfaceQuery Query;
	Query.BodyId = TEXT("World");
	Query.OriginMeters.Z = 20.;
	Query.Direction = -FVector3d::ZAxisVector;
	TArray<FCCLSurfaceSample> Samples;
	TestTrue(TEXT("downward query"), Scene.QuerySurfaces(Query, Samples, Error));
	if (TestEqual(TEXT("same XY has three independent surfaces"), Samples.Num(), 3))
	{
		TestEqual(TEXT("nearest roof first"), Samples[0].SurfaceId, RoofTop.SurfaceId);
		TestEqual(TEXT("underside has stable identity"), Samples[1].SurfaceId, RoofBottom.SurfaceId);
		TestEqual(TEXT("ground survives under roof"), Samples[2].MaterialId, FName(TEXT("Soil")));
	}

	Query.OriginMeters.Z = 5.;
	Query.Direction = FVector3d::ZAxisVector;
	TestTrue(TEXT("upward query"), Scene.QuerySurfaces(Query, Samples, Error));
	TestEqual(TEXT("ceiling underside is nearest from inside"), Samples[0].SurfaceId, RoofBottom.SurfaceId);
	TestTrue(TEXT("stored normal is not flipped toward ray"), Samples[0].Normal == -FVector3d::ZAxisVector);
	Query.OriginMeters.Z = 20.;
	Query.Direction = -FVector3d::ZAxisVector;
	Query.KindMask = static_cast<uint8>(ECCLSurfaceKind::Terrain);
	TestTrue(TEXT("terrain filter"), Scene.QuerySurfaces(Query, Samples, Error));
	TestEqual(TEXT("filter excludes structures"), Samples.Num(), 1);
	TestEqual(TEXT("terrain result"), Samples[0].SurfaceId, Ground.SurfaceId);
	Query.BodyId = TEXT("Other");
	Query.KindMask = 15;
	TestTrue(TEXT("other planetary region"), Scene.QuerySurfaces(Query, Samples, Error));
	TestEqual(TEXT("body ID isolates geometry"), Samples.Num(), 1);
	TestEqual(TEXT("other body surface"), Samples[0].SurfaceId, OtherPlanet.SurfaceId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLShelterDoorTest, "CCL.Environment.Surface.RoofWallAndOpening",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLShelterDoorTest::RunTest(const FString& Parameters)
{
	FCCLSurfaceScene Scene;
	FCCLSurfacePatch Roof = Floor(1, 4.);
	FCCLSurfaceOpening Opening = Door();
	FString Error;
	TestTrue(TEXT("closed room fixture"), Scene.Replace({ Roof, Wall() }, { Opening }, 1, Error));
	FCCLShelterQuery Query;
	Query.BodyId = TEXT("World");
	Query.PositionMeters = FVector3d(-2., 0., 1.5);
	FCCLShelterSample Sample;
	TestTrue(TEXT("closed door shelter"), FCCLShelterEvaluator::Evaluate(Scene, Query, Sample, Error));
	TestEqual(TEXT("roof blocks sunlight"), Sample.Transmission.Sun, 0.);
	TestEqual(TEXT("roof blocks rain"), Sample.Transmission.Precipitation, 0.);
	TestEqual(TEXT("wall blocks wind"), Sample.Transmission.Wind, 0.);
	Opening.OpenFraction = 1.;
	TestTrue(TEXT("open door publishes new revision"), Scene.Replace({ Roof, Wall() }, { Opening }, 2, Error));
	TestTrue(TEXT("open door shelter"), FCCLShelterEvaluator::Evaluate(Scene, Query, Sample, Error));
	TestEqual(TEXT("door admits wind"), Sample.Transmission.Wind, 1.);
	TestEqual(TEXT("door does not remove roof rain cover"), Sample.Transmission.Precipitation, 0.);
	TestEqual(TEXT("open area supports later ventilation"), Scene.GetEffectiveOpeningAreaM2(Opening.SpaceA), 8.);
	Opening.OpenFraction = 0.5;
	TestTrue(TEXT("partially open door"), Scene.Replace({ Roof, Wall() }, { Opening }, 3, Error));
	Query.PositionMeters.Y = -0.5;
	TestTrue(TEXT("open half query"), FCCLShelterEvaluator::Evaluate(Scene, Query, Sample, Error));
	TestEqual(TEXT("low U half is open"), Sample.Transmission.Wind, 1.);
	Query.PositionMeters.Y = 0.5;
	TestTrue(TEXT("closed half query"), FCCLShelterEvaluator::Evaluate(Scene, Query, Sample, Error));
	TestEqual(TEXT("remaining panel still blocks wind"), Sample.Transmission.Wind, 0.);
	TestEqual(TEXT("partial opening area"), Scene.GetEffectiveOpeningAreaM2(Opening.SpaceA), 4.);
	Query.ToPrecipitationSource = FVector3d::XAxisVector;
	Query.PositionMeters.Y = -0.5;
	TestTrue(TEXT("wind-driven rain through door"), FCCLShelterEvaluator::Evaluate(Scene, Query, Sample, Error));
	TestEqual(TEXT("horizontal rain enters open aperture"), Sample.Transmission.Precipitation, 1.);
	TestEqual(TEXT("overhead light still blocked"), Sample.Transmission.Sun, 0.);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLShelterMaterialTest, "CCL.Environment.Surface.MaterialTransmissionAndCoverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLShelterMaterialTest::RunTest(const FString& Parameters)
{
	FCCLSurfacePatch Glass = Floor(1, 4.);
	Glass.Transmission.Sun = 0.8;
	FCCLSurfaceScene Scene;
	FString Error;
	TestTrue(TEXT("glass roof"), Scene.Replace({ Glass }, {}, 1, Error));
	FCCLShelterQuery Query;
	Query.BodyId = TEXT("World");
	Query.PositionMeters.Z = 1.;
	Query.ToWindSource = FVector3d::ZAxisVector;
	FCCLShelterSample Sample;
	TestTrue(TEXT("glass query"), FCCLShelterEvaluator::Evaluate(Scene, Query, Sample, Error));
	TestEqual(TEXT("glass admits light"), Sample.Transmission.Sun, 0.8);
	TestEqual(TEXT("glass blocks water"), Sample.Transmission.Precipitation, 0.);
	TestEqual(TEXT("glass blocks wind"), Sample.Transmission.Wind, 0.);
	FCCLSurfacePatch SecondGlass = Glass;
	SecondGlass.SurfaceId = FGuid(1, 0, 0, 2);
	SecondGlass.CenterMeters.Z = 5.;
	TestTrue(TEXT("two glass layers"), Scene.Replace({ Glass, SecondGlass }, {}, 2, Error));
	TestTrue(TEXT("layered transmission"), FCCLShelterEvaluator::Evaluate(Scene, Query, Sample, Error));
	TestTrue(TEXT("transmission multiplies through layers"), FMath::IsNearlyEqual(Sample.Transmission.Sun, 0.64, 1.e-12));
	Glass.Transmission.Sun = 0.;
	Glass.HalfExtentsMeters = FVector2d(1., 1.);
	TestTrue(TEXT("small roof"), Scene.Replace({ Glass }, {}, 3, Error));
	Query.PositionMeters.X = 0.75;
	Query.ProbeRadiusMeters = 0.5;
	TestTrue(TEXT("roof edge footprint"), FCCLShelterEvaluator::Evaluate(Scene, Query, Sample, Error));
	TestTrue(TEXT("one of five samples exposed"), FMath::IsNearlyEqual(Sample.Transmission.Precipitation, 0.2, 1.e-12));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLSurfaceAtomicTest, "CCL.Environment.Surface.InvalidReplacementAndQueryAreAtomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLSurfaceAtomicTest::RunTest(const FString& Parameters)
{
	FCCLSurfaceScene Scene;
	FCCLSurfacePatch Patch = Wall();
	FCCLSurfaceOpening Opening = Door();
	FString Error;
	TestTrue(TEXT("initial scene"), Scene.Replace({ Patch }, { Opening }, 1, Error));
	TestFalse(TEXT("revision must increase"), Scene.Replace({ Patch }, {}, 1, Error));
	TestFalse(TEXT("duplicate surfaces rejected"), Scene.Replace({ Patch, Patch }, {}, 2, Error));
	Opening.OpenFraction = 2.;
	TestFalse(TEXT("invalid opening fraction"), Scene.Replace({ Patch }, { Opening }, 2, Error));
	Opening = Door();
	Opening.CenterUV.X = 4.;
	TestFalse(TEXT("opening outside wall"), Scene.Replace({ Patch }, { Opening }, 2, Error));
	Opening = Door();
	FCCLSurfaceOpening Duplicate = Opening;
	Duplicate.OpeningId.D = 2;
	TestFalse(TEXT("overlap cannot double count opening area"), Scene.Replace({ Patch }, { Opening, Duplicate }, 2, Error));
	Patch.TangentU = Patch.Normal;
	TestFalse(TEXT("invalid tangent frame"), Scene.Replace({ Patch }, {}, 2, Error));
	Patch = Wall();
	Patch.Transmission.Sun = std::numeric_limits<double>::quiet_NaN();
	TestFalse(TEXT("nonfinite material value"), Scene.Replace({ Patch }, {}, 2, Error));
	TestEqual(TEXT("failed replacements retain revision"), Scene.GetRevision(), uint64(1));
	TestEqual(TEXT("failed replacement retains closed door"), Scene.GetOpenings()[0].OpenFraction, 0.);
	FCCLSurfaceQuery Query;
	Query.BodyId = TEXT("World");
	Query.OriginMeters = FVector3d(-2., 0., 1.);
	Query.Direction = FVector3d::XAxisVector;
	TArray<FCCLSurfaceSample> Samples;
	TestTrue(TEXT("original scene still usable"), Scene.QuerySurfaces(Query, Samples, Error));
	TestEqual(TEXT("original wall exists"), Samples.Num(), 1);
	Query.RequiredRevision = 2;
	TestFalse(TEXT("stale query rejected"), Scene.QuerySurfaces(Query, Samples, Error));
	Query.RequiredRevision = 1;
	Query.Direction.X = 2.;
	TestFalse(TEXT("nonunit direction rejected"), Scene.QuerySurfaces(Query, Samples, Error));
	TestEqual(TEXT("failed query preserves prior samples"), Samples.Num(), 1);
	TestEqual(TEXT("failed query preserves prior identity"), Samples[0].SurfaceId, Wall().SurfaceId);
	FCCLShelterSample Shelter;
	Shelter.Revision = 99;
	FCCLShelterQuery ShelterQuery;
	ShelterQuery.BodyId = TEXT("World");
	ShelterQuery.RequiredRevision = 2;
	TestFalse(TEXT("stale shelter revision rejected"), FCCLShelterEvaluator::Evaluate(Scene, ShelterQuery, Shelter, Error));
	TestEqual(TEXT("failed shelter query preserves output"), Shelter.Revision, uint64(99));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLSurfaceBudgetTest, "CCL.Environment.Surface.BudgetAndStableOrdering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLSurfaceBudgetTest::RunTest(const FString& Parameters)
{
	FCCLSurfaceScene Scene;
	FString Error;
	TestTrue(TEXT("coplanar fixture"), Scene.Replace({ Floor(3, 5.), Floor(1, 5.), Floor(2, 5.) }, {}, 1, Error));
	FCCLSurfaceQuery Query;
	Query.BodyId = TEXT("World");
	TArray<FCCLSurfaceSample> Samples;
	TestTrue(TEXT("complete result"), Scene.QuerySurfaces(Query, Samples, Error));
	if (TestEqual(TEXT("all contacts retained"), Samples.Num(), 3))
	{
		TestEqual(TEXT("stable first contact"), Samples[0].SurfaceId.D, uint32(1));
		TestEqual(TEXT("stable last contact"), Samples[2].SurfaceId.D, uint32(3));
	}

	Query.MaximumResults = 2;
	TestFalse(TEXT("overflow rejects entire query"), Scene.QuerySurfaces(Query, Samples, Error));
	TestEqual(TEXT("no truncated occlusion result"), Samples.Num(), 3);
	TestTrue(TEXT("empty scene is a valid newer scene"), Scene.Replace({}, {}, 2, Error));
	TestTrue(TEXT("empty scene query"), Scene.QuerySurfaces(Query, Samples, Error));
	TestEqual(TEXT("no obstacles"), Samples.Num(), 0);
	FCCLShelterQuery ShelterQuery;
	ShelterQuery.BodyId = TEXT("World");
	FCCLShelterSample Shelter;
	TestTrue(TEXT("outdoors query"), FCCLShelterEvaluator::Evaluate(Scene, ShelterQuery, Shelter, Error));
	TestEqual(TEXT("outdoors sunlight"), Shelter.Transmission.Sun, 1.);
	TestEqual(TEXT("outdoors rain"), Shelter.Transmission.Precipitation, 1.);
	TestEqual(TEXT("outdoors wind"), Shelter.Transmission.Wind, 1.);
	const FGuid OldEpoch = Scene.GetEpoch();
	FCCLSurfaceScene Rebuilt;
	TestTrue(TEXT("rebuild may restore the same saved revision"), Rebuilt.Replace({ Floor(1, 8.) }, {}, 2, Error));
	Scene = MoveTemp(Rebuilt);
	Query.RequiredRevision = 2;
	Query.RequiredEpoch = OldEpoch;
	TestFalse(TEXT("old execution rejected despite equal revision number"), Scene.QuerySurfaces(Query, Samples, Error));
	TestEqual(TEXT("failed old-epoch query leaves output unchanged"), Samples.Num(), 0);
	Query.RequiredEpoch = Scene.GetEpoch();
	TestTrue(TEXT("new execution accepted"), Scene.QuerySurfaces(Query, Samples, Error));
	TestEqual(TEXT("restored surface has a new execution identity"), Samples[0].Epoch, Scene.GetEpoch());
	ShelterQuery.RequiredEpoch = OldEpoch;
	TestFalse(TEXT("old shelter execution rejected"), FCCLShelterEvaluator::Evaluate(Scene, ShelterQuery, Shelter, Error));
	return true;
}
#endif
