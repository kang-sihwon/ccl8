#include "CCLInventoryPersistenceSubsystem.h"

#include "CCLItemDefinition.h"
#include "CCLWorldPickup.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Engine/World.h"
#include "UObject/UObjectIterator.h"

namespace
{
bool Reject(FString& Error, const TCHAR* Reason)
{
    Error = Reason;
    return false;
}

ACCLWorldPickup* PickupOwner(const UCCLInventoryComponent& Inventory)
{
    auto* Pickup = Cast<ACCLWorldPickup>(Inventory.GetOwner());
    return Pickup && Pickup->GetFoodInventory() == &Inventory ? Pickup : nullptr;
}

bool CaptureInventory(const UCCLInventoryComponent& Inventory, FCCLInventorySnapshot& Record, FString& Error)
{
    if (Inventory.IsTransactionOpen() || !Inventory.CanRestore(Inventory.GetEntries()))
    {
        return Reject(Error, TEXT("Inventory is busy or invalid."));
    }
    Record.ContainerId = Inventory.GetPersistenceId();
    Record.Capacity = Inventory.Capacity;
    Record.Environment = Inventory.GetFoodEnvironment();
    if (const auto* Pickup = PickupOwner(Inventory))
    {
        Record.Owner = ECCLInventoryOwner::FoodPickup;
        Record.Transform = Pickup->GetActorTransform();
    }
    for (const auto& Entry : Inventory.GetEntries())
    {
        if (Entry.Slot < 0)
        {
            return Reject(Error, TEXT("Equipped inventory requires a loadout restore participant."));
        }
        auto& Item = Record.Items.AddDefaulted_GetRef();
        Item.Id = Entry.Id;
        Item.Definition = Entry.Definition->GetPathName();
        Item.Quantity = Entry.Quantity;
        Item.Slot = Entry.Slot;
        Item.LoadedAmmo = Entry.LoadedAmmo;
        if (!CCLFood::Encode(Entry.Food, Item.Food, Error))
        {
            return false;
        }
    }
    return true;
}

bool Resolve(const FCCLInventorySnapshot& Record, TArray<FCCLInventoryEntry>& Entries, FString& Error)
{
    for (const auto& Item : Record.Items)
    {
        auto& Entry = Entries.AddDefaulted_GetRef();
        Entry.Id = Item.Id;
        Entry.Definition = LoadObject<UCCLItemDefinition>(nullptr, *Item.Definition);
        Entry.Quantity = Item.Quantity;
        Entry.Slot = Item.Slot;
        Entry.LoadedAmmo = Item.LoadedAmmo;
        if (Entry.Slot < 0 || !CCLFood::Decode(Item.Food, Entry.Food, Error))
        {
            return Reject(Error, TEXT("Equipped or invalid stored inventory."));
        }
    }
    return UCCLInventoryComponent::ValidateEntries(Entries, Record.Capacity) ||
           Reject(Error, TEXT("Stored item definition or capacity is incompatible."));
}
} // namespace

bool UCCLInventoryPersistenceSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const auto* World = Cast<UWorld>(Outer);
    return World && World->IsGameWorld();
}

bool UCCLInventoryPersistenceSubsystem::Bind(UCCLInventoryComponent& Inventory, FGuid Id, FString& Error)
{
    const auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
    if (bPreparing || !Id.IsValid() || Inventory.GetWorld() != GetWorld() || !Inventory.GetOwner()->HasAuthority() ||
        Inventory.IsTransactionOpen() || !Runtime || !Runtime->IsRunning() ||
        (Inventory.PersistenceId.IsValid() && Inventory.PersistenceId != Id))
    {
        return Reject(Error, TEXT("Cannot bind this inventory to a persistent owner."));
    }
    if (const auto* Existing = Live.Find(Id); Existing && Existing->IsValid())
    {
        return Existing->Get() == &Inventory || Reject(Error, TEXT("Persistent container already has a live owner."));
    }
    TArray<FCCLInventoryEntry> Entries;
    if (const auto* Stored = Records.Find(Id))
    {
        if ((Stored->Owner == ECCLInventoryOwner::FoodPickup) != (PickupOwner(Inventory) != nullptr) ||
            Stored->Capacity != Inventory.Capacity || !Resolve(*Stored, Entries, Error) ||
            !Inventory.CanRestore(Entries))
        {
            return Reject(Error, TEXT("Persistent container does not match its loaded component."));
        }
    }
    else
    {
        FCCLInventorySnapshot Record;
        if (!CaptureInventory(Inventory, Record, Error))
        {
            return false;
        }
        Record.ContainerId = Id;
        if (!CCLInventorySnapshot::Validate({Record}, Runtime->GetIdentity().WorldId,
                                            Runtime->GetClock().GetWorldSeconds(), Error))
        {
            return false;
        }
        Entries = Inventory.GetEntries();
    }
    Inventory.FoodEnvironment = Records.Contains(Id) ? Records[Id].Environment : Inventory.GetFoodEnvironment();
    Inventory.PersistenceId = Id;
    Live.Add(Id, &Inventory);
    // Identity is visible before the first notification; validation above makes Restore infallible here.
    verify(Inventory.Restore(Entries));
    return true;
}

