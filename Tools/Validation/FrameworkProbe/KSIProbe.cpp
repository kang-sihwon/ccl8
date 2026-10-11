#include "KSIProbe.h"

#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Modules/ModuleManager.h"
#if WITH_KSI_UI
#include "KSIScreen.h"
#include "KSIUIContext.h"
#include "KSIUIRegistry.h"
#include "KSIUISubsystem.h"
#include "NativeGameplayTags.h"
#include "CommonGameViewportClient.h"
#include "Input/CommonUIActionRouterBase.h"
#include "EnhancedInputComponent.h"
#include "EnhancedPlayerInput.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputKeyEventArgs.h"
UE_DEFINE_GAMEPLAY_TAG_STATIC(ProbeLayer, "UI.Layer.Tools");
UE_DEFINE_GAMEPLAY_TAG_STATIC(ProbeView, "UI.View.Tool");
#endif
#if WITH_KSI_MAP
#include "KSIMapSystem.h"
#endif

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, KSIProbe, "KSIProbe");

bool UKSIProbe::ShouldCreateSubsystem(UObject* Outer) const
{
    const auto* World = Cast<UWorld>(Outer);
    return World && World->IsGameWorld();
}

TStatId UKSIProbe::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UKSIProbe, STATGROUP_Tickables);
}

void UKSIProbe::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    auto* PC = GetWorld()->GetFirstPlayerController();
    if (bFinished || !PC || !PC->GetPawn() || !PC->GetLocalPlayer())
    {
        return;
    }
    StepSeconds += DeltaTime;
    if (StepSeconds < 0.2f)
    {
        return;
    }
    StepSeconds = 0.f;
    bPassed = bPassed && !FModuleManager::Get().IsModuleLoaded("CCL") && !FModuleManager::Get().IsModuleLoaded("CCLEditor");
    auto Check = [this](bool Condition, const TCHAR* Message)
    {
        bPassed = bPassed && Condition;
        UE_LOG(LogTemp, Display, TEXT("KSI_PROBE %s %s"), Condition ? TEXT("CHECK") : TEXT("FAIL"), Message);
    };
