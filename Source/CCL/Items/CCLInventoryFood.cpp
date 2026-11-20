#include "CCLInventoryFood.h"

#include "CCLInventoryComponent.h"

#include "CCLItemDefinition.h"
#include "KSIInventoryEdit.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Engine/World.h"

bool FCCLInventoryFood::FoodContext(const UCCLInventoryComponent& Inventory, FGuid& WorldId, double& WorldSeconds)
{
	const auto* Runtime = Inventory.GetWorld() ? Inventory.GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>() : nullptr;
	if (!Runtime || !Runtime->IsRunning())
	{
		return false;
	}

	FKSIWorldTimeView Time;
    if (!Runtime->ReadWorldTime(Time))
    {
        return false;
    }
    WorldId = Time.WorldId;
    WorldSeconds = Time.WorldSeconds;
	return true;
}

bool FCCLInventoryFood::BuildFoodAddition(const UCCLInventoryComponent& Inventory, TArray<FKSIInventoryEntry>& Entries, UKSIItemDefinition* Definition,
	const FCCLFoodState& SourceFood, double WorldSeconds, FGuid& AddedId)
{
	FString Error;
	auto Food = SourceFood;
	if (Inventory.GetFoodEnvironment().IsDynamic() && !CCLFood::ChangeStorage(Food, Inventory.GetFoodEnvironment().Conditions, WorldSeconds, Error))
	{
		return false;
	}
	const int32 Count = CCLFood::Quantity(Food);
	if (!Definition || !Definition->FindFragment<FCCLItemFragment_Perishable>() || !Food.IsPresent()
		|| Count < 1 || Count > Definition->GetMaxStack() || !CCLFood::Validate(Food, Count, Error))
	{
		return false;
	}

	for (auto& Entry : Entries)
	{
		if (Entry.Slot >= 0 && Entry.Definition == Definition && Count <= Definition->GetMaxStack() - Entry.Quantity)
		{
			FCCLFoodState Merged = Entry.GetState<FCCLItemState>().Food;
			if (CCLFood::Merge(Merged, Food, WorldSeconds, Error))
			{
				Entry.GetState<FCCLItemState>().Food = MoveTemp(Merged);
				Entry.Quantity += Count;
				AddedId = Entry.Id;
				return true;
			}
		}
	}

	const int32 Slot = FKSIInventoryEdit::FindFreeSlot(Entries, Inventory.GetSlotCount());
	if (Slot == INDEX_NONE)
	{
		return false;
	}

	FCCLFoodState Settled = Food;
	if (!CCLFood::Advance(Settled, WorldSeconds, Error))
	{
		return false;
	}

	auto& Entry = Entries.AddDefaulted_GetRef();
	Entry.Id = FGuid::NewGuid();
	Entry.Definition = Definition;
	Entry.Quantity = Count;
	Entry.Slot = Slot;
	Entry.GetState<FCCLItemState>().Food = MoveTemp(Settled);
	AddedId = Entry.Id;
	return true;
}

bool FCCLInventoryFood::MergeFood(UCCLInventoryComponent& Inventory, FGuid SourceId, FGuid DestinationId)
{
	FGuid WorldId;
	double WorldSeconds = 0.;
	if (SourceId == DestinationId || !FoodContext(Inventory, WorldId, WorldSeconds))
	{
		return false;
	}

	FKSIInventoryEdit Edit(Inventory);
	FKSIInventoryEntry* Source = Edit.Find(SourceId);
	FKSIInventoryEntry* Destination = Edit.Find(DestinationId);
	FString Error;
	if (!Source || !Destination || Source->Slot < 0 || Destination->Slot < 0 || Source->Definition != Destination->Definition ||
		!Destination->Definition || Source->Quantity > Destination->Definition->GetMaxStack() - Destination->Quantity ||
		Source->GetState<FCCLItemState>().Food.WorldId != WorldId ||
		!CCLFood::Merge(Destination->GetState<FCCLItemState>().Food, Source->GetState<FCCLItemState>().Food, WorldSeconds, Error))
	{
		return false;
	}

	Destination->Quantity += Source->Quantity;
	Edit.GetEntries().RemoveAll([SourceId](const FKSIInventoryEntry& Entry)
								{ return Entry.Id == SourceId; });
	return Edit.Commit();
}

FGuid FCCLInventoryFood::SplitFood(UCCLInventoryComponent& Inventory, FGuid SourceId, int32 Count, int32 Slot)
{
	FGuid WorldId;
	double WorldSeconds = 0.;
	const FKSIInventoryEntry* Original = Inventory.Find(SourceId);
	if (!Original || Original->Slot < 0 || Count >= Original->Quantity || Slot < 0 || Slot >= Inventory.GetSlotCount() ||
		Inventory.FindSlot(Slot) || !FoodContext(Inventory, WorldId, WorldSeconds) || Original->GetState<FCCLItemState>().Food.WorldId != WorldId)
	{
		return {};
	}

	FKSIInventoryEdit Edit(Inventory);
	FKSIInventoryEntry* Source = Edit.Find(SourceId);
	if (!Source)
	{
		return {};
	}

	FKSIInventoryEntry Taken;
	Taken.Id = FGuid::NewGuid();
	Taken.Definition = Source->Definition;
	Taken.Quantity = Count;
	Taken.Slot = Slot;
	FString Error;
	if (!CCLFood::Split(Source->GetState<FCCLItemState>().Food, Count, WorldSeconds, Taken.GetState<FCCLItemState>().Food, Error))
	{
		return {};
	}

	Source->Quantity -= Count;
	Edit.GetEntries().Add(Taken);
	return Edit.Commit() ? Taken.Id : FGuid();
}

