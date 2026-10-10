#include "CCLFoodZoneSmokeSubsystem.h"

#include "Environment/CCLFoodExperiment.h"
#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLExperimentPlayerController.h"
#include "Environment/CCLExperimentScreen.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Environment/CCLWorldSnapshot.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Items/CCLInventoryComponent.h"
#include "Items/CCLWorldPickup.h"
#include "UI/CCLGameUI.h"
#include "UI/Core/CCLUISubsystem.h"
#include "Camera/CameraActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UnrealClient.h"
#include "Misc/FileHelper.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"

bool UCCLFoodZoneSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
    return false;
#else
    return FParse::Param(FCommandLine::Get(), TEXT("CCLFoodZoneSmoke"));
#endif
}

void UCCLFoodZoneSmokeSubsystem::Tick(float DeltaTime)
{
    const double Now = FPlatformTime::Seconds();
    if (!Started)
    {
        Started = Now;
    }
    if (Now - Started > 240.)
    {
        Finish(false, FString::Printf(TEXT("timeout step=%d"), Step));
        return;
    }
    auto* World = GetWorld();
    auto* Director = World ? ACCLExperimentDirector::Find(World) : nullptr;
    auto* Runtime = World ? World->GetSubsystem<UCCLWorldSimulationSubsystem>() : nullptr;
    auto* Food = World ? ACCLFoodExperiment::Find(World) : nullptr;
    if (!Director || !Director->IsReady() || !Runtime || !Food || Food->GetView().Rows.Num() != 3 || Now < Next)
    {
        return;
    }
    auto* PC = Cast<ACCLExperimentPlayerController>(World->GetFirstPlayerController());
    auto* Agents = World->GetSubsystem<UCCLAgentWorldSubsystem>();
    const auto& View = Food->GetView();
    FString Error;
    auto Check = [&](bool bOK, const TCHAR* Message)
    {
        if (!bOK)
        {
            for (int32 Index = 0; Index < View.Rows.Num(); ++Index)
            {
                const auto& Row = View.Rows[Index];
                UE_LOG(LogTemp, Display, TEXT("CCL_FOOD_ZONE ROW %d count=%d spoiled=%d age=%f fresh=%f mold=%f temp=%f"),
                    Index, Row.Quantity, Row.Spoiled, Row.AgeSeconds, Row.Freshness, Row.Mold, Row.Temperature);
            }
            Finish(false, FString::Printf(TEXT("step=%d %s %s"), Step, Message, *Error));
        }
        return bOK;
    };
    auto Execute = [&](ECCLExperimentAction Action)
    {
        return Director->Execute(PC, Action, TEXT("Zone_07"), Director->GetGeneration(),
                                 Director->FindResult(TEXT("Zone_07"))->RunId, Error);
    };
    const bool bRendered = FParse::Param(FCommandLine::Get(), TEXT("CCLFoodZoneCapture"));
    auto Capture = [&](const TCHAR* Name)
    {
        if (bRendered)
        {
            FString Directory;
            FParse::Value(FCommandLine::Get(), TEXT("CCLFoodZoneSmokeDir="), Directory);
            FScreenshotRequest::RequestScreenshot(Directory / Name, true, false);
        }
    };
    auto Screen = [&]() -> UCCLExperimentScreen*
    {
        auto* UI = PC ? CCLGameUI::Get(PC) : nullptr;
        return UI ? Cast<UCCLExperimentScreen>(UI->FindScreen(PC->GetExperimentView())) : nullptr;
    };
    if (World->GetNetMode() == NM_Client)
    {
        FString Directory, Oracle;
        FParse::Value(FCommandLine::Get(), TEXT("CCLFoodZoneSmokeDir="), Directory);
        if (!FFileHelper::LoadFileToString(Oracle, *(Directory / TEXT("final-view.txt")))) { return; }
        TArray<FString> Fields;
        Oracle.ParseIntoArray(Fields, TEXT(","));
        if (Fields.Num() != 14) { return; }
        if (View.Epoch.ToString() != Fields[0] || !FMath::IsNearlyEqual(View.WorldSeconds, FCString::Atod(*Fields[1]), 1.e-8)
            || Food->GetRenderedCount() != 3) { return; }
        for (int32 Index = 0; Index < 3; ++Index)
        {
            const auto& Row = View.Rows[Index];
            const int32 Base = 2 + Index * 4;
            if (Row.Quantity != FCString::Atoi(*Fields[Base]) || Row.Spoiled != FCString::Atoi(*Fields[Base + 3]) ||
                !FMath::IsNearlyEqual(Row.Freshness, FCString::Atod(*Fields[Base + 1]), 1.e-8) ||
                !FMath::IsNearlyEqual(Row.Mold, FCString::Atod(*Fields[Base + 2]), 1.e-8)) { return; }
        }
        if (Check(Food->GetInventory(0)->GetEntries().IsEmpty() && !Food->Execute(ECCLExperimentAction::FoodAdd, Error),
            TEXT("private inventory and server ownership")))
        {
            Finish(true, TEXT("final restored epoch time and public food quality match server"));
        }
        return;
    }
    auto AdvanceWorld = [&]()
    {
        if (!Runtime->GetClock().HasPendingTime() && !Check(Runtime->QueueGameTime(.25, Error), TEXT("queue bounded food time"))) { return; }
        if (!Runtime->AdvancePending(.25, 1., Error)) { Check(Error.Contains(TEXT("awaits")), TEXT("advance bounded food time")); }
    };
    if (Step == 0)
    {
        Runtime->SetTickableTickType(ETickableTickType::Never);
        if (!Check(Runtime->ChangeTimeScale(0., Error), TEXT("pause")))
        {
            return;
        }
        UE_LOG(LogTemp, Display, TEXT("CCL_FOOD_ZONE BASE_READY"));
        Step = 1;
        Next = Now + 2.;
        return;
    }
    if (!PC || !Director->CanOperate(PC))
    {
        return;
    }
    if (Step == 1)
    {
        if (!Check(View.Rows[0].Quantity == 4 && View.Rows[1].Quantity == 4 && View.Rows[2].Quantity == 4,
                   TEXT("three authored containers")))
        {
            return;
        }
        if (bRendered)
        {
            auto* Camera = World->SpawnActor<ACameraActor>(
                UCCLExperimentDefinition::ZoneCenter(7) + FVector(0, -1600, 520), FRotator(-22, 90, 0));
            PC->SetViewTarget(Camera);
            auto* UI = CCLGameUI::Get(PC);
            if (!UI->IsViewOpen(PC->GetExperimentView()))
            {
                PC->CCLExperiment();
            }
            if (auto* Current = Screen())
            {
                Current->SelectCase(TEXT("Zone_07"));
                Current->RevealFoodControls();
            }
        }
        Step = 2;
        Next = Now + 3.;
        return;
    }
    auto ClickFood = [&](bool bDrop)
    {
        if (!bRendered)
        {
            return Execute(bDrop ? ECCLExperimentAction::FoodDrop : ECCLExperimentAction::FoodCollect);
        }
        auto* Current = Screen();
        auto Control = Current ? (bDrop ? Current->GetFoodDropButton() : Current->GetFoodCollectButton()) : nullptr;
        if (!Control || !Control->IsEnabled() || Control->GetCachedGeometry().GetAbsoluteSize().IsNearlyZero())
        {
            return false;
        }
        const FVector2D Point =
            Control->GetCachedGeometry().GetAbsolutePosition() + Control->GetCachedGeometry().GetAbsoluteSize() * .5;
        auto& App = FSlateApplication::Get();
        const auto Window = App.FindWidgetWindow(Control.ToSharedRef());
        App.ProcessMouseMoveEvent(
            FPointerEvent(0, Point, Point, TSet<FKey>(), EKeys::Invalid, 0, FModifierKeysState()));
        App.ProcessMouseButtonDownEvent(Window ? Window->GetNativeWindow() : nullptr,
                                        FPointerEvent(0, Point, Point, TSet<FKey>{EKeys::LeftMouseButton},
                                                      EKeys::LeftMouseButton, 0, FModifierKeysState()));
        App.ProcessMouseButtonUpEvent(
            FPointerEvent(0, Point, Point, TSet<FKey>(), EKeys::LeftMouseButton, 0, FModifierKeysState()));
        return true;
    };
    if (Step == 2)
    {
        if (!Check(ClickFood(true), TEXT("drop control")))
        {
            return;
        }
        Step = 201;
        Next = Now + 1.;
        return;
    }
    if (Step == 201)
    {
        if (!Check(Food->FindGroundSample() && View.GroundQuantity == 1 && View.Rows[0].Quantity == 3 &&
                       !Execute(ECCLExperimentAction::FoodDrop) &&
                       Runtime->Save(Agents->GetSimulation(), GroundSaved, Error),
                   TEXT("single ground sample saved")))
        {
            return;
        }
        Capture(TEXT("food-ground.png"));
        Step = 2010;
        Next = Now + 1.;
        return;
    }
    if (Step == 2010)
    {
        const auto* Grid = Runtime->GetSurfaceSimulation().FindRegion(Director->WeatherRegionId());
        if (!Check(Grid != nullptr, TEXT("weather grid")))
        {
            return;
        }
        Food->FindGroundSample()->SetActorLocation((Grid->OriginMeters + FVector(.25, .25, 1.)) * 100.);
        if (!Check(Runtime->ChangeTimeScale(4., Error), TEXT("position clock")))
        {
            return;
        }
        TargetWorld = Runtime->GetClock().GetWorldSeconds() + 2.;
        Step = 2011;
        return;
    }
    if (Step == 2011)
    {
        if (Runtime->GetClock().GetWorldSeconds() < TargetWorld)
        {
            AdvanceWorld();
            return;
        }
        ACCLWorldPickup* Pickup = nullptr;
        for (TActorIterator<ACCLWorldPickup> It(World); It; ++It)
        {
            if (!It->IsActorBeingDestroyed() && It->Definition == Food->GetInventory(0)->GetEntries()[0].Definition)
            {
                Pickup = *It;
                break;
            }
        }
        if (!Check(Pickup &&
                       Pickup->GetFoodInventory()->GetFoodEnvironment().Source == ECCLFoodEnvironment::WeatherRegion,
                   TEXT("live pickup uses outdoor region")))
        {
            return;
        }
        const auto* Room = Runtime->GetSurfaceSimulation().GetAirSimulation().FindSpace(Director->FireSpaceId());
        if (!Check(Room != nullptr, TEXT("indoor room")))
        {
            return;
        }
        Pickup->SetActorLocation((Room->MinimumMeters + Room->MaximumMeters) * 50.);
        GroundPickup = Pickup;
        TargetWorld = Runtime->GetClock().GetWorldSeconds() + 2.;
        Step = 2012;
        return;
    }
    if (Step == 2012)
    {
        if (Runtime->GetClock().GetWorldSeconds() < TargetWorld)
        {
            AdvanceWorld();
            return;
        }
        if (!Check(GroundPickup.IsValid() &&
                       GroundPickup->GetFoodInventory()->GetFoodEnvironment().Source == ECCLFoodEnvironment::AirSpace,
                   TEXT("live pickup enters indoor air")))
        {
            return;
        }
        GroundId = GroundPickup->PersistentId;
        if (!Check(Runtime->Save(Agents->GetSimulation(), IndoorSaved, Error), TEXT("save indoor pickup"))) { return; }
        GroundPickup->Destroy();
        TargetWorld = Runtime->GetClock().GetWorldSeconds() + 2.;
        Step = 20121;
        return;
    }
    if (Step == 20121)
    {
        if (Runtime->GetClock().GetWorldSeconds() < TargetWorld) { AdvanceWorld(); return; }
        TArray<uint8> OfflineBytes;
        FCCLWorldSnapshot Offline;
        if (!Check(Runtime->Save(Agents->GetSimulation(), OfflineBytes, Error) &&
            FCCLWorldSnapshotCodec::Decode(OfflineBytes, Offline, Error), TEXT("save unloaded indoor food"))) { return; }
        const auto* Record = Offline.Inventories.FindByPredicate([this](const auto& Value) { return Value.ContainerId == GroundId; });
        FCCLFoodState FoodState;
        if (!Check(Record && Record->Environment.Source == ECCLFoodEnvironment::AirSpace && Record->Items.Num() == 1 &&
            CCLFood::Decode(Record->Items[0].Food, FoodState, Error) &&
            FMath::IsNearlyEqual(FoodState.UpdatedWorldSeconds, Offline.Clock.WorldSeconds, 1.e-8) &&
            Runtime->Restore(Agents->GetSimulation(), IndoorSaved, Error), TEXT("unloaded food continues at saved indoor position"))) { return; }
        GroundPickup.Reset();
        for (TActorIterator<ACCLWorldPickup> It(World); It; ++It)
        {
            if (!It->IsActorBeingDestroyed() && It->PersistentId == GroundId) { GroundPickup = *It; break; }
        }
        if (!Check(GroundPickup.IsValid() && GroundPickup->GetFoodInventory()->GetFoodEnvironment().Source == ECCLFoodEnvironment::AirSpace,
            TEXT("indoor pickup recreated with its binding"))) { return; }
        GroundPickup->SetActorLocation(FVector(-5000., -5000., 10000.));
        TargetWorld = Runtime->GetClock().GetWorldSeconds() + 2.;
        Step = 2013;
        return;
    }
    if (Step == 2013)
    {
        if (Runtime->GetClock().GetWorldSeconds() < TargetWorld)
        {
            AdvanceWorld();
            return;
        }
        if (!Check(GroundPickup.IsValid() && !GroundPickup->GetFoodInventory()->GetFoodEnvironment().IsDynamic() &&
                       Runtime->ChangeTimeScale(0., Error),
                   TEXT("outside region fallback")))
        {
            return;
        }
        GroundPickup->SetActorLocation(Food->GetGroundPosition());
        if (!Check(ClickFood(false), TEXT("collect control")))
        {
            return;
        }
        Step = 202;
        Next = Now + 1.;
        return;
    }
    if (Step == 202)
    {
        if (!Check(!Food->FindGroundSample() && Food->GetInventory(0)->GetEntries()[0].Quantity == 4 &&
                       Runtime->Restore(Agents->GetSimulation(), GroundSaved, Error),
                   TEXT("collect and restore dropped sample")))
        {
            return;
        }
        Step = 203;
        Next = Now + 1.;
        return;
    }
    if (Step == 203)
    {
        if (!Check(Food->FindGroundSample() && View.GroundQuantity == 1 && View.Rows[0].Quantity == 3 &&
                       Execute(ECCLExperimentAction::Reset),
                   TEXT("restored pickup then reset")))
        {
            return;
        }
        Step = 204;
        Next = 0.;
        return;
    }
    if (Step == 204)
    {
        if (Director->IsRestoring())
        {
            return;
        }
        if (!Check(Runtime->ChangeTimeScale(0., Error) && !Food->FindGroundSample() &&
                   Food->GetInventory(0)->GetEntries()[0].Quantity == 4,
                   TEXT("pause reset world, remove pickup and restore four foods")))
        {
            return;
        }
        Step = 205;
        Next = Now + 1.;
        return;
    }
    if (Step == 205)
    {
        if (bRendered)
        {
            auto* Current = Screen();
            auto Control = Current ? Current->GetFoodAddButton() : nullptr;
            if (!Check(Control && Control->IsEnabled() &&
                           !Control->GetCachedGeometry().GetAbsoluteSize().IsNearlyZero(),
                       TEXT("food add button visible")))
            {
                return;
            }
            const FVector2D Point = Control->GetCachedGeometry().GetAbsolutePosition() +
                                    Control->GetCachedGeometry().GetAbsoluteSize() * 0.5;
            auto& App = FSlateApplication::Get();
            const auto Window = App.FindWidgetWindow(Control.ToSharedRef());
            App.ProcessMouseMoveEvent(
                FPointerEvent(0, Point, Point, TSet<FKey>(), EKeys::Invalid, 0, FModifierKeysState()));
            App.ProcessMouseButtonDownEvent(Window ? Window->GetNativeWindow() : nullptr,
                                            FPointerEvent(0, Point, Point, TSet<FKey>{EKeys::LeftMouseButton},
                                                          EKeys::LeftMouseButton, 0, FModifierKeysState()));
            App.ProcessMouseButtonUpEvent(
                FPointerEvent(0, Point, Point, TSet<FKey>(), EKeys::LeftMouseButton, 0, FModifierKeysState()));
        }
        else if (!Check(Execute(ECCLExperimentAction::FoodAdd), TEXT("add fresh sample")))
        {
            return;
        }
        TargetWorld = Now + 2.;
        Step = 21;
        return;
    }
    if (Step == 21)
    {
        if (Food->GetInventory(0)->GetEntries()[0].Quantity != 6 && Now < TargetWorld)
        {
            return;
        }
        if (!Check(Food->GetInventory(0)->GetEntries()[0].Quantity == 6 && Execute(ECCLExperimentAction::FoodMoveCold),
                   TEXT("button adds and transfer preserves counts")))
        {
            return;
        }
        Step = 22;
        Next = Now + 1.;
        return;
    }
    if (Step == 22)
    {
        Capture(TEXT("food-controls.png"));
        if (!Check(Runtime->ChangeTimeScale(4., Error), TEXT("advance food aging")))
        {
            return;
        }
        TargetWorld = Runtime->GetClock().GetWorldSeconds() + 360.;
        Step = 3;
        return;
    }
    if (Step == 3)
    {
        if (View.WorldSeconds < TargetWorld)
        {
            if (Runtime->GetClock().GetWorldSeconds() < TargetWorld) { AdvanceWorld(); }
            return;
        }
        if (!Check(View.Rows[0].Quantity == 5 && View.Rows[0].Spoiled == 5 && View.Rows[0].Mold > 0.9 &&
                       View.Rows[1].Quantity == 5 && View.Rows[1].Freshness > 0.98 && View.Rows[1].Mold == 0. &&
                       View.Rows[2].Freshness > 0.7 && View.Rows[2].Mold == 0. && Runtime->ChangeTimeScale(0., Error),
                   TEXT("open rotten cold preserved sealed slower")))
        {
            return;
        }
        if (bRendered && Screen())
        {
            PC->CCLExperiment();
        }
        Step = 4;
        Next = Now + 2.;
        return;
    }
    if (Step == 4)
    {
        Capture(TEXT("food-states.png"));
        if (!Check(Execute(ECCLExperimentAction::FoodThaw) && Runtime->ChangeTimeScale(4., Error), TEXT("thaw")))
        {
            return;
        }
        TargetWorld = Runtime->GetClock().GetWorldSeconds() + 150.;
        Step = 5;
        return;
    }
    if (Step == 5)
    {
        if (View.WorldSeconds < TargetWorld)
        {
            if (Runtime->GetClock().GetWorldSeconds() < TargetWorld) { AdvanceWorld(); }
            return;
        }
        if (!Check(View.Rows[1].Mold > 0.05 && View.Rows[1].Freshness < 0.6 && Runtime->ChangeTimeScale(0., Error),
                   TEXT("thaw continues existing history")))
        {
            return;
        }
        Step = 6;
        Next = Now + 2.;
        return;
    }
    if (Step == 6)
    {
        FoodBefore.Reset();
        for (int32 Index = 0; Index < 3; ++Index)
        {
            FString Encoded;
            if (!Check(CCLFood::Encode(Food->GetInventory(Index)->GetEntries()[0].Food, Encoded, Error),
                       TEXT("capture food before save")))
            {
                return;
            }
            FoodBefore.Add(Encoded);
        }
        if (!Check(Execute(ECCLExperimentAction::Save) && Runtime->Save(Agents->GetSimulation(), Saved, Error) &&
                       Execute(ECCLExperimentAction::FoodAdd) && Execute(ECCLExperimentAction::FoodThaw) &&
                       Execute(ECCLExperimentAction::Load),
                   TEXT("saved world and terrain restored together")))
        {
            return;
        }
        Step = 7;
        Next = Now + 2.;
        return;
    }
    if (Step == 7)
    {
        if (Director->IsRestoring())
        {
            return;
        }
        for (int32 Index = 0; Index < 3; ++Index)
        {
            FString Encoded;
            if (!Check(CCLFood::Encode(Food->GetInventory(Index)->GetEntries()[0].Food, Encoded, Error) &&
                           Encoded == FoodBefore[Index],
                       TEXT("food history exact after experiment load")))
            {
                return;
            }
        }
        if (!Check(Execute(ECCLExperimentAction::Reset), TEXT("reset baseline")))
        {
            return;
        }
        Step = 8;
        Next = Now + 1.;
        return;
    }
    if (Step == 8)
    {
        if (Director->IsRestoring())
        {
            return;
        }
        if (!Check(Food->GetInventory(0)->GetEntries()[0].Quantity == 4 &&
                       Food->GetInventory(1)->GetEntries()[0].Food.Storage.TemperatureCelsius == -5. &&
                       Runtime->Restore(Agents->GetSimulation(), Saved, Error),
                   TEXT("reset authored samples then restore saved food")))
        {
            return;
        }
        Step = 9;
        Next = Now + 3.;
        return;
    }
    if (!Check(View.Rows[0].Quantity == 5 && View.Rows[1].Mold > 0.05 &&
                   (World->GetNetMode() == NM_DedicatedServer || Food->GetRenderedCount() == 3),
               TEXT("final observed food state")))
    {
        return;
    }
    if (View.Epoch != Runtime->GetEpoch() || !FMath::IsNearlyEqual(View.WorldSeconds, Runtime->GetClock().GetWorldSeconds(), 1.e-8)) { return; }
    FString Directory;
    FParse::Value(FCommandLine::Get(), TEXT("CCLFoodZoneSmokeDir="), Directory);
    FString Oracle = FString::Printf(TEXT("%s,%.17g"), *View.Epoch.ToString(), View.WorldSeconds);
    for (const auto& Row : View.Rows)
    {
        Oracle += FString::Printf(TEXT(",%d,%.17g,%.17g,%d"), Row.Quantity, Row.Freshness, Row.Mold, Row.Spoiled);
    }
    if (!Check(FFileHelper::SaveStringToFile(Oracle, *(Directory / TEXT("final-view.txt"))), TEXT("save public view oracle"))) { return; }
    UE_LOG(LogTemp, Display, TEXT("CCL_FOOD_ZONE RESTORE_READY"));
    Finish(true, TEXT("zone07 controls storage aging transfer thaw save load and reset verified"));
}

void UCCLFoodZoneSmokeSubsystem::Finish(bool bSuccess, const FString& Message)
{
    bComplete = 1;
    UE_LOG(LogTemp, Display, TEXT("CCL_FOOD_ZONE %s %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Message);
    if (!bSuccess || (GetWorld() && GetWorld()->GetNetMode() == NM_Standalone))
    {
        FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1, TEXT("CCLFoodZoneSmoke"));
    }
}

TStatId UCCLFoodZoneSmokeSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLFoodZoneSmokeSubsystem, STATGROUP_Tickables);
}
