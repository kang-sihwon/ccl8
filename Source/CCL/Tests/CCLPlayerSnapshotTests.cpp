#include "Session/CCLPlayerSnapshot.h"

#include "Environment/CCLWorldSnapshot.h"
#include "Agents/CCLLifeSimulation.h"
#include "Items/CCLItemDefinition.h"
#include "Misc/AutomationTest.h"
#include "Misc/Base64.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
const FGuid PlayerWorld(9071, 2, 3, 4);

FCCLPlayerSnapshot Profile()
{
    FCCLPlayerSnapshot P;
    P.IdentityKey = TEXT("local:0");
    P.AccountId = FGuid(9072, 2, 3, 4);
    P.Points = 17;
    P.Quest = 2;
    P.HealthFraction = 0.625f;
    P.StaminaFraction = 0.25f;
    P.Skills = {FName(TEXT("DA_PowerTraining")), FName(TEXT("DA_VitalityTraining"))};
    FCCLSavedItem Staff;
    Staff.Id = FGuid(9073, 2, 3, 4);
    Staff.Definition = TEXT("DA_TrainingStaff");
    Staff.Quantity = 1;
    P.Items.Add(Staff);
    for (const auto Hand : {FGameplayTag(CCLItemTags::Slot_LeftHand), FGameplayTag(CCLItemTags::Slot_RightHand)})
    {
        FCCLEquippedSlot Slot;
        Slot.Slot = Hand;
        Slot.Id = Staff.Id;
        P.EquipmentSlots.Add(Slot);
    }
    FCCLSavedItem Bread;
    Bread.Id = FGuid(9074, 2, 3, 4);
    Bread.Definition = TEXT("DA_FoodSample");
    Bread.Quantity = 3;
    Bread.Slot = 15;
    FCCLFoodState Food;
    FString Error;
    CCLFood::Create(PlayerWorld, FGuid(9075, 2, 3, 4), 3, 5., {}, {}, Food, Error);
    CCLFood::Advance(Food, 30., Error);
    CCLFood::Encode(Food, Bread.Food, Error);
    P.Items.Add(Bread);
    return P;
}