bool FCCLInventoryFood::TransferFood(UCCLInventoryComponent& Inventory, FGuid SourceId, int32 Count, UCCLInventoryComponent* Destination)
{
	FGuid WorldId, TargetWorldId, AddedId;
	double WorldSeconds = 0., TargetSeconds = 0.;
	if (!Destination || Destination == &Inventory || Destination->GetWorld() != Inventory.GetWorld() ||
		!FoodContext(Inventory, WorldId, WorldSeconds) || !FoodContext(*Destination, TargetWorldId, TargetSeconds) ||
		WorldId != TargetWorldId || WorldSeconds != TargetSeconds)
	{
		return false;
	}

	FKSIInventoryEdit SourceEdit(Inventory);
	FKSIInventoryEdit TargetEdit(*Destination);
	FKSIInventoryEntry* Source = SourceEdit.Find(SourceId);
	FCCLFoodState Taken;
	FString Error;
	if (!SourceEdit.IsOpen() || !TargetEdit.IsOpen() || !Source || Source->Slot < 0 || Source->GetState<FCCLItemState>().Food.WorldId != WorldId ||
		!CCLFood::Split(Source->GetState<FCCLItemState>().Food, Count, WorldSeconds, Taken, Error) ||
		!BuildFoodAddition(*Destination, TargetEdit.GetEntries(), Source->Definition, Taken, WorldSeconds, AddedId))
	{
		return false;
	}

	if (Count == Source->Quantity && !Destination->Find(AddedId))
	{
		if (Destination->Find(SourceId))
		{
			return false;
		}

		TargetEdit.Find(AddedId)->Id = SourceId;
	}

	Source->Quantity -= Count;
	SourceEdit.GetEntries().RemoveAll([](const FKSIInventoryEntry& Entry)
									  { return Entry.Quantity == 0; });
	if (!SourceEdit.Validate() || !TargetEdit.Validate())
	{
		return false;
	}

	// Both authoritative arrays are final before either observer is notified.
	verify(SourceEdit.Commit());
	verify(TargetEdit.Commit());
	SourceEdit.Publish();
	TargetEdit.Publish();
	return true;
}

bool FCCLInventoryFood::ChangeFoodStorage(UCCLInventoryComponent& Inventory, FGuid Id, const FCCLFoodStorage& Storage)
{
	FGuid WorldId;
	double WorldSeconds = 0.;
	if (Inventory.GetFoodEnvironment().IsDynamic() || !FoodContext(Inventory, WorldId, WorldSeconds))
	{
		return false;
	}

	FKSIInventoryEdit Edit(Inventory);
	FKSIInventoryEntry* Entry = Edit.Find(Id);
	FString Error;
	return Entry && Entry->GetState<FCCLItemState>().Food.WorldId == WorldId &&
		   CCLFood::ChangeStorage(Entry->GetState<FCCLItemState>().Food, Storage, WorldSeconds, Error) && Edit.Commit();
}

bool FCCLInventoryFood::GetFoodQuality(const UCCLInventoryComponent& Inventory, FGuid Id, FCCLFoodQuality& Quality)
{
	FGuid WorldId;
	double WorldSeconds = 0.;
	const auto* Entry = Inventory.Find(Id);
	FString Error;
	return Entry && FoodContext(Inventory, WorldId, WorldSeconds) && Entry->GetState<FCCLItemState>().Food.WorldId == WorldId
		&& CCLFood::Evaluate(Entry->GetState<FCCLItemState>().Food, WorldSeconds, Quality, Error);
}

bool FCCLInventoryFood::ChangeFoodEnvironment(UCCLInventoryComponent& Inventory, const FCCLFoodEnvironmentBinding& Binding, FString& Error)
{
	auto* Runtime = Inventory.GetWorld() ? Inventory.GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>() : nullptr;
	FKSIInventoryEdit Edit(Inventory);
	if (!Edit.IsOpen() || !Inventory.HasPersistentOwner() || !Runtime || !Runtime->IsRunning())
	{
		Error = TEXT("Food environment requires an idle registered server inventory owner.");
		return false;
	}

	FCCLFoodEnvironmentBinding PreparedBinding = Binding;
	if (!CCLFoodEnvironment::Sample(PreparedBinding, Runtime->GetSurfaceSimulation(), Error))
	{
		return false;
	}

	for (FKSIInventoryEntry& Entry : Edit.GetEntries())
	{
		// 상수 접근으로 먼저 확인해 음식이 아닌 항목에 빈 상태를 만들지 않는다.
		if (!static_cast<const FKSIInventoryEntry&>(Entry).GetState<FCCLItemState>().Food.IsPresent())
		{
			continue;
		}

		FCCLFoodState& Food = Entry.GetState<FCCLItemState>().Food;
		if (Food.WorldId != Runtime->GetIdentity().WorldId ||
			!CCLFood::ChangeStorage(Food, PreparedBinding.Conditions, Runtime->GetClock().GetWorldSeconds(), Error))
		{
			return false;
		}
	}
	return Inventory.CommitWithFoodEnvironment(Edit, PreparedBinding);
}
