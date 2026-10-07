#include "CCLInventoryComponent.h"

#include "CCLItemDefinition.h"
#include "Net/UnrealNetwork.h"

UCCLInventoryComponent::UCCLInventoryComponent()
{
	SetIsReplicatedByDefault(true);
}

void UCCLInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UCCLInventoryComponent, List, COND_OwnerOnly);
}

FGuid UCCLInventoryComponent::Add(UCCLItemDefinition* Definition, int32 Quantity)
{
	if (!GetOwner()->HasAuthority() || !CanAdd(Definition, Quantity))
	{
		return FGuid();
	}
	for (auto& Entry : List.Entries)
	{
		if (Entry.Slot >= 0 && Entry.Definition == Definition && Quantity <= Definition->GetMaxStack() - Entry.Quantity)
		{
			Entry.Quantity += Quantity;
			List.MarkItemDirty(Entry);
			const FGuid Id = Entry.Id;
			OnChanged.Broadcast();
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
	OnChanged.Broadcast();
	return Id;
}

bool UCCLInventoryComponent::Remove(FGuid Id, int32 Quantity)
{
	if (!GetOwner()->HasAuthority() || !Id.IsValid() || Quantity <= 0)
	{
		return false;
	}
	const int32 Index = List.Entries.IndexOfByPredicate([Id](const FCCLInventoryEntry& Entry) { return Entry.Id == Id; });
	if (Index == INDEX_NONE || List.Entries[Index].Quantity < Quantity)
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
	OnChanged.Broadcast();
	return true;
}

bool UCCLInventoryComponent::CanAdd(const UCCLItemDefinition* Definition, int32 Quantity) const
{
	if (!Definition || Quantity <= 0 || Quantity > Definition->GetMaxStack())
	{
		return false;
	}
	for (const auto& Entry : List.Entries)
	{
		if (Entry.Slot >= 0 && Entry.Definition == Definition && Quantity <= Definition->GetMaxStack() - Entry.Quantity)
		{
			return true;
		}
	}
	return List.Entries.FilterByPredicate([](const FCCLInventoryEntry& Entry) { return Entry.Slot >= 0; }).Num() <
	       FMath::Clamp(Capacity, 1, 128);
}

const FCCLInventoryEntry* UCCLInventoryComponent::Find(FGuid Id) const
{
	return List.Entries.FindByPredicate([Id](const FCCLInventoryEntry& Entry) { return Entry.Id == Id; });
}

bool UCCLInventoryComponent::Restore(const TArray<FCCLInventoryEntry>& Entries)
{
	if (!GetOwner()->HasAuthority() || Entries.Num() > FMath::Clamp(Capacity, 1, 128) + 128)
	{
		return false;
	}

	TSet<FGuid> Seen;
	TSet<int32> Slots;
	for (const auto& Entry : Entries)
	{
		if (!Entry.Id.IsValid() || Seen.Contains(Entry.Id) || !Entry.Definition || Entry.Quantity <= 0 || Entry.Quantity > Entry.Definition->GetMaxStack()) { return false; }
		if (Entry.Slot < INDEX_NONE || Entry.Slot >= FMath::Clamp(Capacity, 1, 128) || (Entry.Slot >= 0 && Slots.Contains(Entry.Slot)))
		{
			return false;
		}

		Slots.Add(Entry.Slot);
		Seen.Add(Entry.Id);
	}
	List.Entries.Reset();
	for (const auto& Value : Entries)
	{
		auto& Entry = List.Entries.AddDefaulted_GetRef();
		Entry.Id = Value.Id;
		Entry.Definition = Value.Definition;
		Entry.Quantity = Value.Quantity;
		Entry.Slot = Value.Slot;
		List.MarkItemDirty(Entry);
	}
	List.MarkArrayDirty();
	OnChanged.Broadcast();
	return true;
}

const FCCLInventoryEntry* UCCLInventoryComponent::FindSlot(int32 Slot) const
{
	if (Slot < 0 || Slot >= FMath::Clamp(Capacity, 1, 128))
	{
		return nullptr;
	}

	return List.Entries.FindByPredicate([Slot](const FCCLInventoryEntry& Entry) { return Entry.Slot == Slot; });
}

bool UCCLInventoryComponent::MoveToSlot(FGuid Id, int32 Slot)
{
	if (!GetOwner()->HasAuthority() || Slot < 0 || Slot >= FMath::Clamp(Capacity, 1, 128))
	{
		return false;
	}

	auto* Source = List.Entries.FindByPredicate([Id](const FCCLInventoryEntry& Entry) { return Entry.Id == Id; });
	if (!Source || Source->Slot < 0)
	{
		return false;
	}

	if (Source->Slot == Slot)
	{
		return true;
	}

	auto* Destination = List.Entries.FindByPredicate([Slot](const FCCLInventoryEntry& Entry) { return Entry.Slot == Slot; });
	if (Destination)
	{
		Destination->Slot = Source->Slot;
		List.MarkItemDirty(*Destination);
	}

	Source->Slot = Slot;
	List.MarkItemDirty(*Source);
	OnChanged.Broadcast();
	return true;
}
