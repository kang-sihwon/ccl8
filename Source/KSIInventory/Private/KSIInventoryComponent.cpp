#include "KSIInventoryComponent.h"

#include "KSIInventoryEdit.h"
#include "KSIItemDefinition.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

namespace
{
	bool HasSameContent(const FKSIInventoryEntry& A, const FKSIInventoryEntry& B)
	{
		return A.Id == B.Id && A.Definition == B.Definition && A.Quantity == B.Quantity && A.Slot == B.Slot && A.State == B.State;
	}

	// 복제 ID를 건드리지 않고 내용만 옮긴다. FFastArraySerializerItem의 복사는 복제 ID를 초기화한다.
	void CopyContent(FKSIInventoryEntry& Target, const FKSIInventoryEntry& Source)
	{
		Target.Id = Source.Id;
		Target.Definition = Source.Definition;
		Target.Quantity = Source.Quantity;
		Target.Slot = Source.Slot;
		Target.State = Source.State;
	}
} // namespace

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
	FKSIInventoryEdit Edit(*this);
	FGuid Id;
	if (!Edit.Add(Definition, Quantity, Id) || !Edit.Commit())
	{
		return FGuid();
	}
	return Id;
}

bool UKSIInventoryComponent::Remove(FGuid Id, int32 Quantity)
{
	FKSIInventoryEdit Edit(*this);
	return Edit.Remove(Id, Quantity) && Edit.Commit();
}

bool UKSIInventoryComponent::MoveToSlot(FGuid Id, int32 Slot)
{
	const FKSIInventoryEntry* Source = Find(Id);
	if (!CanMutate() || !Source || Source->Slot < 0 || Slot < 0 || Slot >= GetSlotCount())
	{
		return false;
	}

	if (Source->Slot == Slot)
	{
		return true;
	}

	FKSIInventoryEdit Edit(*this);
	return Edit.Place(Id, Slot) && Edit.Commit();
}

bool UKSIInventoryComponent::Restore(const TArray<FKSIInventoryEntry>& Entries)
{
	// Entries가 이 컴포넌트의 GetEntries()일 수 있으므로 Replace가 먼저 복사한다.
	FKSIInventoryEdit Edit(*this);
	Edit.Replace(Entries);
	return Edit.Commit();
}

bool UKSIInventoryComponent::CanAdd(const UKSIItemDefinition* Definition, int32 Quantity) const
{
	if (!Definition || Quantity <= 0 || Quantity > Definition->GetMaxStack())
	{
		return false;
	}

	for (const FKSIInventoryEntry& Entry : List.Entries)
	{
		if (Entry.Slot >= 0 && Entry.Definition == Definition && Quantity <= Definition->GetMaxStack() - Entry.Quantity && CanStackEntry(Entry))
		{
			return true;
		}
	}
	return FKSIInventoryEdit::FindFreeSlot(List.Entries, GetSlotCount()) != INDEX_NONE;
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
	for (const FKSIInventoryEntry& Entry : Entries)
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

bool UKSIInventoryComponent::CanMutate() const
{
	return GetOwner() && GetOwner()->HasAuthority() && !bEditOpen;
}

const FKSIInventoryEntry* UKSIInventoryComponent::Find(FGuid Id) const
{
	return List.Entries.FindByPredicate([Id](const FKSIInventoryEntry& Entry)
										{ return Entry.Id == Id; });
}

const FKSIInventoryEntry* UKSIInventoryComponent::FindSlot(int32 Slot) const
{
	if (Slot < 0 || Slot >= GetSlotCount())
	{
		return nullptr;
	}

	return List.Entries.FindByPredicate([Slot](const FKSIInventoryEntry& Entry)
										{ return Entry.Slot == Slot; });
}

bool UKSIInventoryComponent::StageAdd(TArray<FKSIInventoryEntry>& Entries, UKSIItemDefinition* Definition, int32 Quantity, FGuid& OutId) const
{
	if (!Definition || Quantity <= 0 || Quantity > Definition->GetMaxStack())
	{
		return false;
	}

	for (FKSIInventoryEntry& Entry : Entries)
	{
		if (Entry.Slot >= 0 && Entry.Definition == Definition && CanStackEntry(Entry) && Quantity <= Definition->GetMaxStack() - Entry.Quantity)
		{
			Entry.Quantity += Quantity;
			OutId = Entry.Id;
			return true;
		}
	}

	const int32 FreeSlot = FKSIInventoryEdit::FindFreeSlot(Entries, GetSlotCount());
	if (FreeSlot == INDEX_NONE)
	{
		return false;
	}

	FKSIInventoryEntry& Entry = Entries.AddDefaulted_GetRef();
	Entry.Slot = FreeSlot;
	Entry.Id = FGuid::NewGuid();
	Entry.Definition = Definition;
	Entry.Quantity = Quantity;
	OutId = Entry.Id;
	return true;
}

bool UKSIInventoryComponent::StageRemove(TArray<FKSIInventoryEntry>& Entries, FGuid Id, int32 Quantity) const
{
	if (!Id.IsValid() || Quantity <= 0)
	{
		return false;
	}

	const int32 Index = Entries.IndexOfByPredicate([Id](const FKSIInventoryEntry& Entry)
												   { return Entry.Id == Id; });
	if (Index == INDEX_NONE || Entries[Index].Quantity < Quantity || Entries[Index].State.IsValid())
	{
		return false;
	}

	Entries[Index].Quantity -= Quantity;
	if (Entries[Index].Quantity == 0)
	{
		Entries.RemoveAt(Index);
	}
	return true;
}

bool UKSIInventoryComponent::CanStackEntry(const FKSIInventoryEntry& Entry) const
{
	return !Entry.State.IsValid();
}

void UKSIInventoryComponent::OnRep_List()
{
	OnChanged.Broadcast();
}

void UKSIInventoryComponent::ApplyEdit(const TArray<FKSIInventoryEntry>& Entries)
{
	TMap<FGuid, int32> Order;
	Order.Reserve(Entries.Num());
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		Order.Add(Entries[Index].Id, Index);
	}

	// TArray는 원소를 비트 단위로 옮기므로 제거와 정렬 뒤에도 각 항목의 복제 ID가 유지된다.
	const int32 Removed = List.Entries.RemoveAll([&Order](const FKSIInventoryEntry& Entry)
												 { return !Order.Contains(Entry.Id); });

	for (const FKSIInventoryEntry& Value : Entries)
	{
		FKSIInventoryEntry* Existing = List.Entries.FindByPredicate([&Value](const FKSIInventoryEntry& Entry)
																	{ return Entry.Id == Value.Id; });
		if (Existing && HasSameContent(*Existing, Value))
		{
			continue;
		}

		FKSIInventoryEntry& Target = Existing ? *Existing : List.Entries.AddDefaulted_GetRef();
		CopyContent(Target, Value);
		List.MarkItemDirty(Target);
	}

	List.Entries.Sort([&Order](const FKSIInventoryEntry& A, const FKSIInventoryEntry& B)
					  { return Order[A.Id] < Order[B.Id]; });
	if (Removed > 0)
	{
		List.MarkArrayDirty();
	}
}