bool UCCLInventoryPersistenceSubsystem::BindPickup(ACCLWorldPickup& Pickup, FString& Error, bool bCreatingEmpty)
{
    if (bPreparing || !Pickup.HasAuthority() || Pickup.GetWorld() != GetWorld() ||
        (!Pickup.PersistentId.IsValid() && Pickup.HasAnyFlags(RF_WasLoaded)))
    {
        return Reject(Error,
                      TEXT("Food pickup needs an idle world and an authored persistent ID when loaded from a map."));
    }
    const FGuid Id = Pickup.PersistentId.IsValid() ? Pickup.PersistentId : FGuid::NewGuid();
    const bool bWasStored = Records.Contains(Id);
    if (Pickup.HasAnyFlags(RF_WasLoaded) && bWasStored && Records[Id].Owner == ECCLInventoryOwner::FoodPickup &&
        Live.FindRef(Id).IsValid() && Live.FindRef(Id).Get() != Pickup.FoodInventory)
    {
        verify(Pickup.FoodInventory->Restore({}));
        Pickup.bCollected = 1;
        Pickup.Destroy();
        return true;
    }
    if (!Bind(*Pickup.FoodInventory, Id, Error))
    {
        return false;
    }
    Pickup.PersistentId = Id;
    if (const auto* Entry = Pickup.FoodInventory->FindSlot(0))
    {
        Pickup.Definition = Entry->Definition;
        Pickup.Quantity = Entry->Quantity;
        Pickup.bFoodInitialized = 1;
        if (bWasStored)
        {
            Pickup.SetActorTransform(Records[Id].Transform, false, nullptr, ETeleportType::TeleportPhysics);
        }
    }
    else if (!bCreatingEmpty || bWasStored)
    {
        Pickup.Quantity = 0;
        Pickup.bCollected = 1;
        Pickup.Destroy();
    }
    return true;
}

void UCCLInventoryPersistenceSubsystem::CancelEmptyPickup(ACCLWorldPickup& Pickup)
{
    if (!bPreparing && Pickup.FoodInventory->GetEntries().IsEmpty() &&
        Live.FindRef(Pickup.PersistentId).Get() == Pickup.FoodInventory)
    {
        Live.Remove(Pickup.PersistentId);
        Records.Remove(Pickup.PersistentId);
        Pickup.FoodInventory->PersistenceId.Invalidate();
        Pickup.PersistentId.Invalidate();
    }
}

void UCCLInventoryPersistenceSubsystem::Unbind(UCCLInventoryComponent& Inventory)
{
    const FGuid Id = Inventory.PersistenceId;
    if (Live.FindRef(Id).Get() != &Inventory)
    {
        return;
    }
    FCCLInventorySnapshot Record;
    FString Error;
    if (CaptureInventory(Inventory, Record, Error))
    {
        Records.Add(Id, MoveTemp(Record));
    }
    else
    {
        bCaptureFailed = 1;
        UE_LOG(LogTemp, Error, TEXT("CCL_INVENTORY unload capture failed: %s"), *Error);
    }
    Live.Remove(Id);
    Inventory.PersistenceId.Invalidate();
}

