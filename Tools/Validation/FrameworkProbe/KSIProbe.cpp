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

#if WITH_KSI_CORE || WITH_KSI_INVENTORY
#include "KSIWorldParticipant.h"
#include "KSIWorldTime.h"
#endif
#if WITH_KSI_INVENTORY
#include "KSIInventoryComponent.h"
#include "KSIItemDefinition.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#endif

#if WITH_KSI_COMBAT
#include "KSICombatProbe.h"
#endif

#if WITH_KSI_WORLD
#include "KSIWorldProbe.h"
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
#if WITH_KSI_CORE
    Check(!FModuleManager::Get().IsModuleLoaded("CCL") && !FModuleManager::Get().IsModuleLoaded("KSIInventory"), TEXT("contracts have no game or inventory module"));
    FKSIWorldSectionRegistry::Register(TEXT("Probe"), 10,
        [](const FKSIWorldSectionContext& Context, const TArray<uint8>& Bytes, FString& Error)
        {
            return Context.WorldId.IsValid() && Context.WorldSeconds == 42. && Bytes.Num() == 1;
        });
    FKSIWorldSectionContext Context;
    Context.WorldId = FGuid::NewGuid();
    Context.WorldSeconds = 42.;
    FString Error;
    const auto* Section = FKSIWorldSectionRegistry::Find(TEXT("Probe"));
    Check(Section && Section->Validator(Context, {1}, Error), TEXT("section registration and validation without game facts"));
    Check(FKSIWorldSectionRegistry::GetOrdered().Num() == 1, TEXT("deterministic section registry"));
    FKSIWorldSectionRegistry::Unregister(TEXT("Probe"));
    {
        const FKSIWorldSectionRegistrar Registration(TEXT("Scoped"), 1,
            [](const FKSIWorldSectionContext&, const TArray<uint8>&, FString&) { return true; });
        Check(FKSIWorldSectionRegistry::Find(TEXT("Scoped")) != nullptr, TEXT("validator registration scope"));
    }
    Check(FKSIWorldSectionRegistry::GetOrdered().IsEmpty(), TEXT("owner lifetime releases validator callback"));
    struct FClock final : IKSIWorldTimeSource
    {
        virtual bool ReadWorldTime(FKSIWorldTimeView& Out) const override
        {
            Out.WorldSeconds = 42.;
            return true;
        }
    } Clock;
    FKSIWorldTimeView Time;
    Check(Clock.ReadWorldTime(Time) && Time.WorldSeconds == 42., TEXT("external clock read contract"));
