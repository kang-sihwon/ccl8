#include "CCLEcologyLegacyTest.h"
#include "CCLEcologySpace.h"
#include "CCLSurfaceSimulation.h"
#include "CCLWorldAdvance.h"
#include "CCLWorldClock.h"
#include "Agents/CCLLifeSimulation.h"
#include "Misc/AutomationTest.h"
#include "Misc/Crc.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	FGuid Id(uint32 N)
	{
		return FGuid(940, 941, 942, N);
	}

	TArray<uint8> Capture(const FCCLEcologySimulation& E)
	{
		TArray<uint8> Bytes;
		FString Error;
		E.Capture(Bytes, Error);
		return Bytes;
	}

	bool Setup(FCCLEcologySimulation& E, FString& Error, bool bSpatial = true)
	{
		FCCLEcologySpecies S;
		S.Id = Id(2);
		FCCLEcologyHabitat H;
		H.Id = Id(3);
		H.MaximumMeters = FVector(10., 10., 4.);
		FCCLEcologyPopulation P;
		P.Id = Id(4);
		P.SpeciesId = S.Id;
		P.HabitatId = H.Id;
		P.Cohorts.Add(Id(5), {Id(5), 0., 2});
		FCCLEcologyForage F;
		F.Id = Id(8);
		F.HabitatId = H.Id;
		F.PositionMeters = FVector(7., 5., 2.);
		FCCLEcologyBehavior B;
		B.Id = Id(7);
		B.Energy = .1;
		B.MetabolismPerSecond = 0.;
		B.bUseWorldSpace = bSpatial;
		B.EnergyPerUnit = 1.;
		return E.Initialize(Id(1), 0., Error) && E.AddSpecies(S, Error) && E.AddHabitat(H, Error) && E.AddPopulation(P, Error)
			&& E.Materialize(Id(6), P.Id, Id(5), B.Id, true, Error) && E.AddForage(F, Error) && E.AddBehavior(B, Error);
	}
	struct FDetourSpace : ICCLEcologySpace
	{
		uint8 bUnavailable = 0;
		uint8 bBlocked = 0;
		uint8 bRequireWaterTime = 0;
		virtual bool FindRoute(const FCCLEcologyRouteRequest& Q, const FCCLSurfaceSimulation* S, TArray<FVector>& Out, FString& Error) const override
		{
			if (bUnavailable || (bRequireWaterTime && (!S || S->GetWorldSeconds() != Q.WorldSeconds)))
			{
				Error = TEXT("Not ready");
				return false;
			}

			Out.Reset();
			if (bBlocked)
			{
				return true;
			}

			Out.Add(Q.FromMeters);
			if (Q.FromMeters.X < 7. - 1.e-6)
			{
				if (Q.FromMeters.Y < 7. - 1.e-6)
				{
					Out.Add(FVector(5., 7., 2.));
				}

				Out.Add(FVector(7., 7., 2.));
			}
			Out.Add(Q.TargetMeters);
			return true;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLSpaceTravelTest, "CCL.Environment.Ecology.Space.DetourTimeAndNoRemoteFeeding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLSpaceTravelTest::RunTest(const FString& Parameters)
{
	FCCLEcologySimulation E;
	FString Error;
	FDetourSpace Space;
	if (!TestTrue(TEXT("spatial setup"), Setup(E, Error)))
	{
		return false;
	}

	TestTrue(TEXT("first route metre"), E.AdvanceTo(1., Error, 256, &Space));
	TestTrue(TEXT("first leg avoids straight shortcut"), E.FindIndividual(Id(7))->PositionMeters.Equals(FVector(5., 6., 2.)));
	TestTrue(TEXT("detour arrival"), E.AdvanceTo(6., Error, 256, &Space));
	TestTrue(TEXT("arrived after six route metres"), E.FindIndividual(Id(7))->PositionMeters.Equals(FVector(7., 5., 2.)));
	TestEqual(TEXT("no eating during travel"), E.GetBehaviors()[Id(7)].ConsumedUnits, 0.);
	TestTrue(TEXT("feed after arrival"), E.AdvanceTo(7., Error, 256, &Space));
	TestTrue(TEXT("actual food consumed"), E.GetBehaviors()[Id(7)].ConsumedUnits > .8);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLSpaceFailureTest, "CCL.Environment.Ecology.Space.QueryFailureAndBlockedPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLSpaceFailureTest::RunTest(const FString& Parameters)
{
	FCCLEcologySimulation E;
	FString Error;
	FDetourSpace Space;
	if (!TestTrue(TEXT("fixture"), Setup(E, Error)))
	{
		return false;
	}

	const auto Before = Capture(E);
	TestFalse(TEXT("missing spatial provider cannot use authored shortcut"), E.AdvanceTo(1., Error));
	Space.bUnavailable = 1;
	TestFalse(TEXT("unready path query rejects candidate"), E.AdvanceTo(1., Error, 256, &Space));
	TestTrue(TEXT("query failure preserves energy food position and time"), Capture(E) == Before);
	Space.bUnavailable = 0;
	Space.bBlocked = 1;
	TestTrue(TEXT("blocked route is valid no-move state"), E.AdvanceTo(1., Error, 256, &Space));
	TestTrue(TEXT("no teleport on blocked route"), E.FindIndividual(Id(7))->PositionMeters.Equals(FVector(5.,5.,2.)));
	TestTrue(TEXT("blocked activity visible"), E.GetBehaviors()[Id(7)].Activity == ECCLEcologyActivity::Blocked);
	TestEqual(TEXT("blocked route cannot eat"), E.GetBehaviors()[Id(7)].ConsumedUnits, 0.);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLSpaceTimeTest, "CCL.Environment.Ecology.Space.CoupledSurfaceBoundaryAndVersion4",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLSpaceTimeTest::RunTest(const FString& Parameters)
{
	FCCLEcologySimulation E, Old, Restored;
	FCCLSurfaceSimulation Surface;
	FCCLWorldClock Clock;
	FCCLLifeSimulation Life;
	FString Error;
	FDetourSpace Space;
	Space.bRequireWaterTime = 1;
	if (!TestTrue(TEXT("coupled setup"), Setup(E, Error) && Surface.Initialize(Id(1), 0., 0., 0, Error)
		&& Life.Initialize(FCCLLifeSimulation::MerchantScenario(42), Error) && Clock.QueueGameTime(1., false, Error)))
	{
		return false;
	}

	TestTrue(TEXT("spatial world step stops at next behavior boundary"), CCLWorldAdvance::Advance(Clock, Life, 1., 15., Error,
		256, &Surface, nullptr, nullptr, nullptr, nullptr, &E, 256, &Space));
	TestEqual(TEXT("no future water used for earlier ecology step"), Clock.GetWorldSeconds(), 1.);
	TestEqual(TEXT("surface time same"), Surface.GetWorldSeconds(), 1.);
	TestTrue(TEXT("legacy policy fixture"), Setup(Old, Error, false));
	auto Legacy = Capture(Old);
	CCLMakeEcologyV5Fixture(Legacy);
	Legacy.RemoveAt(Legacy.Num() - 13, 9); // Version 5 radius/mode in the single final behavior record.
	uint32 Version = 4;
	FMemory::Memcpy(Legacy.GetData() + 4, &Version, 4);
	const uint32 Crc = FCrc::MemCrc32(Legacy.GetData(), Legacy.Num() - 4);
	FMemory::Memcpy(Legacy.GetData() + Legacy.Num() - 4, &Crc, 4);
	TestTrue(TEXT("version 4 restores authored spatial policy"), Restored.Restore(Legacy, Error));
	TestFalse(TEXT("legacy never opts into world geometry silently"), Restored.HasSpatialBehavior());
	TestTrue(TEXT("legacy preserves histories"), Capture(Old) == Capture(Restored));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLLiquidBodyTest, "CCL.Environment.Ecology.Space.LiquidBodyDepthDryAndIce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLLiquidBodyTest::RunTest(const FString& Parameters)
{
	FString Error;
	auto MakeWater = [&](bool bDry, bool bIce)
	{
		FCCLSurfaceSimulation S;
		S.Initialize(Id(1),0.,0.,0,Error);
		FCCLSurfaceGrid G;
		G.RegionId=Id(9);
		G.Size=FIntPoint(4,4);
		G.SpacingMeters=1.;
		G.Cells.SetNum(16);
		for (auto& C : G.Cells)
		{
			C.WaterCubicMeters=bIce ? .1 : 2.;
			C.IceCubicMeters=bIce ? 1.9 : 0.;
		}

		if (bDry)
		{
			G.Cells[5].WaterCubicMeters=0.;
		}

		G.Ledger.InitialCubicMeters=G.TotalCubicMeters();
		TestTrue(TEXT("water fixture"), S.AddRegion(G,Error));
		return S;
	};
	const auto Water=MakeWater(false,false), Dry=MakeWater(true,false), Ice=MakeWater(false,true);
	TestTrue(TEXT("body fits actual liquid"), FCCLEcologyWorldSpace::IsInLiquid(Water,FVector(1.,1.,1.),.2));
	TestFalse(TEXT("body extends above surface"), FCCLEcologyWorldSpace::IsInLiquid(Water,FVector(1.,1.,1.9),.2));
	TestFalse(TEXT("body crosses dry cell"), FCCLEcologyWorldSpace::IsInLiquid(Dry,FVector(1.,1.,1.),.2));
	TestFalse(TEXT("ice volume is not liquid habitat"), FCCLEcologyWorldSpace::IsInLiquid(Ice,FVector(1.,1.,1.),.2));
	return true;
}
#endif