bool UCCLInventoryPersistenceSubsystem::CheckUnboundFood(FString& Error,
                                                         const UCCLInventoryComponent* SessionInventory,
    TConstArrayView<UCCLInventoryComponent*> PlayerInventories) const
{
    for (TObjectIterator<UCCLInventoryComponent> It; It; ++It)
    {
        if (It->GetWorld() != GetWorld() || !IsValid(*It) || !IsValid(It->GetOwner()) ||
            !It->GetOwner()->HasAuthority())
        {
            continue;
        }
        for (const auto& Entry : It->GetEntries())
        {
            if (*It != SessionInventory && !PlayerInventories.Contains(*It) && Entry.Food.IsPresent() && Live.FindRef(It->GetPersistenceId()).Get() != *It)
            {
                return Reject(Error, TEXT("Food inventory needs an explicit persistent container identity before world "
                                          "save or restore."));
            }
        }
    }
    return true;
}

bool UCCLInventoryPersistenceSubsystem::Capture(FGuid WorldId, double WorldSeconds,
                                                TArray<FCCLInventorySnapshot>& Output, FString& Error,
                                                const UCCLInventoryComponent* SessionInventory,
    TConstArrayView<UCCLInventoryComponent*> PlayerInventories) const
{
    if (SessionInventory && (!IsValid(SessionInventory) || SessionInventory->GetWorld() != GetWorld() ||
                             SessionInventory->GetPersistenceId().IsValid() || SessionInventory->IsTransactionOpen() ||
                             !SessionInventory->CanRestore(SessionInventory->GetEntries())))
    {
        return Reject(Error, TEXT("Session inventory must be valid, unbound and captured by its session owner."));
    }
    if (bPreparing || bCaptureFailed || GetWorld()->GetNetMode() == NM_Client ||
        !CheckUnboundFood(Error, SessionInventory, PlayerInventories))
    {
        return Reject(Error, Error.IsEmpty() ? TEXT("Inventory capture is unavailable.") : *FString(Error));
    }
    auto Candidate = Records;
    for (const auto& Pair : Live)
    {
        const auto* Inventory = Pair.Value.Get();
        FCCLInventorySnapshot Record;
        if (!Inventory || !CaptureInventory(*Inventory, Record, Error))
        {
            return Reject(Error, TEXT("Registered inventory is missing or busy."));
        }
        Candidate.Add(Pair.Key, MoveTemp(Record));
    }
    TArray<FCCLInventorySnapshot> Values;
    Candidate.GenerateValueArray(Values);
    Values.Sort([](const auto& A, const auto& B) { return A.ContainerId < B.ContainerId; });
    if (!CCLInventorySnapshot::Validate(Values, WorldId, WorldSeconds, Error))
    {
        return false;
    }
    Output = MoveTemp(Values);
    return true;
}

