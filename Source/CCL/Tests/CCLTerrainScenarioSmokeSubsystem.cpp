#include "CCLTerrainScenarioSmokeSubsystem.h"

#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLExperimentPlayerController.h"
#include "Environment/CCLExperimentScreen.h"
#include "Environment/CCLTerrainRegion.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "UI/CCLGameUI.h"
#include "UI/Core/CCLUISubsystem.h"
#include "Camera/CameraActor.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

bool UCCLTerrainScenarioSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
    return false;
#else
    return FParse::Param(FCommandLine::Get(), TEXT("CCLTerrainScenarioSmoke"));
#endif
}

void UCCLTerrainScenarioSmokeSubsystem::Tick(float DeltaTime)
{
    const double Now = FPlatformTime::Seconds();
    if (!Started) { Started = Now; }
    if (Now - Started > 180.) { Finish(false, FString::Printf(TEXT("watchdog step=%d"), Step)); return; }
    auto* World = GetWorld();
    auto* Director = World ? ACCLExperimentDirector::Find(World) : nullptr;
    auto* Region = Director ? Director->GetTerrainRegion() : nullptr;
    auto* PC = World ? Cast<ACCLExperimentPlayerController>(World->GetFirstPlayerController()) : nullptr;
    if (Now < Next || !World || !World->HasBegunPlay() || !Director || !Director->IsReady()
        || !Region || !Region->IsTerrainReady() || Region->IsPreparing() || !PC || !PC->GetPawn()) { return; }
    auto* Runtime = World->GetSubsystem<UCCLWorldSimulationSubsystem>();
    auto* Agents = World->GetSubsystem<UCCLAgentWorldSubsystem>();
    // Keep explicit clock inputs exact while asynchronous terrain preparation spans frames.
    Runtime->SetTickableTickType(ETickableTickType::Never);
    FString Error;
    FString Directory;
    FString Mode;
    FParse::Value(FCommandLine::Get(), TEXT("CCLTerrainScenarioDir="), Directory);
    FParse::Value(FCommandLine::Get(), TEXT("CCLTerrainScenarioMode="), Mode);
    const bool bRendered = FParse::Param(FCommandLine::Get(), TEXT("CCLTerrainScenarioCapture"));
    auto Execute = [&](ECCLExperimentAction Action)
    {
        const auto* Result = Director->FindResult(TEXT("Zone_05"));
        return Director->Execute(PC, Action, TEXT("Zone_05"), Director->GetGeneration(), Result ? Result->RunId : FGuid(), Error, Region->GetPublicationSerial());
    };
    auto Height = [&](double X, double Y, double Z)
    {
        const FVector Origin = Region->GetActorLocation() + FVector(X, Y, 0.);
        FHitResult Hit;
        const bool bHit = World->LineTraceSingleByChannel(Hit, Origin + FVector(0.,0.,500.), Origin - FVector(0.,0.,350.), ECC_Visibility);
        if (!bHit || Hit.GetActor() != Region || FMath::Abs(Hit.ImpactPoint.Z - Origin.Z - Z) > 10.)
        {
            Error = FString::Printf(TEXT("collision at %.0f %.0f expected %.0f hit %s %s origin %s"), X,Y,Z,*GetNameSafe(Hit.GetActor()),*Hit.ImpactPoint.ToString(),*Origin.ToString());
            return false;
        }
        return true;
    };
    auto Check = [&](bool bPassed)
    {
        if (!bPassed) { Finish(false, FString::Printf(TEXT("step=%d %s"),Step,*Error)); }
        return bPassed;
    };
    if (Directory.IsEmpty() || (Mode != TEXT("write") && Mode != TEXT("read"))) { Finish(false,TEXT("isolated test parameters missing")); return; }
    if (Step == 0 && Mode == TEXT("read"))
    {
        TArray<uint8> ExpectedBytes;
        FCCLWorldSnapshot Expected;
        if (!Check(FFileHelper::LoadFileToArray(ExpectedBytes, *(Directory / TEXT("expected.bin")))
            && FCCLWorldSnapshotCodec::Decode(ExpectedBytes, Expected, Error)
            && Runtime->GetIdentity().WorldId != Expected.Identity.WorldId && Execute(ECCLExperimentAction::Load))) { return; }
        Step = 7;
        return;
    }
    if (Step == 0)
    {
        if (bRendered)
        {
            auto* UI = CCLGameUI::Get(PC);
            auto* Screen = UI ? Cast<UCCLExperimentScreen>(UI->FindScreen(PC->GetExperimentView())) : nullptr;
            if (!Screen) { return; }
            Screen->SelectCase(TEXT("Zone_05"));
            Step = 10;
            Next = Now + 1.;
            return;
        }
        if (!Check(Execute(ECCLExperimentAction::Start))) { return; }
        Step = 1;
        return;
    }
    if (Step == 10)
    {
        auto* UI = CCLGameUI::Get(PC);
        auto* Screen = UI ? Cast<UCCLExperimentScreen>(UI->FindScreen(PC->GetExperimentView())) : nullptr;
        const auto Button = Screen ? Screen->GetStartButton() : nullptr;
        if (!Button || Button->GetCachedGeometry().GetAbsoluteSize().IsNearlyZero()) { return; }
        const FVector2D Point = Button->GetCachedGeometry().GetAbsolutePosition() + Button->GetCachedGeometry().GetAbsoluteSize() * 0.5f;
        auto& App = FSlateApplication::Get();
        const auto Window = App.FindWidgetWindow(Button.ToSharedRef());
        App.ProcessMouseMoveEvent(FPointerEvent(0, Point, Point, TSet<FKey>(), EKeys::Invalid, 0, FModifierKeysState()));
        App.ProcessMouseButtonDownEvent(Window ? Window->GetNativeWindow() : nullptr,
            FPointerEvent(0, Point, Point, TSet<FKey>{EKeys::LeftMouseButton}, EKeys::LeftMouseButton, 0, FModifierKeysState()));
        App.ProcessMouseButtonUpEvent(FPointerEvent(0, Point, Point, TSet<FKey>(), EKeys::LeftMouseButton, 0, FModifierKeysState()));
        ++ClickAttempts;
        Step = 1;
        Next = Now + 0.25;
        return;
    }
    if (Step == 1)
    {
        const auto* Result = Director->FindResult(TEXT("Zone_05"));
        if (!Result || Result->Status == ECCLExperimentStatus::Running) { return; }
        if (Result->Status == ECCLExperimentStatus::Ready)
        {
            if (!bRendered || ClickAttempts >= 4) { Finish(false,TEXT("rendered Start pointer click did not start the selected case")); return; }
            // A newly visible panel can still have the preceding Slate hit-test grid.
            Step = 10;
            Next = Now + 0.25;
            return;
        }
        Error = Result->Detail;
        if (!Check(Result->Status == ECCLExperimentStatus::Passed && Height(0.,0.,-150.) && Execute(ECCLExperimentAction::TerrainDeposit))) { return; }
        Step = 2;
        return;
    }
    if (Step == 2)
    {
        if (!Check(Region->DidLastRequestSucceed() && Height(400.,300.,150.) && Execute(ECCLExperimentAction::TerrainChannel))) { return; }
        Step = 3;
        return;
    }
    if (Step == 3)
    {
        if (!Region->IsNavigationReady()) { return; }
        FCCLWorldSnapshot Expected;
        TArray<uint8> ExpectedBytes;
        if (!Check(Region->DidLastRequestSucceed() && Height(0.,200.,-150.) && Region->GetTerrainStore().GetRevision() == 4
            && Runtime->ApplySnowContact(ACCLExperimentDirector::SnowRegionId(), FGuid(15, 1, 1, 1), 1, UCCLExperimentDefinition::ZoneCenter(3) / 100., 0.27, Error)
            && Runtime->QueueGameTime(37., Error) && Runtime->AdvancePending(40.,2400.,Error)
            && Runtime->ChangeTimeScale(7.,Error) && Runtime->QueueGameTime(1.25,Error) && Execute(ECCLExperimentAction::Save)
            && FCCLWorldSnapshotCodec::Capture(Runtime->GetIdentity(),Runtime->GetClock(),Agents->GetSimulation(),Expected,Error,&Runtime->GetEnvironmentInputs(), &Runtime->GetSurfaceSimulation())
            && FCCLWorldSnapshotCodec::Encode(Expected,ExpectedBytes,Error)
            && FFileHelper::SaveArrayToFile(ExpectedBytes, *(Directory / TEXT("expected.bin"))))) { return; }
        if (bRendered) { FScreenshotRequest::RequestScreenshot(Directory / TEXT("terrain-controls.png"),true,false); }
        Step = 4;
        Next = Now + 1.;
        return;
    }
    if (Step == 4)
    {
        if (bRendered)
        {
            PC->CCLExperiment();
            auto* Camera = World->SpawnActor<ACameraActor>();
            const FVector Target = Region->GetActorLocation();
            Camera->SetActorLocation(Target + FVector(-1500.,-1700.,1700.));
            Camera->SetActorRotation((Target-Camera->GetActorLocation()).Rotation());
            PC->SetViewTarget(Camera);
        }
        Step = 5;
        Next = Now + 1.;
        return;
    }
    if (Step == 5)
    {
        if (bRendered) { FScreenshotRequest::RequestScreenshot(Directory / TEXT("terrain-overview.png"),false,false); }
        Step = 6;
        Next = Now + 1.;
        return;
    }
    if (Step == 6)
    {
        if (!Check(Execute(ECCLExperimentAction::TerrainReset))) { return; }
        Step = 8;
        return;
    }
    if (Step == 8)
    {
        if (!Check(Region->GetTerrainStore().GetRevision() == 1 && Height(0.,0.,0.) && Height(400.,300.,0.)
            && Execute(ECCLExperimentAction::Load))) { return; }
        Step = 7;
        return;
    }
    if (Step == 7)
    {
        TArray<uint8> ExpectedBytes, NormalizedExpected, ActualBytes;
        FCCLWorldSnapshot Expected, Actual;
        FCCLLifeSimulation ExpectedLife;
        if (!Check(Region->DidLastRequestSucceed() && Region->GetTerrainStore().GetRevision() == 4
            && Height(0.,0.,-150.) && Height(0.,200.,-150.) && Height(400.,300.,150.)
            && FFileHelper::LoadFileToArray(ExpectedBytes, *(Directory / TEXT("expected.bin")))
            && FCCLWorldSnapshotCodec::Decode(ExpectedBytes,Expected,Error)
            && ExpectedLife.Load(Expected.Life,Error) && ExpectedLife.Save(Expected.Life)
            && FCCLWorldSnapshotCodec::Encode(Expected,NormalizedExpected,Error)
            && FCCLWorldSnapshotCodec::Capture(Runtime->GetIdentity(),Runtime->GetClock(),Agents->GetSimulation(),Actual,Error,&Runtime->GetEnvironmentInputs(), &Runtime->GetSurfaceSimulation())
            && FCCLWorldSnapshotCodec::Encode(Actual,ActualBytes,Error) && ActualBytes == NormalizedExpected)) { return; }
        if (Mode == TEXT("write"))
        {
            Step = 9;
            Next = Now + 0.5;
            return;
        }
        Finish(true, TEXT("read restores saved world, clock, life and changed terrain collision"));
    }
    if (Step == 9)
    {
        if (!Check(Execute(ECCLExperimentAction::Start) && Execute(ECCLExperimentAction::Stop))) { return; }
        Step = 11;
        Next = Now + 0.5;
        return;
    }
    if (Step == 11)
    {
        if (!Check(Region->GetTerrainStore().GetRevision() == 4 && Height(0.,0.,-150.)
            && Director->FindResult(TEXT("Zone_05"))->Status == ECCLExperimentStatus::Failed)) { return; }
        Finish(true,TEXT("write controls, excavation, deposit, channel, navigation, reset, restore and cancelled rerun"));
    }
}

TStatId UCCLTerrainScenarioSmokeSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLTerrainScenarioSmokeSubsystem, STATGROUP_Tickables);
}

void UCCLTerrainScenarioSmokeSubsystem::Finish(bool bSuccess, const FString& Message)
{
    UE_LOG(LogTemp, Display, TEXT("CCL_TERRAIN_SCENARIO %s %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Message);
    bComplete = 1;
    FPlatformMisc::RequestExitWithStatus(false,bSuccess ? 0 : 2);
}