#endif
#if WITH_KSI_INVENTORY
    Check(!FModuleManager::Get().IsModuleLoaded("CCL") && !FModuleManager::Get().IsModuleLoaded("KSIUI"), TEXT("storage without game extensions or UI"));
    auto MakeBox = [this]()
    {
        auto* Actor = GetWorld()->SpawnActor<AActor>();
        auto* Box = NewObject<UKSIInventoryComponent>(Actor);
        Actor->AddInstanceComponent(Box);
        Box->RegisterComponent();
        Box->Capacity = 2;
        return Box;
    };
    auto* A = MakeBox();
    auto* B = MakeBox();
    auto* Definition = NewObject<UKSIItemDefinition>(this, TEXT("PlainMaterial"));
    Definition->MaxStackCount = 10;
    TArray<FText> DefinitionErrors;
    Check(Definition->ValidateDefinition(DefinitionErrors), TEXT("generic item definition without registered features"));
    const FGuid Id = A->Add(Definition, 3);
    Check(Id.IsValid() && A->Add(Definition, 2) == Id && A->Find(Id)->Quantity == 5, TEXT("storage and stacking preserve identity"));
    Check(A->MoveToSlot(Id, 1) && A->FindSlot(1)->Id == Id, TEXT("slot move"));
    int32 Notifications = 0;
    bool bAtomic = true;
    bool bReentryRejected = true;
    const FDelegateHandle Listener = A->OnChanged.AddLambda([&]()
    {
        ++Notifications;
        bAtomic = bAtomic && A->Find(Id) && A->Find(Id)->Quantity == 3 && B->GetEntries().Num() == 1;
        bReentryRejected = bReentryRejected && !A->Remove(Id, 1) && !B->Add(Definition, 1).IsValid();
    });
    Check(A->TransferTo(Id, 2, *B), TEXT("two-container partial transfer"));
    A->OnChanged.Remove(Listener);
    Check(Notifications == 1 && bAtomic && bReentryRejected, TEXT("transfer observers see both commits and cannot reenter"));
    Check(B->GetEntries().Num() == 1 && B->GetEntries()[0].Id != Id && B->GetEntries()[0].Quantity == 2, TEXT("split creates new identity"));
    Check(A->TransferTo(Id, 3, *B) && !A->Find(Id) && B->Find(Id), TEXT("whole transfer preserves identity"));
    const FGuid Extra = A->Add(Definition, 1);
    Check(!A->TransferTo(Extra, 1, *B) && A->Find(Extra)->Quantity == 1, TEXT("full destination rejects atomically"));
    TArray<uint8> Saved;
    Check(B->Capture(Saved), TEXT("capture bounded storage bytes"));
    const FString Path = FPaths::ProjectSavedDir() / TEXT("plain-inventory.bin");
    Check(FFileHelper::SaveArrayToFile(Saved, *Path), TEXT("write inventory snapshot"));
    TArray<uint8> Read;
    Check(FFileHelper::LoadFileToArray(Read, *Path) && Read == Saved, TEXT("read identical snapshot bytes"));
    auto Resolve = [Definition](const FString& Name) -> UKSIItemDefinition*
    {
        return Name == Definition->GetPrimaryAssetId().ToString() ? Definition : nullptr;
    };
    Check(B->Restore({}) && B->RestoreSnapshot(Read, Resolve) && B->Find(Id) && B->Find(Id)->Quantity == 3, TEXT("restore identity and quantity through explicit catalog resolver"));
    bool bResolverLocked = true;
    Check(B->RestoreSnapshot(Read, [&](const FString& Name) -> UKSIItemDefinition*
    {
        bResolverLocked = bResolverLocked && !B->Remove(Id, 1);
        return Resolve(Name);
    }) && bResolverLocked, TEXT("catalog callback cannot mutate storage during restore"));
    TArray<uint8> Corrupt = Read;
    Corrupt.Add(99);
    Check(!B->RestoreSnapshot(Corrupt, Resolve) && B->Find(Id)->Quantity == 3, TEXT("trailing data rejected without mutation"));
    Check(!B->RestoreSnapshot(Read, [](const FString&) -> UKSIItemDefinition* { return nullptr; }), TEXT("unknown definition rejected"));
    for (int32 Length = 0; Length < Read.Num(); ++Length)
    {
        Corrupt = Read;
        Corrupt.SetNum(Length);
        if (B->RestoreSnapshot(Corrupt, Resolve))
        {
            Check(false, TEXT("truncated snapshot accepted"));
            break;
        }
    }
    TArray<uint8> After;
    Check(B->Capture(After) && After == Saved, TEXT("rejected saves preserve byte-exact state"));
    auto* FirstDefinition = NewObject<UKSIItemDefinition>(A, TEXT("DuplicateId"));
    auto* SecondDefinition = NewObject<UKSIItemDefinition>(B, TEXT("DuplicateId"));
    Check(A->Restore({}) && A->Add(FirstDefinition, 1).IsValid() && A->Add(SecondDefinition, 1).IsValid(), TEXT("distinct definitions can be held in separate slots"));
    Check(!A->Capture(After), TEXT("ambiguous primary asset identities rejected before save"));
    A->GetOwner()->Destroy();
    B->GetOwner()->Destroy();
#endif
#if WITH_KSI_COMBAT
    bPassed = RunKSICombatProbe(GetWorld()) && bPassed;
#endif
#if WITH_KSI_WORLD
    bPassed = RunKSIWorldProbe() && bPassed;
#endif
    bFinished = 1;
    UE_LOG(LogTemp, Display, TEXT("KSI_PROBE %s"), bPassed ? TEXT("PASS") : TEXT("FAIL"));
    FPlatformMisc::RequestExitWithStatus(false, bPassed ? 0 : 1);
}