bool World(FCCLWorldSnapshot& Snapshot, FString& Error)
{
    FCCLLifeSimulation Life;
    FCCLWorldClock Clock;
    FCCLWorldIdentity Identity;
    Identity.WorldId = PlayerWorld;
    auto P = Profile();
    if (!Life.Initialize(FCCLLifeSimulation::MerchantScenario(42), Error) || !Life.OpenAccount(P.AccountId, 73) ||
        !Life.TryAdvanceTo(30., Error) || !Clock.Reset(30., 60., Error) ||
        !FCCLWorldSnapshotCodec::Capture(Identity, Clock, Life, Snapshot, Error))
    {
        return false;
    }
    Snapshot.Players = {P};
    return true;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLPlayerSnapshotRoundTripTest, "CCL.Environment.Save.PlayerProfileRoundTrip",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLPlayerSnapshotRoundTripTest::RunTest(const FString& Parameters)
{
    FString Error;
    const auto P = Profile();
    TArray<uint8> Bytes, Again;
    TArray<FCCLPlayerSnapshot> Decoded;
    TestNotNull(TEXT("experiment pickup has an allowed definition ID"), FCCLSessionCodec::Item(TEXT("DA_FoodSample")));
    if (!TestTrue(TEXT("encode complete profile"), CCLPlayerSnapshot::Encode({P}, PlayerWorld, 30., Bytes, Error)) ||
        !TestTrue(TEXT("decode complete profile"),
                  CCLPlayerSnapshot::Decode(Bytes, PlayerWorld, 30., Decoded, Error)) ||
        !TestEqual(TEXT("one profile"), Decoded.Num(), 1))
    {
        AddError(Error);
        return false;
    }
    TestTrue(TEXT("deterministic round trip"),
             CCLPlayerSnapshot::Encode(Decoded, PlayerWorld, 30., Again, Error) && Again == Bytes);
    TestEqual(TEXT("identity key"), Decoded[0].IdentityKey, P.IdentityKey);
    TestEqual(TEXT("food bytes"), Decoded[0].Items[1].Food, P.Items[1].Food);
    TestEqual(TEXT("injury"), Decoded[0].HealthFraction, 0.625f);
    TestEqual(TEXT("stamina"), Decoded[0].StaminaFraction, 0.25f);
    TestEqual(TEXT("completed quest independent from host checkpoint"), Decoded[0].Quest, uint8(2));
    TestEqual(TEXT("both occupied hands"), Decoded[0].EquipmentSlots.Num(), 2);
    auto Wide = P;
    Wide.Capacity = 32;
    Wide.Items[1].Slot = 31;
    TestTrue(TEXT("profile preserves actual inventory capacity"),
             CCLPlayerSnapshot::Encode({Wide}, PlayerWorld, 30., Again, Error));
    FCCLSessionRecord Reentrant;
    Reentrant.AccountId = P.AccountId;
    Reentrant.Coins = 0;
    Reentrant.AgentSimulation = TEXT("nested world is forbidden");
    TestFalse(TEXT("profile validation cannot recurse into a nested world"),
              FCCLSessionCodec::ValidateProfile(Reentrant, PlayerWorld, 30., 16));
    FCCLWorldSnapshot Saved, Output;
    if (!TestTrue(TEXT("world fixture"), World(Saved, Error)) ||
        !TestTrue(TEXT("world profile encode"), FCCLWorldSnapshotCodec::Encode(Saved, Bytes, Error)) ||
        !TestTrue(TEXT("world profile decode"), FCCLWorldSnapshotCodec::Decode(Bytes, Output, Error)))
    {
        AddError(Error);
        return false;
    }
    TestEqual(TEXT("world schema six"), Output.Schema, uint32(6));
    TestFalse(TEXT("new profile data is not migrated absence"), Output.bPlayersMigrated != 0);
    TestEqual(TEXT("profile travels with completed time"), Output.Clock.WorldSeconds, 30.);
    TestEqual(TEXT("world contains profile"), Output.Players.Num(), 1);
    FCCLLifeSimulation Life;
    TestTrue(TEXT("account balance has one owner"),
             Life.Load(Output.Life, Error) && Life.GetEconomy().Accounts[P.AccountId].Balance == 73);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLPlayerSnapshotRejectTest, "CCL.Environment.Save.PlayerProfileReject",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLPlayerSnapshotRejectTest::RunTest(const FString& Parameters)
{
    FString Error;
    const auto P = Profile();
    TArray<uint8> Bytes = {91, 92};
    auto Reject = [&](FCCLPlayerSnapshot Bad, const TCHAR* Message)
    {
        TestFalse(Message, CCLPlayerSnapshot::Encode({Bad}, PlayerWorld, 30., Bytes, Error));
        TestTrue(TEXT("failed encode preserves output"), Bytes == TArray<uint8>({91, 92}));
    };
    auto Bad = P;
    Bad.IdentityKey = TEXT("Remote:Unsafe");
    Reject(Bad, TEXT("noncanonical identity key"));
    Bad = P;
    Bad.IdentityKey = FString::ChrN(193, 'a');
    Reject(Bad, TEXT("oversized identity key"));
    Bad = P;
    Bad.AccountId.Invalidate();
    Reject(Bad, TEXT("missing account"));
    Bad = P;
    Bad.HealthFraction = std::numeric_limits<float>::quiet_NaN();
    Reject(Bad, TEXT("nonfinite health"));
    Bad = P;
    Bad.StaminaFraction = 1.1f;
    Reject(Bad, TEXT("stamina out of range"));
    Bad = P;
    Bad.Capacity = 15;
    Reject(Bad, TEXT("item outside capacity"));
    Bad = P;
    Bad.EquipmentSlots.Pop();
    Reject(Bad, TEXT("partial two-hand occupancy"));
    Bad = P;
    Bad.EquipmentSlots[0].Id = FGuid::NewGuid();
    Reject(Bad, TEXT("unknown equipment reference"));
    Bad = P;
    Bad.Items[1].Definition = TEXT("UnknownItem");
    Reject(Bad, TEXT("unknown item definition"));
    Bad = P;
    Bad.Items[1].Quantity = 2;
    Reject(Bad, TEXT("food quantity mismatch"));
    Bad = P;
    Bad.Items[1].LoadedAmmo = 1;
    Reject(Bad, TEXT("ammo on food"));
    Bad = P;
    Bad.Items.Add(P.Items[1]);
    Reject(Bad, TEXT("duplicate item GUID"));
    Bad = P;
    Bad.Skills.Add(P.Skills[0]);
    Reject(Bad, TEXT("duplicate skill"));
    Bad = P;
    Bad.Points = -1;
    Reject(Bad, TEXT("negative points"));
    Bad = P;
    Bad.Quest = 3;
    Reject(Bad, TEXT("unknown quest state"));
    TestFalse(TEXT("foreign food world"), CCLPlayerSnapshot::Validate({P}, FGuid::NewGuid(), 30., Error));
    TestFalse(TEXT("future food history"), CCLPlayerSnapshot::Validate({P}, PlayerWorld, 29., Error));
    auto Other = P;
    Other.IdentityKey = TEXT("local:1");
    Other.Items.Reset();
    Other.EquipmentSlots.Reset();
    TestFalse(TEXT("duplicate account across profiles"),
              CCLPlayerSnapshot::Validate({P, Other}, PlayerWorld, 30., Error));
    Other.AccountId = FGuid::NewGuid();
    Other.IdentityKey = P.IdentityKey;
    TestFalse(TEXT("duplicate identity across profiles"),
              CCLPlayerSnapshot::Validate({P, Other}, PlayerWorld, 30., Error));
    Other.IdentityKey = TEXT("local:1");
    Other.Items = P.Items;
    Other.EquipmentSlots = P.EquipmentSlots;
    TestFalse(TEXT("duplicate items across profiles"),
              CCLPlayerSnapshot::Validate({P, Other}, PlayerWorld, 30., Error));
    TArray<FCCLPlayerSnapshot> Limit;
    for (int32 Index = 0; Index < CCLPlayerSnapshot::MaximumPlayers; ++Index)
    {
        FCCLPlayerSnapshot Empty;
        Empty.IdentityKey = FString::Printf(TEXT("test:%d"), Index);
        Empty.AccountId = FGuid(9801, 2, 3, Index + 1);
        Empty.Capacity = 128;
        Limit.Add(Empty);
    }
    TestTrue(TEXT("exact profile count bound accepts valid profiles"),
             CCLPlayerSnapshot::Validate(Limit, PlayerWorld, 30., Error));
    auto Extra = Limit[0];
    Extra.IdentityKey = TEXT("test:64");
    Extra.AccountId = FGuid(9801, 2, 3, 65);
    Limit.Add(Extra);
    TestFalse(TEXT("one additional valid profile exceeds count bound"),
              CCLPlayerSnapshot::Validate(Limit, PlayerWorld, 30., Error));
    Limit.Pop();
    for (int32 Index = 0; Index < Limit.Num(); ++Index)
    {
        for (int32 Slot = 0; Slot < 64; ++Slot)
        {
            FCCLSavedItem Item;
            Item.Id = FGuid(9802, Index + 1, Slot + 1, 1);
            Item.Definition = TEXT("DA_RecoveryPotion");
            Item.Quantity = 1;
            Item.Slot = Slot;
            Limit[Index].Items.Add(Item);
        }
    }
    TestTrue(TEXT("exact total item bound accepts valid inventories"),
             CCLPlayerSnapshot::Validate(Limit, PlayerWorld, 30., Error));
    auto ExtraItem = Limit[0].Items[0];
    ExtraItem.Id = FGuid(9802, 100, 100, 1);
    ExtraItem.Slot = 64;
    Limit[0].Items.Add(ExtraItem);
    TestFalse(TEXT("one additional valid item exceeds total bound"),
              CCLPlayerSnapshot::Validate(Limit, PlayerWorld, 30., Error));
    if (!TestTrue(TEXT("valid binary fixture"), CCLPlayerSnapshot::Encode({P}, PlayerWorld, 30., Bytes, Error)))
    {
        return false;
    }
    TArray<FCCLPlayerSnapshot> Output = {P};
    auto CheckWire = [&](const TArray<uint8>& Corrupt, const TCHAR* Message)
    {
        TestFalse(Message, CCLPlayerSnapshot::Decode(Corrupt, PlayerWorld, 30., Output, Error));
        TestTrue(TEXT("failed decode preserves profile"),
                 Output.Num() == 1 && Output[0].AccountId == P.AccountId && Output[0].Items[1].Food == P.Items[1].Food);
    };
    auto Corrupt = Bytes;
    Corrupt[4] = 2;
    CheckWire(Corrupt, TEXT("future body version"));
    Corrupt = Bytes;
    Corrupt.Add(0);
    CheckWire(Corrupt, TEXT("trailing bytes"));
    Corrupt = Bytes;
    Corrupt.Pop();
    CheckWire(Corrupt, TEXT("truncated bytes"));
    Corrupt = Bytes;
    const int32 Huge = MAX_int32;
    FMemory::Memcpy(Corrupt.GetData() + 6, &Huge, 4);
    CheckWire(Corrupt, TEXT("oversized profile allocation"));
    Corrupt = Bytes;
    FMemory::Memcpy(Corrupt.GetData() + 10, &Huge, 4);
    CheckWire(Corrupt, TEXT("oversized key allocation"));
    Corrupt = Bytes;
    Corrupt[14] = 0xff;
    CheckWire(Corrupt, TEXT("invalid UTF8 key"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLPlayerSnapshotWorldOwnershipTest, "CCL.Environment.Save.PlayerWorldOwnership",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCCLPlayerSnapshotWorldOwnershipTest::RunTest(const FString& Parameters)
{
    FString Error;
    FCCLWorldSnapshot Saved;
    if (!TestTrue(TEXT("world fixture"), World(Saved, Error)))
    {
        return false;
    }
    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("valid world"), FCCLWorldSnapshotCodec::Encode(Saved, Bytes, Error)))
    {
        return false;
    }
    const auto Before = Bytes;
    auto Bad = Saved;
    Bad.Players[0].AccountId = FGuid::NewGuid();
    TestFalse(TEXT("account must exist in same economic ledger"), FCCLWorldSnapshotCodec::Encode(Bad, Bytes, Error));
    TestTrue(TEXT("invalid account preserves bytes"), Bytes == Before);
    Bad = Saved;
    FCCLInventorySnapshot Container;
    Container.ContainerId = FGuid::NewGuid();
    const auto& Bread = Saved.Players[0].Items[1];
    FCCLStoredItem Item;
    Item.Id = Bread.Id;
    Item.Definition = FCCLSessionCodec::Item(Bread.Definition)->GetPathName();
    Item.Quantity = Bread.Quantity;
    Item.Slot = 0;
    Item.Food = Bread.Food;
    Container.Items.Add(Item);
    Bad.Inventories.Add(Container);
    TestFalse(TEXT("same item cannot be in player and world pickup or container"),
              FCCLWorldSnapshotCodec::Encode(Bad, Bytes, Error));
    TestTrue(TEXT("duplicate item preserves bytes"), Bytes == Before);
    Bad = Saved;
    Bad.Players[0].Environment.Source = ECCLFoodEnvironment::AirSpace;
    Bad.Players[0].Environment.SourceId = FGuid::NewGuid();
    TestFalse(TEXT("profile storage must name an existing world environment"),
              FCCLWorldSnapshotCodec::Encode(Bad, Bytes, Error));
    FCCLSessionRecord Session;
    Session.AgentSimulation = FBase64::Encode(Before);
    Session.Items.Add(Bread);
    TestFalse(TEXT("personal outer session cannot duplicate profile item"), FCCLSessionCodec::Validate(Session));
    Session.Items.Reset();
    Session.AccountId = Saved.Players[0].AccountId;
    Session.Coins = 0;
    TestFalse(TEXT("personal outer session cannot duplicate profile account"), FCCLSessionCodec::Validate(Session));
    Session.AccountId.Invalidate();
    TestTrue(TEXT("outer session can contain other player profiles without recursion"),
             FCCLSessionCodec::Validate(Session));
    return true;
}
#endif