bool UCCLInventoryPersistenceSubsystem::PrepareRestore(const TArray<FCCLInventorySnapshot>& Input, FGuid WorldId,
                                                       double WorldSeconds, FString& Error,
    TConstArrayView<UCCLInventoryComponent*> PlayerInventories)
{
    if (bPreparing || GetWorld()->GetNetMode() == NM_Client || !CheckUnboundFood(Error, nullptr, PlayerInventories) ||
        !CCLInventorySnapshot::Validate(Input, WorldId, WorldSeconds, Error))
    {
        return false;
    }
    TMap<FGuid, TArray<FCCLInventoryEntry>> Resolved;
    for (const auto& Record : Input)
    {
        if (!Resolve(Record, Resolved.FindOrAdd(Record.ContainerId), Error))
        {
            return false;
        }
        if (Record.Owner == ECCLInventoryOwner::FoodPickup && !Record.Items.IsEmpty() &&
            !Resolved[Record.ContainerId][0].Definition->FindFragment<FCCLItemFragment_Perishable>())
        {
            return Reject(Error, TEXT("Stored pickup must resolve to a perishable definition."));
        }
    }
    for (const auto& Pair : Live)
    {
        auto* Inventory = Pair.Value.Get();
        const auto* Record = Input.FindByPredicate([&](const auto& R) { return R.ContainerId == Pair.Key; });
        if (!Inventory || Inventory->IsTransactionOpen() ||
            (Record && (Record->Owner == ECCLInventoryOwner::FoodPickup) != (PickupOwner(*Inventory) != nullptr)))
        {
            return Reject(Error, TEXT("Live inventory owner is missing, busy or incompatible."));
        }
        if (!PickupOwner(*Inventory) &&
            (!Inventory->CanRestore(Resolved.FindOrAdd(Pair.Key)) ||
             (Record && Inventory->Capacity != Record->Capacity) ||
             Inventory->GetEntries().ContainsByPredicate([](const auto& E) { return E.Slot < 0; })))
        {
            return Reject(Error, TEXT("Live inventory is incompatible with world restore."));
        }
    }
    bPreparing = 1;
    for (const auto& Record : Input)
    {
        Prepared.Add(Record.ContainerId, Record);
        if (Record.Owner != ECCLInventoryOwner::FoodPickup || Record.Items.IsEmpty())
        {
            continue;
        }
        auto* Pickup = GetWorld()->SpawnActorDeferred<ACCLWorldPickup>(
            ACCLWorldPickup::StaticClass(), Record.Transform, nullptr, nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn, ESpawnActorScaleMethod::OverrideRootScale);
        if (!Pickup)
        {
            CancelRestore();
            return Reject(Error, TEXT("Could not prepare a restored food pickup."));
        }
        Pickup->SetReplicates(false);
        Pickup->SetActorHiddenInGame(true);
        Pickup->SetActorTickEnabled(false);
        Pickup->bCollected = 1;
        Pickup->PersistentId = Record.ContainerId;
        SpawnedPickups.Add(Pickup);
        Pending.Add(Pickup->FoodInventory, Resolved[Record.ContainerId]);
    }
    for (const auto& Pair : Live)
    {
        auto* Inventory = Pair.Value.Get();
        if (auto* Pickup = PickupOwner(*Inventory))
        {
            RetiredPickups.Add(Pickup);
        }
        else
        {
            Pending.Add(Inventory, MoveTemp(Resolved.FindOrAdd(Pair.Key)));
        }
        if (!Inventory->BeginTransaction())
        {
            CancelRestore();
            return Reject(Error, TEXT("Inventory transaction could not be acquired."));
        }
        Locked.Add(Inventory);
    }
    for (const auto& Pickup : SpawnedPickups)
    {
        if (!Pickup->FoodInventory->BeginTransaction())
        {
            CancelRestore();
            return Reject(Error, TEXT("Prepared pickup transaction could not be acquired."));
        }
        Locked.Add(Pickup->FoodInventory);
    }
    return true;
}

void UCCLInventoryPersistenceSubsystem::CancelRestore()
{
    for (const auto& Inventory : Locked)
    {
        if (Inventory.IsValid())
        {
            Inventory->EndTransaction(false);
        }
    }
    Pending.Reset();
    Prepared.Reset();
    Locked.Reset();
    for (const auto& Pickup : SpawnedPickups)
    {
        if (IsValid(Pickup))
        {
            Pickup->Destroy();
        }
    }
    SpawnedPickups.Reset();
    RetiredPickups.Reset();
    bPreparing = 0;
}

void UCCLInventoryPersistenceSubsystem::CommitRestore(bool bPublish)
{
    check(bPreparing);
    Records = MoveTemp(Prepared);
    for (const auto& Old : RetiredPickups)
    {
        if (!Old.IsValid())
        {
            continue;
        }
        Live.Remove(Old->PersistentId);
        Old->FoodInventory->PersistenceId.Invalidate();
        Old->bCollected = 1;
        Old->Destroy();
    }
    RetiredPickups.Reset();
    for (const auto& Pickup : SpawnedPickups)
    {
        Pickup->FoodInventory->PersistenceId = Pickup->PersistentId;
        Live.Add(Pickup->PersistentId, Pickup->FoodInventory);
    }
    for (auto& Pair : Pending)
    {
        check(Pair.Key.IsValid());
        const auto* Record = Records.Find(Pair.Key->GetPersistenceId());
        Pair.Key->FoodEnvironment = Record ? Record->Environment : FCCLFoodEnvironmentBinding();
        verify(Pair.Key->Restore(Pair.Value));
    }
    Pending.Reset();
    for (const auto& Pickup : SpawnedPickups)
    {
        const auto* Entry = Pickup->FoodInventory->FindSlot(0);
        check(Entry);
        Pickup->Definition = Entry->Definition;
        Pickup->Quantity = Entry->Quantity;
        Pickup->bFoodInitialized = 1;
        Pickup->bCollected = 0;
        Pickup->SetActorHiddenInGame(false);
        Pickup->SetReplicates(true);
        Pickup->FinishSpawning(Records[Pickup->PersistentId].Transform);
        Pickup->UpdateFoodView();
        Pickup->ForceNetUpdate();
    }
    SpawnedPickups.Reset();
    bCaptureFailed = 0;
    if (bPublish) { PublishRestore(); }
}

