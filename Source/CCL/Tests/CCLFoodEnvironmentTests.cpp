#include "Items/CCLFoodEnvironment.h"
#include "Items/CCLInventorySnapshot.h"
#include "Environment/CCLSurfaceSimulation.h"
#include "Environment/CCLSurfaceQuery.h"
#include "Misc/AutomationTest.h"
#include "Environment/CCLWorldSnapshot.h"
#include "Agents/CCLLifeSimulation.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
const FGuid FoodWorld(901, 902, 903, 904);
const FGuid WeatherId(901, 902, 903, 905);
bool SurfaceAt(FCCLSurfaceSimulation& Surface, double Time, FString& Error)
{
    FCCLWorldStep Step;
    Step.StepId = Surface.GetStepId() + 1;
    Step.GameFromSeconds = Surface.GetGameSeconds();
    Step.GameToSeconds = Step.GameFromSeconds + (Time - Surface.GetWorldSeconds()) / 60.;
    Step.WorldFromSeconds = Surface.GetWorldSeconds();
    Step.WorldToSeconds = Time;
    const TArray<FCCLSurfaceOpening> Openings;
    return Surface.Advance(Step, Error, nullptr, nullptr, nullptr, &Openings);
}
bool Setup(FCCLSurfaceSimulation& Surface, FCCLInventorySnapshot& Record, FString& Error)
{
    FCCLSurfaceGrid Grid;
    Grid.RegionId = WeatherId;
    Grid.Cells.SetNum(1);
    Grid.Forcing.TemperatureCelsius = -5.;
    Grid.Forcing.FlowConductance = 0.;
    Grid.Forcing.PhaseMetersPerDegreeWorldSecond = 0.;
    if (!Surface.Initialize(FoodWorld, 0., 0., 0, Error) || !Surface.AddRegion(Grid, Error))
    {
        return false;
    }
    Record.ContainerId = FGuid(911, 912, 913, 914);
    Record.Environment.Source = ECCLFoodEnvironment::WeatherRegion;
    Record.Environment.SourceId = WeatherId;
    if (!CCLFoodEnvironment::Sample(Record.Environment, Surface, Error))
    {
        return false;
    }
    FCCLFoodState Food;
    auto& Item = Record.Items.AddDefaulted_GetRef();
    Item.Id = FGuid(921, 922, 923, 924);
    Item.Definition = TEXT("/Game/Progression/DA_Bread.DA_Bread");
    Item.Quantity = 2;
    Item.Slot = 0;
    return CCLFood::Create(FoodWorld, FGuid(931, 932, 933, 934), 2, 0., {}, Record.Environment.Conditions, Food,
                           Error) &&
           CCLFood::Encode(Food, Item.Food, Error);
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLFoodEnvironmentHistoryTest, "CCL.Environment.Food.EnvironmentHeldHistory",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLFoodEnvironmentHistoryTest::RunTest(const FString& Parameters)
{
    FString Error;
    FCCLSurfaceSimulation Surface;
    FCCLInventorySnapshot Record;
    if (!TestTrue(TEXT("setup frozen environment"), Setup(Surface, Record, Error)))
    {
        return false;
    }
    auto Whole = Record;
    auto WholeSurface = Surface;
    auto Warm = Surface.FindRegion(WeatherId)->Forcing;
    Warm.TemperatureCelsius = 20.;
    TestTrue(TEXT("warming does not rewrite held sample"), Surface.ChangeForcing(WeatherId, Warm, Error));
    TestTrue(TEXT("partial world interval"),
             SurfaceAt(Surface, .5, Error) && CCLFoodEnvironment::Advance(Record, Surface, 0., Error));
    TestEqual(TEXT("sample held before boundary"), Record.Environment.Conditions.TemperatureCelsius, -5.);
    TArray<uint8> Bytes;
    TArray<FCCLInventorySnapshot> Restored;
    TestTrue(TEXT("mid-boundary persistence"), CCLInventorySnapshot::Encode({Record}, FoodWorld, .5, Bytes, Error) &&
                                                   CCLInventorySnapshot::Decode(Bytes, FoodWorld, .5, Restored, Error));
    if (!TestEqual(TEXT("restored container"), Restored.Num(), 1))
    {
        return false;
    }
    Record = Restored[0];
    TestTrue(TEXT("finish boundary"),
             SurfaceAt(Surface, 1., Error) && CCLFoodEnvironment::Advance(Record, Surface, .5, Error));
    FCCLFoodState Food;
    TestTrue(TEXT("decode food"), CCLFood::Decode(Record.Items[0].Food, Food, Error));
    TestEqual(TEXT("past frozen interval stayed frozen"), Food.Lots[0].SpoilageSeconds, 0.);
    TestEqual(TEXT("new environment for next interval"), Food.Storage.TemperatureCelsius, 20.);
    TestTrue(TEXT("warm exposure"), SurfaceAt(Surface, 2., Error) &&
                                        CCLFoodEnvironment::Advance(Record, Surface, 1., Error) &&
                                        CCLFood::Decode(Record.Items[0].Food, Food, Error));
    TestTrue(TEXT("one warm second at sampled humidity"),
             FMath::IsNearlyEqual(Food.Lots[0].SpoilageSeconds, .25 + .75 * (.004 / (.005 * FMath::Exp(1.2))), 1.e-8));
    TestTrue(TEXT("whole interval reference"),
             WholeSurface.ChangeForcing(WeatherId, Warm, Error) && SurfaceAt(WholeSurface, 1., Error) &&
                 CCLFoodEnvironment::Advance(Whole, WholeSurface, 0., Error) && SurfaceAt(WholeSurface, 2., Error) &&
                 CCLFoodEnvironment::Advance(Whole, WholeSurface, 1., Error));
    TestEqual(TEXT("split and saved exposure matches whole reference"), Record.Items[0].Food, Whole.Items[0].Food);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLFoodEnvironmentFailureTest, "CCL.Environment.Food.EnvironmentSourceAndFailure",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLFoodEnvironmentFailureTest::RunTest(const FString& Parameters)
{
    FString Error;
    FCCLSurfaceSimulation Surface;
    FCCLInventorySnapshot Record;
    TestTrue(TEXT("setup"), Setup(Surface, Record, Error));
    FCCLAirSpace Air;
    Air.SpaceId = FGuid(1, 2, 3, 4);
    Air.ClimateRegionId = WeatherId;
    TestTrue(TEXT("indoor source"), Air.Initialize(30., .008, Error) && Surface.AddAirSpace(Air, Error));
    FCCLFoodEnvironmentBinding Indoor;
    Indoor.Source = ECCLFoodEnvironment::AirSpace;
    Indoor.SourceId = Air.SpaceId;
    Indoor.Conditions.SpoilageMultiplier = .2;
    TestTrue(TEXT("indoor sample"), CCLFoodEnvironment::Sample(Indoor, Surface, Error));
    TestTrue(TEXT("actual room temperature"), FMath::IsNearlyEqual(Indoor.Conditions.TemperatureCelsius, 30., 1.e-8));
    TestTrue(TEXT("humidity uses room vapor density"),
             FMath::IsNearlyEqual(Indoor.Conditions.RelativeHumidity, .008 / (.005 * FMath::Exp(1.8)), 1.e-8));
    TestEqual(TEXT("container preservation remains separate"), Indoor.Conditions.SpoilageMultiplier, .2);
    Record.Environment.SourceId = FGuid(8, 8, 8, 8);
    const auto Before = Record;
    TestTrue(TEXT("surface interval"), SurfaceAt(Surface, 1., Error));
    TestFalse(TEXT("missing source fails"), CCLFoodEnvironment::Advance(Record, Surface, 0., Error));
    TestEqual(TEXT("failed candidate preserves food"), Record.Items[0].Food, Before.Items[0].Food);
    TestEqual(TEXT("failed candidate preserves sample"), Record.Environment.SampleWorldSeconds,
              Before.Environment.SampleWorldSeconds);
    Record.Environment.SourceId = WeatherId;
    TestTrue(TEXT("advance surface beyond an unobserved interval"), SurfaceAt(Surface, 2., Error));
    TestFalse(TEXT("large unsampled jump fails"), CCLFoodEnvironment::Advance(Record, Surface, 0., Error));
    Record.Environment.SampleWorldSeconds = 2.;
    TestFalse(TEXT("future sample rejected by persistence"),
              CCLInventorySnapshot::Validate({Record}, FoodWorld, 1., Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLFoodEnvironmentLegacyTest, "CCL.Environment.Food.EnvironmentLegacyContainer",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLFoodEnvironmentLegacyTest::RunTest(const FString& Parameters)
{
    FString Error;
    FCCLSurfaceSimulation Surface;
    FCCLInventorySnapshot Record;
    TestTrue(TEXT("setup"), Setup(Surface, Record, Error));
    Record.Environment = {};
    TArray<uint8> Bytes;
    TestTrue(TEXT("encode current body"), CCLInventorySnapshot::Encode({Record}, FoodWorld, 0., Bytes, Error));
    // Version 1 had neither the 57-byte environment record nor the 81-byte owner record.
    Bytes.SetNum(Bytes.Num() - 138);
    Bytes[4] = 1;
    TArray<FCCLInventorySnapshot> Read;
    TestTrue(TEXT("read actual legacy layout"), CCLInventorySnapshot::Decode(Bytes, FoodWorld, 0., Read, Error));
    if (!TestEqual(TEXT("legacy record count"), Read.Num(), 1))
    {
        return false;
    }
    TestFalse(TEXT("legacy conditions remain fixed"), Read[0].Environment.IsDynamic());
    TestEqual(TEXT("food history is preserved"), Read[0].Items[0].Food, Record.Items[0].Food);
    Record.Items.Reset();
    Record.Environment.Source = ECCLFoodEnvironment::WeatherRegion;
    Record.Environment.SourceId = WeatherId;
    TestTrue(TEXT("empty dynamic container survives"),
             CCLInventorySnapshot::Encode({Record}, FoodWorld, 0., Bytes, Error) &&
                 CCLInventorySnapshot::Decode(Bytes, FoodWorld, 0., Read, Error));
    TestTrue(TEXT("source survives empty state"), Read[0].Environment.SourceId == WeatherId && Read[0].Items.IsEmpty());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLFoodEnvironmentWorldContextTest, "CCL.Environment.Food.EnvironmentWorldContext",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLFoodEnvironmentWorldContextTest::RunTest(const FString& Parameters)
{
    FString Error;
    FCCLSurfaceSimulation Surface;
    FCCLInventorySnapshot Record;
    FCCLLifeSimulation Life;
    FCCLWorldClock Clock;
    FCCLWorldIdentity Identity;
    Identity.WorldId = FoodWorld;
    if (!TestTrue(TEXT("world setup"), Setup(Surface, Record, Error) &&
                                           Life.Initialize(FCCLLifeSimulation::MerchantScenario(42), Error) &&
                                           Clock.Reset(0., 60., Error)))
    {
        return false;
    }
    FCCLWorldSnapshot Snapshot;
    if (!TestTrue(TEXT("capture common world"),
                  FCCLWorldSnapshotCodec::Capture(Identity, Clock, Life, Snapshot, Error, nullptr, &Surface)))
    {
        return false;
    }
    Snapshot.Inventories = {Record};
    TArray<uint8> Bytes;
    TestTrue(TEXT("valid bound world encodes"), FCCLWorldSnapshotCodec::Encode(Snapshot, Bytes, Error));
    const auto Before = Bytes;
    Snapshot.Inventories[0].Environment.SourceId = FGuid(8, 8, 8, 8);
    TestFalse(TEXT("missing food source rejects entire world"), FCCLWorldSnapshotCodec::Encode(Snapshot, Bytes, Error));
    TestTrue(TEXT("failed world encode preserves output"), Bytes == Before);
    Snapshot.Inventories[0] = Record;
    Snapshot.Inventories[0].Environment.Conditions.TemperatureCelsius = 20.;
    TestFalse(TEXT("cached condition and item mismatch rejects entire world"),
              FCCLWorldSnapshotCodec::Encode(Snapshot, Bytes, Error));
    TestTrue(TEXT("inconsistent history preserves output"), Bytes == Before);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLFoodPositionTest, "CCL.Environment.Food.PositionBinding",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLFoodPositionTest::RunTest(const FString& Parameters)
{
    FString Error;
    FCCLSurfaceSimulation Surface;
    FCCLInventorySnapshot Record;
    if (!TestTrue(TEXT("setup"), Setup(Surface, Record, Error)))
    {
        return false;
    }
    FCCLFoodEnvironmentBinding Binding;
    TestTrue(TEXT("outdoor selection"), CCLFoodEnvironment::ResolveAt(FVector(.1, .1, 100.), Surface, Binding, Error));
    TestTrue(TEXT("outdoor ignores altitude"),
             Binding.Source == ECCLFoodEnvironment::WeatherRegion && Binding.SourceId == WeatherId);
    TestTrue(TEXT("exclusive grid edge"), CCLFoodEnvironment::ResolveAt(FVector(.5, .1, 0.), Surface, Binding, Error));
    TestTrue(TEXT("outside default"), !Binding.IsDynamic() && Binding.Conditions.TemperatureCelsius == 20. &&
                                          Binding.Conditions.RelativeHumidity == .5);
    auto Overlap = *Surface.FindRegion(WeatherId);
    Overlap.RegionId = FGuid(1, 2, 3, 4);
    TestTrue(TEXT("overlap region"), Surface.AddRegion(Overlap, Error));
    TestTrue(TEXT("deterministic equal-area choice"),
             CCLFoodEnvironment::ResolveAt(FVector(.1), Surface, Binding, Error) &&
                 Binding.SourceId == Overlap.RegionId);
    FCCLAirSpace Room;
    Room.SpaceId = FGuid(41, 42, 43, 44);
    Room.ClimateRegionId = WeatherId;
    Room.MaximumMeters = FVector(.5, .5, 2.);
    Room.VolumeCubicMeters = .5;
    TestTrue(TEXT("room"), Room.Initialize(30., .008, Error) && Surface.AddAirSpace(Room, Error));
    TestTrue(TEXT("indoor wins"), CCLFoodEnvironment::ResolveAt(FVector(.1), Surface, Binding, Error) &&
                                      Binding.SourceId == Room.SpaceId &&
                                      Binding.Source == ECCLFoodEnvironment::AirSpace);
    TestTrue(TEXT("above room returns weather"),
             CCLFoodEnvironment::ResolveAt(FVector(.1, .1, 2.), Surface, Binding, Error) &&
                 Binding.SourceId == Overlap.RegionId);
    Record.Owner = ECCLInventoryOwner::FoodPickup;
    Record.Capacity = 1;
    Record.Transform.SetLocation(FVector(10.));
    TestTrue(TEXT("old cold history"),
             SurfaceAt(Surface, 1., Error) && CCLFoodEnvironment::Advance(Record, Surface, 0., Error));
    FCCLFoodState Before, After;
    TestTrue(TEXT("decode old"), CCLFood::Decode(Record.Items[0].Food, Before, Error));
    TestTrue(TEXT("relocation settles old and binds room"),
             CCLFoodEnvironment::RelocatePickup(Record, Surface, Error) &&
                 Record.Environment.Source == ECCLFoodEnvironment::AirSpace &&
                 CCLFood::Decode(Record.Items[0].Food, After, Error));
    TestEqual(TEXT("past exposure unchanged"), After.Lots[0].SpoilageSeconds, Before.Lots[0].SpoilageSeconds);
    TestTrue(TEXT("warm next interval"), SurfaceAt(Surface, 2., Error) &&
                                             CCLFoodEnvironment::Advance(Record, Surface, 1., Error) &&
                                             CCLFood::Decode(Record.Items[0].Food, After, Error) &&
                                             After.Lots[0].SpoilageSeconds > Before.Lots[0].SpoilageSeconds);
    Record.Transform.SetLocation(FVector(10000.));
    TestTrue(TEXT("leave room clears dynamic binding"),
             CCLFoodEnvironment::RelocatePickup(Record, Surface, Error) && !Record.Environment.IsDynamic() &&
                 CCLFood::Decode(Record.Items[0].Food, After, Error) && After.Storage.TemperatureCelsius == 20.);
    After.Storage.TemperatureCelsius = -5.;
    TestTrue(TEXT("legacy fixed pickup reset to outdoor fallback"),
        CCLFood::Encode(After, Record.Items[0].Food, Error) &&
        CCLFoodEnvironment::RelocatePickup(Record, Surface, Error) &&
        CCLFood::Decode(Record.Items[0].Food, After, Error) && After.Storage.TemperatureCelsius == 20.);
    return true;
}

#endif
