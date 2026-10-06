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
		if (Entry.Definition == Definition && Quantity <= Definition->GetMaxStack() - Entry.Quantity)
		{
			Entry.Quantity += Quantity;
			List.MarkItemDirty(Entry);
			const FGuid Id = Entry.Id;
			OnChanged.Broadcast();
			return Id;
		}
	}
	auto& Entry = List.Entries.AddDefaulted_GetRef();
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
		if (Entry.Definition == Definition && Quantity <= Definition->GetMaxStack() - Entry.Quantity)
		{
			return true;
		}
	}
	return List.Entries.Num() < FMath::Clamp(Capacity, 1, 128);
}

const FCCLInventoryEntry* UCCLInventoryComponent::Find(FGuid Id) const
{
	return List.Entries.FindByPredicate([Id](const FCCLInventoryEntry& Entry) { return Entry.Id == Id; });
}