void UCCLInventoryPersistenceSubsystem::PublishRestore()
{
    // The world coordinator publishes only after every owner has committed its state.
    auto Notifications = MoveTemp(Locked);
    for (const auto& Inventory : Notifications)
    {
        if (Inventory.IsValid())
        {
            Inventory->EndTransaction(true);
        }
    }
    bPreparing = 0;
}

bool UCCLInventoryPersistenceSubsystem::HasEnvironmentBindings() const
{
    for (const auto& Pair : Live)
    {
        if (Pair.Value.IsValid() && (Pair.Value->GetFoodEnvironment().IsDynamic() || PickupOwner(*Pair.Value)))
        {
            return true;
        }
    }
    for (const auto& Pair : Records)
    {
        if (!Live.FindRef(Pair.Key).IsValid() &&
            (Pair.Value.Environment.IsDynamic() ||
             (Pair.Value.Owner == ECCLInventoryOwner::FoodPickup && !Pair.Value.Items.IsEmpty())))
        {
            return true;
        }
    }
    return false;
}

bool UCCLInventoryPersistenceSubsystem::PrepareAdvance(const FCCLWorldStep& Step, const FCCLSurfaceSimulation& Surface,
                                                       FString& Error)
{
    if (bPreparing || bCaptureFailed || GetWorld()->GetNetMode() == NM_Client ||
        Step.WorldToSeconds != Surface.GetWorldSeconds())
    {
        return Reject(Error, TEXT("Inventory environment advance is unavailable."));
    }
    if (Step.WorldToSeconds == Step.WorldFromSeconds)
    {
        bPreparing = 1;
        Prepared = Records;
        return true;
    }
    auto Candidate = Records;
    TMap<TWeakObjectPtr<UCCLInventoryComponent>, TArray<FCCLInventoryEntry>> Entries;
    for (const auto& Pair : Live)
    {
        auto* Inventory = Pair.Value.Get();
        if (!Inventory)
        {
            return Reject(Error, TEXT("Registered environment owner is missing."));
        }
        // A live owner's current policy supersedes its last unloaded record.
        if (!Inventory->GetFoodEnvironment().IsDynamic() && !PickupOwner(*Inventory))
        {
            Candidate.Remove(Pair.Key);
            continue;
        }
        FCCLInventorySnapshot Record;
        if (!CaptureInventory(*Inventory, Record, Error))
        {
            return false;
        }
        Candidate.Add(Pair.Key, MoveTemp(Record));
    }
    for (auto& Pair : Candidate)
    {
        const bool bPickup = Pair.Value.Owner == ECCLInventoryOwner::FoodPickup && !Pair.Value.Items.IsEmpty();
        if (bPickup &&
            !CCLFoodEnvironment::RelocatePickup(
                Pair.Value, GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>()->GetSurfaceSimulation(), Error))
        {
            return false;
        }
        if (!Pair.Value.Environment.IsDynamic() && !bPickup)
        {
            continue;
        }
        if (!CCLFoodEnvironment::Advance(Pair.Value, Surface, Step.WorldFromSeconds, Error))
        {
            return false;
        }
        if (auto* Inventory = Live.FindRef(Pair.Key).Get())
        {
            auto& Resolved = Entries.FindOrAdd(Inventory);
            if (!Resolve(Pair.Value, Resolved, Error) || !Inventory->CanRestore(Resolved))
            {
                return false;
            }
        }
    }
    TArray<FCCLInventorySnapshot> Check;
    Candidate.GenerateValueArray(Check);
    if (!CCLInventorySnapshot::Validate(Check, Surface.GetWorldId(), Step.WorldToSeconds, Error))
    {
        return false;
    }
    bPreparing = 1;
    Prepared = MoveTemp(Candidate);
    Pending = MoveTemp(Entries);
    for (const auto& Pair : Pending)
    {
        if (!Pair.Key.IsValid() || !Pair.Key->BeginTransaction())
        {
            CancelRestore();
            return Reject(Error, TEXT("Food environment inventory could not be locked."));
        }
        Locked.Add(Pair.Key);
    }
    return true;
}
