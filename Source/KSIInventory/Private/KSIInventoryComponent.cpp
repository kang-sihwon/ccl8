#include "KSIInventoryComponent.h"

#include "KSIItemDefinition.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

UKSIInventoryComponent::UKSIInventoryComponent()
{
    SetIsReplicatedByDefault(true);
}

void UKSIInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UKSIInventoryComponent, List, COND_OwnerOnly);
}

FGuid UKSIInventoryComponent::Add(UKSIItemDefinition* Definition, int32 Quantity)
{
    if (!CanMutate() || !CanAdd(Definition, Quantity))
    {
        return FGuid();
    }

    for (auto& Entry : List.Entries)
    {
        if (Entry.Slot >= 0 && Entry.Definition == Definition && CanStackEntry(Entry) && Quantity <= Definition->GetMaxStack() - Entry.Quantity)
        {
            Entry.Quantity += Quantity;
            List.MarkItemDirty(Entry);
            const FGuid Id = Entry.Id;
            NotifyChanged();
            return Id;
        }
    }
    int32 FreeSlot = 0;
    while (FindSlot(FreeSlot))
    {
        ++FreeSlot;
    }

    auto& Entry = List.Entries.AddDefaulted_GetRef();
    Entry.Slot = FreeSlot;
    Entry.Id = FGuid::NewGuid();
    Entry.Definition = Definition;
    Entry.Quantity = Quantity;
    List.MarkItemDirty(Entry);
    const FGuid Id = Entry.Id;
    NotifyChanged();
    return Id;
}

bool UKSIInventoryComponent::Remove(FGuid Id, int32 Quantity)
{
    if (!CanMutate() || !Id.IsValid() || Quantity <= 0)
    {
        return false;
    }
    const int32 Index = List.Entries.IndexOfByPredicate([Id](const FKSIInventoryEntry& Entry) { return Entry.Id == Id; });
    if (Index == INDEX_NONE || List.Entries[Index].Quantity < Quantity || List.Entries[Index].State.IsValid())
    {
        return false;
    }

    List.Entries[Index].Quantity -= Quantity;
    if (List.Entries[Index].Quantity == 0)
    {
        List.Entries.RemoveAt(Index);
        List.MarkArrayDirty();
    }
    else
    {
        List.MarkItemDirty(List.Entries[Index]);
    }
    NotifyChanged();
    return true;
}

bool UKSIInventoryComponent::CanAdd(const UKSIItemDefinition* Definition, int32 Quantity) const
{
    if (!Definition || Quantity <= 0 || Quantity > Definition->GetMaxStack())
    {
        return false;
    }
    for (const auto& Entry : List.Entries)
    {
        if (Entry.Slot >= 0 && Entry.Definition == Definition && Quantity <= Definition->GetMaxStack() - Entry.Quantity
            && CanStackEntry(Entry))
        {
            return true;
        }
    }
    return List.Entries.FilterByPredicate([](const FKSIInventoryEntry& Entry) { return Entry.Slot >= 0; }).Num() <
           FMath::Clamp(Capacity, 1, 128);
}

const FKSIInventoryEntry* UKSIInventoryComponent::Find(FGuid Id) const
{
    return List.Entries.FindByPredicate([Id](const FKSIInventoryEntry& Entry) { return Entry.Id == Id; });
}

bool UKSIInventoryComponent::CanRestore(const TArray<FKSIInventoryEntry>& Entries) const
{
    return GetOwner() && GetOwner()->HasAuthority() && ValidateEntries(Entries, Capacity);
}

