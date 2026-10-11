#include "CCLInventoryNetworkSmoke.h"
#include "Items/CCLInventoryComponent.h"
#include "Items/CCLItemDefinition.h"
#include "Session/CCLSessionRecord.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
const FGuid RifleId(101, 102, 103, 104);
const FGuid FoodId(201, 202, 203, 204);
const FGuid WorldId(301, 302, 303, 304);
}

ACCLInventoryNetworkProbe::ACCLInventoryNetworkProbe()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    Inventory = CreateDefaultSubobject<UCCLInventoryComponent>(TEXT("Inventory"));
}

bool UCCLInventoryNetworkSmoke::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
    return false;
#else
    const auto* World = Cast<UWorld>(Outer);
    return World && World->IsGameWorld() && FParse::Param(FCommandLine::Get(), TEXT("CCLInventoryNetworkSmoke"));
#endif
}

TStatId UCCLInventoryNetworkSmoke::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLInventoryNetworkSmoke, STATGROUP_Tickables);
}

bool UCCLInventoryNetworkSmoke::Seed(ACCLInventoryNetworkProbe& Probe, bool bNewState)
{
    FKSIInventoryEntry Rifle;
    Rifle.Id = RifleId;
    Rifle.Definition = FCCLSessionCodec::Item(TEXT("DA_Rifle"));
    Rifle.Quantity = 1;
    Rifle.Slot = 0;
    Rifle.GetState<FCCLItemState>().LoadedAmmo = bNewState ? 0 : 1;
    FKSIInventoryEntry Food;
    Food.Id = FoodId;
    Food.Definition = FCCLSessionCodec::Item(TEXT("DA_Bread"));
    Food.Quantity = 2;
    Food.Slot = 1;
    FString Error;
    const auto* Policy = Food.Definition ? Food.Definition->FindFragment<FCCLItemFragment_Perishable>() : nullptr;
    if (!Policy || !CCLFood::Create(WorldId, FoodId, 2, 0., Policy->Policy, {}, Food.GetState<FCCLItemState>().Food, Error)
        || (bNewState && !CCLFood::Advance(Food.GetState<FCCLItemState>().Food, 10., Error)))
    {
        return false;
    }
    return Probe.Inventory->Restore({Rifle, Food});
}

void UCCLInventoryNetworkSmoke::Finish(bool bSuccess, const TCHAR* Reason)
{
    bFinished = 1;
    UE_LOG(LogTemp, Display, TEXT("CCL_INVENTORY_NET %s %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), Reason);
    FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1);
}

void UCCLInventoryNetworkSmoke::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (bFinished)
    {
        return;
    }
    Elapsed += DeltaTime;
    if (Directory.IsEmpty())
    {
        FParse::Value(FCommandLine::Get(), TEXT("CCLInventoryNetDir="), Directory);
        FParse::Value(FCommandLine::Get(), TEXT("CCLInventoryNetRole="), Role);
        UE_LOG(LogTemp, Display, TEXT("CCL_INVENTORY_NET READY"));
    }
    if (Directory.IsEmpty() || Elapsed > 120.f)
    {
        Finish(false, TEXT("configuration or replication timeout"));
        return;
    }
    auto Signal = [this](const FString& Name) { return FPaths::FileExists(Directory / Name); };
    if (GetWorld()->GetNetMode() != NM_Client)
    {
        if (!bUpdated && Signal(TEXT("first-initial")))
        {
            bUpdated = 1;
            for (TActorIterator<ACCLInventoryNetworkProbe> It(GetWorld()); It; ++It)
            {
                if (!Seed(**It, true))
                {
                    Finish(false, TEXT("updated fixture rejected"));
                    return;
                }
                It->ForceNetUpdate();
            }
        }
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            auto* PC = It->Get();
            if (!PC || PC->IsLocalController())
            {
                continue;
            }
            bool bExists = false;
            for (TActorIterator<ACCLInventoryNetworkProbe> Probe(GetWorld()); Probe; ++Probe)
            {
                bExists = bExists || Probe->GetOwner() == PC;
            }
            if (!bExists)
            {
                auto* Probe = GetWorld()->SpawnActor<ACCLInventoryNetworkProbe>();
                Probe->SetOwner(PC);
                if (!Seed(*Probe, bUpdated != 0))
                {
                    Finish(false, TEXT("initial fixture rejected"));
                    return;
                }
            }
        }
        if (Signal(TEXT("first-done")) && Signal(TEXT("late-done")))
        {
            Finish(true, TEXT("owner-only state update and late join"));
        }
        return;
    }
    auto* PC = GetWorld()->GetFirstPlayerController();
    bool bOtherVisible = false;
    for (TActorIterator<ACCLInventoryNetworkProbe> It(GetWorld()); It; ++It)
    {
        if (It->GetOwner() == PC)
        {
            if (!Observed.IsValid())
            {
                Observed = *It;
                It->Inventory->OnChanged.AddWeakLambda(this, [this]() { ++Notifications; });
            }
        }
        else
        {
            bOtherVisible = true;
            if (!It->Inventory->GetEntries().IsEmpty())
            {
                Finish(false, TEXT("another owner inventory leaked"));
                return;
            }
        }
    }
    auto* Probe = Observed.Get();
    if (!Probe)
    {
        return;
    }
    const auto* Rifle = Probe->Inventory->Find(RifleId);
    const auto* Food = Probe->Inventory->Find(FoodId);
    if (!Rifle || !Food)
    {
        return;
    }
    const auto* FoodState = Food->State.GetPtr<FCCLItemState>();
    const auto* RifleState = Rifle->State.GetPtr<FCCLItemState>();
    if (!FoodState || !RifleState)
    {
        Finish(false, TEXT("extension state type lost"));
        return;
    }
    FString Error;
    if (FoodState->Food.WorldId != WorldId || !CCLFood::Validate(FoodState->Food, 2, Error)
        || Probe->Inventory->Remove(RifleId, 1) || Probe->Inventory->Add(Rifle->Definition, 1).IsValid())
    {
        Finish(false, TEXT("state content or client authority"));
        return;
    }
    if (Role == TEXT("first") && RifleState->LoadedAmmo == 1 && FoodState->Food.UpdatedWorldSeconds == 0.)
    {
        FFileHelper::SaveStringToFile(TEXT("ready"), *(Directory / TEXT("first-initial")));
    }
    if (RifleState->LoadedAmmo == 0 && FoodState->Food.UpdatedWorldSeconds == 10.)
    {
        FFileHelper::SaveStringToFile(TEXT("updated"), *(Directory / (Role + TEXT("-updated"))));
        if (bOtherVisible && (Role == TEXT("late") || Notifications > 0))
        {
            FFileHelper::SaveStringToFile(TEXT("done"), *(Directory / (Role + TEXT("-done"))));
            Finish(true, TEXT("typed food and ammo, update notification, authority and privacy"));
        }
    }
}
