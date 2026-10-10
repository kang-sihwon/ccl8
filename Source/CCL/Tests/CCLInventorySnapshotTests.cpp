#include "Items/CCLInventorySnapshot.h"
#include "Items/CCLFoodState.h"
#include "Environment/CCLWorldSnapshot.h"
#include "Agents/CCLLifeSimulation.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
const FGuid InventoryWorld(811, 812, 813, 814);
FCCLInventorySnapshot MakeInventoryRecord()
{
    FCCLInventorySnapshot Record;
    Record.ContainerId = FGuid(821, 822, 823, 824);
    auto& Item = Record.Items.AddDefaulted_GetRef();
    Item.Id = FGuid(831, 832, 833, 834);
    Item.Definition = TEXT("/Game/Progression/DA_Bread.DA_Bread");
    Item.Quantity = 3;
    Item.Slot = 2;
    FCCLFoodState Food;
    FString Error;
    CCLFood::Create(InventoryWorld, FGuid(841, 842, 843, 844), 3, 10., {}, {}, Food, Error);
    CCLFood::Advance(Food, 30., Error);
    CCLFood::Encode(Food, Item.Food, Error);
    return Record;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLInventorySnapshotRoundTripTest, "CCL.Environment.Food.InventoryWorldEnvelope",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLInventorySnapshotRoundTripTest::RunTest(const FString& Parameters)
{
    FString Error;
    auto Record = MakeInventoryRecord();
    FCCLInventorySnapshot Empty;
    Empty.ContainerId = FGuid(1, 2, 3, 4);
    TArray<FCCLInventorySnapshot> Output;
    TArray<uint8> Bytes;
    TestTrue(TEXT("encode loaded and empty container"),
             CCLInventorySnapshot::Encode({Record, Empty}, InventoryWorld, 30., Bytes, Error));
    TestTrue(TEXT("decode"), CCLInventorySnapshot::Decode(Bytes, InventoryWorld, 30., Output, Error));
    if (!TestEqual(TEXT("two containers"), Output.Num(), 2))
    {
        return false;
    }
    TestEqual(TEXT("food history"), Output[0].Items[0].Food, Record.Items[0].Food);
    TestEqual(TEXT("item identity"), Output[0].Items[0].Id, Record.Items[0].Id);
    TestEqual(TEXT("placement"), Output[0].Items[0].Slot, 2);
    TestTrue(TEXT("empty is explicit"), Output[1].Items.IsEmpty());

    FCCLLifeSimulation Life;
    FCCLWorldClock Clock;
    FCCLWorldIdentity Identity;
    Identity.WorldId = InventoryWorld;
    TestTrue(TEXT("life initialize"), Life.Initialize(FCCLLifeSimulation::MerchantScenario(42), Error));
    TestTrue(TEXT("life advance"), Life.TryAdvanceTo(30., Error));
    TestTrue(TEXT("clock matches"), Clock.Reset(30., 60., Error));
    FCCLWorldSnapshot Snapshot, Decoded;
    TestTrue(TEXT("capture world"), FCCLWorldSnapshotCodec::Capture(Identity, Clock, Life, Snapshot, Error));
    Snapshot.Inventories = {Record, Empty};
    TestTrue(TEXT("world encode"), FCCLWorldSnapshotCodec::Encode(Snapshot, Bytes, Error));
    TestTrue(TEXT("world decode"), FCCLWorldSnapshotCodec::Decode(Bytes, Decoded, Error));
    if (!TestEqual(TEXT("world containers"), Decoded.Inventories.Num(), 2))
    {
        return false;
    }
    TestEqual(TEXT("world and food travel together"), Decoded.Inventories[0].Items[0].Food, Record.Items[0].Food);
    Snapshot.Inventories[0].Items[0].Quantity = 2;
    const auto Before = Bytes;
    TestFalse(TEXT("quantity mismatch rejected"), FCCLWorldSnapshotCodec::Encode(Snapshot, Bytes, Error));
    TestTrue(TEXT("failed world encode preserves output"), Before == Bytes);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLInventorySnapshotRejectTest, "CCL.Environment.Food.InventoryWorldReject",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLInventorySnapshotRejectTest::RunTest(const FString& Parameters)
{
    FString Error;
    const auto Record = MakeInventoryRecord();
    TestFalse(TEXT("container identity collision"),
              CCLInventorySnapshot::Validate({Record, Record}, InventoryWorld, 30., Error));
    auto Other = Record;
    Other.ContainerId = FGuid(1, 2, 3, 4);
    TestFalse(TEXT("item in two owners"), CCLInventorySnapshot::Validate({Record, Other}, InventoryWorld, 30., Error));
    TestFalse(TEXT("future food"), CCLInventorySnapshot::Validate({Record}, InventoryWorld, 29., Error));
    TestFalse(TEXT("different world"), CCLInventorySnapshot::Validate({Record}, FGuid(1, 1, 1, 1), 30., Error));
    Other = Record;
    Other.Items[0].Definition = TEXT("/Script/Engine.Actor");
    TestFalse(TEXT("non-content reference"), CCLInventorySnapshot::Validate({Other}, InventoryWorld, 30., Error));
    Other = Record;
    Other.Capacity = 2;
    TestFalse(TEXT("slot outside capacity"), CCLInventorySnapshot::Validate({Other}, InventoryWorld, 30., Error));
    TArray<uint8> Bytes;
    TestTrue(TEXT("encode"), CCLInventorySnapshot::Encode({Record}, InventoryWorld, 30., Bytes, Error));
    TArray<FCCLInventorySnapshot> Output = {Other};
    auto Bad = Bytes;
    const int32 Huge = MAX_int32;
    FMemory::Memcpy(Bad.GetData() + 6, &Huge, 4);
    TestFalse(TEXT("oversized count before allocation"),
              CCLInventorySnapshot::Decode(Bad, InventoryWorld, 30., Output, Error));
    Bad = Bytes;
    FMemory::Memcpy(Bad.GetData() + 62, &Huge, 4);
    TestFalse(TEXT("oversized definition before allocation"),
              CCLInventorySnapshot::Decode(Bad, InventoryWorld, 30., Output, Error));
    Bad = Bytes;
    Bad[4] = 4;
    TestFalse(TEXT("future payload version"), CCLInventorySnapshot::Decode(Bad, InventoryWorld, 30., Output, Error));
    Bad = Bytes;
    Bad.Add(0);
    TestFalse(TEXT("trailing data"), CCLInventorySnapshot::Decode(Bad, InventoryWorld, 30., Output, Error));
    Bad = Bytes;
    Bad.Pop();
    TestFalse(TEXT("truncated data"), CCLInventorySnapshot::Decode(Bad, InventoryWorld, 30., Output, Error));
    TestEqual(TEXT("decode failures preserve output"), Output[0].Capacity, 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLPickupSnapshotTest, "CCL.Environment.Food.PickupOwnerSnapshot",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLPickupSnapshotTest::RunTest(const FString& Parameters)
{
    FString Error;
    auto Record = MakeInventoryRecord();
    Record.Owner = ECCLInventoryOwner::FoodPickup;
    Record.Capacity = 1;
    Record.Items[0].Slot = 0;
    Record.Transform = FTransform(FRotator(10, 20, 30), FVector(100, -200, 300), FVector(2, 3, 4));
    auto Empty = Record;
    Empty.ContainerId = FGuid(4, 3, 2, 1);
    Empty.Items.Reset();
    TArray<uint8> Bytes;
    TArray<FCCLInventorySnapshot> Output;
    TestTrue(TEXT("pickup and collected record"),
             CCLInventorySnapshot::Encode({Record, Empty}, InventoryWorld, 30., Bytes, Error));
    TestTrue(TEXT("decode pickup"), CCLInventorySnapshot::Decode(Bytes, InventoryWorld, 30., Output, Error));
    if (!TestEqual(TEXT("both owners"), Output.Num(), 2))
    {
        return false;
    }
    TestTrue(TEXT("transform"), Output[0].Transform.Equals(Record.Transform, 1.e-10));
    TestTrue(TEXT("owner kind"), Output[0].Owner == ECCLInventoryOwner::FoodPickup);
    TestTrue(TEXT("collected tombstone"), Output[1].Items.IsEmpty());
    TestEqual(TEXT("food"), Output[0].Items[0].Food, Record.Items[0].Food);
    auto Bad = Record;
    Bad.Owner = ECCLInventoryOwner(99);
    TestFalse(TEXT("future kind"), CCLInventorySnapshot::Validate({Bad}, InventoryWorld, 30., Error));
    Bad = Record;
    Bad.Transform.SetScale3D(FVector::ZeroVector);
    TestFalse(TEXT("zero scale"), CCLInventorySnapshot::Validate({Bad}, InventoryWorld, 30., Error));
    Bad = Record;
    Bad.Items[0].Food.Empty();
    TestFalse(TEXT("pickup needs food"), CCLInventorySnapshot::Validate({Bad}, InventoryWorld, 30., Error));
    Bad = Record;
    Bad.Capacity = 2;
    TestFalse(TEXT("pickup single slot"), CCLInventorySnapshot::Validate({Bad}, InventoryWorld, 30., Error));
    Bad = Record;
    Bad.Owner = ECCLInventoryOwner::Container;
    TestFalse(TEXT("ordinary owner cannot smuggle transform"),
              CCLInventorySnapshot::Validate({Bad}, InventoryWorld, 30., Error));
    auto Ordinary = MakeInventoryRecord();
    TestTrue(TEXT("current container"), CCLInventorySnapshot::Encode({Ordinary}, InventoryWorld, 30., Bytes, Error));
    Bytes.SetNum(Bytes.Num() - 81);
    Bytes[4] = 2;
    TestTrue(TEXT("version 2 migration"), CCLInventorySnapshot::Decode(Bytes, InventoryWorld, 30., Output, Error));
    TestTrue(TEXT("version 2 ordinary"),
             Output[0].Owner == ECCLInventoryOwner::Container && Output[0].Transform.Equals(FTransform::Identity));
    TestEqual(TEXT("legacy food unchanged"), Output[0].Items[0].Food, Ordinary.Items[0].Food);
    return true;
}
#endif