bool UKSIInventoryComponent::ValidateEntries(const TArray<FKSIInventoryEntry>& Entries, int32 Capacity)
{
    if (Capacity < 1 || Capacity > 128 || Entries.Num() > Capacity + 128)
    {
        return false;
    }

    TSet<FGuid> Seen;
    TSet<int32> Slots;
    for (const auto& Entry : Entries)
    {
        if (!Entry.Id.IsValid() || Seen.Contains(Entry.Id) || !Entry.Definition || Entry.Quantity <= 0 || Entry.Quantity > Entry.Definition->GetMaxStack())
        {
            return false;
        }

        if (Entry.Slot < INDEX_NONE || Entry.Slot >= FMath::Clamp(Capacity, 1, 128) || (Entry.Slot >= 0 && Slots.Contains(Entry.Slot)))
        {
            return false;
        }


        Slots.Add(Entry.Slot);
        Seen.Add(Entry.Id);
    }
    return true;
}

bool UKSIInventoryComponent::Restore(const TArray<FKSIInventoryEntry>& Entries)
{
    if (!CanMutate() || !CanRestore(Entries))
    {
        return false;
    }
    // Restore may receive GetEntries() from this same component.
    const TArray<FKSIInventoryEntry> Candidate = Entries;
    RestoreValidated(Candidate);
    return true;
}

void UKSIInventoryComponent::RestoreValidated(const TArray<FKSIInventoryEntry>& Entries)
{
    List.Entries.Reset();
    for (const auto& Value : Entries)
    {
        auto& Entry = List.Entries.AddDefaulted_GetRef();
        Entry.Id = Value.Id;
        Entry.Definition = Value.Definition;
        Entry.Quantity = Value.Quantity;
        Entry.Slot = Value.Slot;
        Entry.State = Value.State;
        List.MarkItemDirty(Entry);
    }
    List.MarkArrayDirty();
    NotifyChanged();
}

const FKSIInventoryEntry* UKSIInventoryComponent::FindSlot(int32 Slot) const
{
    if (Slot < 0 || Slot >= FMath::Clamp(Capacity, 1, 128))
    {
        return nullptr;
    }

    return List.Entries.FindByPredicate([Slot](const FKSIInventoryEntry& Entry) { return Entry.Slot == Slot; });
}

bool UKSIInventoryComponent::MoveToSlot(FGuid Id, int32 Slot)
{
    if (!CanMutate() || Slot < 0 || Slot >= FMath::Clamp(Capacity, 1, 128))
    {
        return false;
    }

    auto* Source = List.Entries.FindByPredicate([Id](const FKSIInventoryEntry& Entry) { return Entry.Id == Id; });
    if (!Source || Source->Slot < 0)
    {
        return false;
    }

    if (Source->Slot == Slot)
    {
        return true;
    }

    auto* Destination = List.Entries.FindByPredicate([Slot](const FKSIInventoryEntry& Entry) { return Entry.Slot == Slot; });
    if (Destination)
    {
        Destination->Slot = Source->Slot;
        List.MarkItemDirty(*Destination);
    }

    Source->Slot = Slot;
    List.MarkItemDirty(*Source);
    NotifyChanged();
    return true;
}

bool UKSIInventoryComponent::BeginTransaction()
{
    if (!CanMutate() || bTransactionOpen)
    {
        return false;
    }

    bTransactionOpen = 1;
    bPendingChanged = 0;
    return true;
}

bool UKSIInventoryComponent::CanMutate() const
{
    return GetOwner() && GetOwner()->HasAuthority() && !bMutationLocked;
}

void UKSIInventoryComponent::EndTransaction(bool bPublish)
{
    if (!bMutationLocked)
    {
        FinishTransaction(bPublish);
    }
}

void UKSIInventoryComponent::FinishTransaction(bool bPublish)
{
    const bool bNotify = bTransactionOpen && bPendingChanged && bPublish;
    bTransactionOpen = 0;
    bPendingChanged = 0;
    if (bNotify)
    {
        OnChanged.Broadcast();
    }
}

void UKSIInventoryComponent::NotifyChanged()
{
    if (bTransactionOpen)
    {
        bPendingChanged = 1;
    }
    else
    {
        OnChanged.Broadcast();
    }
}

void UKSIInventoryComponent::OnRep_List()
{
    NotifyChanged();
}

bool UKSIInventoryComponent::CanStackEntry(const FKSIInventoryEntry& Entry) const
{
    return !Entry.State.IsValid();
}
