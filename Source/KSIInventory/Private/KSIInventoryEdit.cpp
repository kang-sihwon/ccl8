#include "KSIInventoryEdit.h"

FKSIInventoryEdit::FKSIInventoryEdit(UKSIInventoryComponent& InInventory)
	: Inventory(&InInventory)
{
	if (!InInventory.CanMutate())
	{
		return;
	}

	InInventory.bEditOpen = 1;
	Staged = InInventory.List.Entries;
	bOpen = 1;
}

FKSIInventoryEdit::~FKSIInventoryEdit()
{
	if (!bOpen)
	{
		return;
	}

	Publish();
	if (UKSIInventoryComponent* Target = Inventory.Get())
	{
		Target->bEditOpen = 0;
	}
}

bool FKSIInventoryEdit::Add(UKSIItemDefinition* Definition, int32 Quantity, FGuid& OutId)
{
	return CanStage() && Inventory->StageAdd(Staged, Definition, Quantity, OutId);
}

bool FKSIInventoryEdit::Remove(FGuid Id, int32 Quantity)
{
	return CanStage() && Inventory->StageRemove(Staged, Id, Quantity);
}

bool FKSIInventoryEdit::Place(FGuid Id, int32 Slot)
{
	if (!CanStage() || Slot < INDEX_NONE || Slot >= Inventory->GetSlotCount())
	{
		return false;
	}

	FKSIInventoryEntry* Source = Find(Id);
	if (!Source)
	{
		return false;
	}

	if (Source->Slot == Slot)
	{
		return true;
	}

	if (Slot != INDEX_NONE)
	{
		FKSIInventoryEntry* Occupant = Staged.FindByPredicate([Slot](const FKSIInventoryEntry& Entry)
															  { return Entry.Slot == Slot; });
		if (Occupant)
		{
			if (Source->Slot == INDEX_NONE)
			{
				return false;
			}

			Occupant->Slot = Source->Slot;
		}
	}

	Source->Slot = Slot;
	return true;
}

void FKSIInventoryEdit::Replace(TArray<FKSIInventoryEntry> Entries)
{
	if (CanStage())
	{
		Staged = MoveTemp(Entries);
	}
}

bool FKSIInventoryEdit::Validate() const
{
	const UKSIInventoryComponent* Target = Inventory.Get();
	return bOpen && !bCommitted && Target && Target->CanRestore(Staged);
}

bool FKSIInventoryEdit::Commit()
{
	if (!Validate())
	{
		return false;
	}

	Inventory->ApplyEdit(Staged);
	bCommitted = 1;
	return true;
}

void FKSIInventoryEdit::Publish()
{
	UKSIInventoryComponent* Target = Inventory.Get();
	if (!bCommitted || bPublished || !Target)
	{
		return;
	}

	bPublished = 1;
	Target->OnChanged.Broadcast();
}

FKSIInventoryEntry* FKSIInventoryEdit::Find(FGuid Id)
{
	return Staged.FindByPredicate([Id](const FKSIInventoryEntry& Entry)
								  { return Entry.Id == Id; });
}

int32 FKSIInventoryEdit::FindFreeSlot() const
{
	const UKSIInventoryComponent* Target = Inventory.Get();
	return Target ? FindFreeSlot(Staged, Target->GetSlotCount()) : INDEX_NONE;
}

int32 FKSIInventoryEdit::FindFreeSlot(const TArray<FKSIInventoryEntry>& Entries, int32 SlotCount)
{
	for (int32 Slot = 0; Slot < SlotCount; ++Slot)
	{
		const bool bUsed = Entries.ContainsByPredicate([Slot](const FKSIInventoryEntry& Entry)
													   { return Entry.Slot == Slot; });
		if (!bUsed)
		{
			return Slot;
		}
	}
	return INDEX_NONE;
}

FKSIInventoryEdit* FKSIInventoryEdit::FindIn(TArrayView<const TUniquePtr<FKSIInventoryEdit>> Edits, const UKSIInventoryComponent* Inventory)
{
	for (const TUniquePtr<FKSIInventoryEdit>& Edit : Edits)
	{
		if (Edit && Edit->GetInventory() == Inventory)
		{
			return Edit.Get();
		}
	}
	return nullptr;
}

void FKSIInventoryEdit::PublishAndRelease(TArray<TUniquePtr<FKSIInventoryEdit>>& Edits)
{
	TArray<TUniquePtr<FKSIInventoryEdit>> Notifications = MoveTemp(Edits);
	for (TUniquePtr<FKSIInventoryEdit>& Edit : Notifications)
	{
		Edit->Publish();
		Edit.Reset();
	}
}

bool FKSIInventoryEdit::CanStage() const
{
	return bOpen && !bCommitted && Inventory.IsValid();
}