#if WITH_KSI_UI
    Check(!FModuleManager::Get().IsModuleLoaded("KSIMap"), TEXT("UI has no map module"));
    auto* Local = PC->GetLocalPlayer();
    auto* UI = Local->GetSubsystem<UKSIUISubsystem>();
    auto* Router = Local->GetSubsystem<UCommonUIActionRouterBase>();
    auto* Input = Cast<UEnhancedInputComponent>(PC->InputComponent);
    auto* PlayerInput = Cast<UEnhancedPlayerInput>(PC->PlayerInput);
    auto* Viewport = Cast<UCommonGameViewportClient>(GetWorld()->GetGameViewport());
    Check(UI && Router && Input && PlayerInput && Viewport && Viewport->Viewport,
        TEXT("Enhanced Input and CommonUI runtime are configured"));
    if (bPassed)
    {
        auto SendKey = [Viewport](EInputEvent Event)
        {
            Viewport->InputKey(FInputKeyEventArgs(Viewport->Viewport, FInputDeviceId::CreateFromInternalId(0),
                EKeys::K, Event, FPlatformTime::Cycles64()));
        };
        switch (Step++)
        {
        case 0:
        {
            auto* Registry = NewObject<UKSIUIRegistry>(this);
            Registry->Layers.Add({ProbeLayer, EKSIUILayerLayout::Overlay, 0});
            Registry->InputMapping = NewObject<UInputMappingContext>(Registry);
            auto* Action = NewObject<UInputAction>(Registry->InputMapping);
            Action->ValueType = EInputActionValueType::Boolean;
            Registry->InputMapping->MapKey(Action, EKeys::K);
            FKSIUIViewDefinition Definition;
            Definition.Tag = ProbeView;
            Definition.Layer = ProbeLayer;
            Definition.WidgetClass = UKSIScreen::StaticClass();
            Definition.RequiredContextClass = UKSIUIContext::StaticClass();
            Definition.InputPolicy = EKSIUIInputPolicy::Game;
            Registry->Views.Add(Definition);
            Check(UI->ConfigureRegistry(Registry), TEXT("configure data-driven tool layout"));
            const auto Handle = UI->OpenView(ProbeView, NewObject<UKSIUIContext>(this), this);
            ViewId = Handle.Id;
            Check(Handle.IsValid() && UI->FindScreen(Handle), TEXT("open tool screen without game content"));
            break;
        }
        case 1:
            Check(Local->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()->HasMappingContext(UI->GetRegistry()->InputMapping),
                TEXT("CommonUI activates the registry mapping context"));
            Check(Router->CanProcessNormalGameInput(), TEXT("initial router accepts gameplay input"));
            SendKey(IE_Pressed);
            break;
        case 2:
            Check(PlayerInput->GetActionValue(UI->GetRegistry()->InputMapping->GetMapping(0).Action).Get<bool>(), TEXT("viewport key sets Enhanced Input action value"));
            SendKey(IE_Released);
            break;
        case 3:
        {
            FKSIUIPresentationDefinition Presentation;
            Presentation.bAllViews = 1;
            Presentation.bBlockGameplay = 1;
            const auto Request = UI->PushPresentation(Presentation, this);
            PresentationId = Request.Id;
            Check(Request.IsValid(), TEXT("presentation request is valid"));
            break;
        }
        case 4:
            Check(UI->IsGameplayInputBlocked() && !Router->CanProcessNormalGameInput(),
                TEXT("presentation blocks the actual input router"));
            SendKey(IE_Pressed);
            break;
        case 5:
            Check(!PlayerInput->GetActionValue(UI->GetRegistry()->InputMapping->GetMapping(0).Action).Get<bool>(), TEXT("blocked viewport key leaves game action value zero"));
            SendKey(IE_Released);
            UI->ReleasePresentation({PresentationId});
            break;
        case 6:
            Check(!UI->IsGameplayInputBlocked() && Router->CanProcessNormalGameInput(),
                TEXT("release restores the actual input router"));
            SendKey(IE_Pressed);
            break;
        case 7:
            Check(PlayerInput->GetActionValue(UI->GetRegistry()->InputMapping->GetMapping(0).Action).Get<bool>(), TEXT("released viewport key sets game action value again"));
            SendKey(IE_Released);
            UI->CloseView({ViewId});
            break;
        case 8:
            Check(UI->GetViewCount() == 0 && UI->GetPresentationCount() == 0 && Router->CanProcessNormalGameInput(),
                TEXT("screen lifetime ends and routing remains restored"));
            break;
        }
    }
    if (bPassed && Step <= 8)
    {
        return;
    }
#endif
#if WITH_KSI_MAP
    Check(!FModuleManager::Get().IsModuleLoaded("KSIUI"), TEXT("map has no UI module"));
    auto* Map = PC->GetLocalPlayer()->GetSubsystem<UKSIMapSubsystem>();
    Check(Map != nullptr, TEXT("observer subsystem created"));
    if (Map)
    {
        auto* Region = NewObject<UKSIMapRegionDefinition>(this);
        Region->Min = FVector2D(-100, -100);
        Region->Max = FVector2D(100, 100);
        Region->Terrain.Add({});
        Map->SetRegion(Region);
        auto* Actor = GetWorld()->SpawnActor<AActor>();
        auto* Marker = NewObject<UKSIMapMarkerComponent>(Actor);
        Actor->AddInstanceComponent(Marker);
        Marker->RegisterComponent();
        Marker->bRequireLineOfSight = 0;
        Marker->RevealDistance = 100000;
        Map->Refresh(PC);
        Check(Map->GetMarkers().Num() == 1 && Map->GetTerrain().Num() == 1, TEXT("authored terrain and generic actor marker"));
        Check(UKSIMapSubsystem::Project(FVector::ZeroVector, Map->GetBounds(), FVector2D(200, 100)).Equals(FVector2D(100, 50)), TEXT("map projection"));
        Marker->RevealDistance = 0;
        Map->Refresh(PC);
        Check(Map->GetMarkers().IsEmpty(), TEXT("observer visibility policy"));
        Actor->Destroy();
        Map->Refresh(nullptr);
        Check(Map->GetMarkers().IsEmpty(), TEXT("observer release clears markers"));
    }
#endif
    bFinished = 1;
    UE_LOG(LogTemp, Display, TEXT("KSI_PROBE %s"), bPassed ? TEXT("PASS") : TEXT("FAIL"));
    FPlatformMisc::RequestExitWithStatus(false, bPassed ? 0 : 1);
}
