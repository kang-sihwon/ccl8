#include "CCLInventoryFood.h"

#include "CCLInventoryComponent.h"

#include "CCLItemDefinition.h"
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
	if (Inventory.FoodEnvironment.IsDynamic() && !CCLFood::ChangeStorage(Food, Inventory.FoodEnvironment.Conditions, WorldSeconds, Error))
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

	int32 Slot = 0;
	while (Entries.ContainsByPredicate([Slot](const auto& Entry) { return Entry.Slot == Slot; }))
	{
		++Slot;
	}

	if (Slot >= FMath::Clamp(Inventory.Capacity, 1, 128))
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

FGuid FCCLInventoryFood::AddFood(UCCLInventoryComponent& Inventory, UKSIItemDefinition* Definition, const FCCLFoodState& Food)
{
	FGuid WorldId, AddedId;
	double WorldSeconds = 0.;
	if (!Inventory.GetOwner() || !Inventory.CanMutate() || !FoodContext(Inventory, WorldId, WorldSeconds) || Food.WorldId != WorldId)
	{
		return {};
	}

	auto Candidate = Inventory.List.Entries;
	return BuildFoodAddition(Inventory, Candidate, Definition, Food, WorldSeconds, AddedId) && Inventory.Restore(Candidate) ? AddedId : FGuid();
}

bool FCCLInventoryFood::MergeFood(UCCLInventoryComponent& Inventory, FGuid SourceId, FGuid DestinationId)
{
	FGuid WorldId;
	double WorldSeconds = 0.;
	if (!Inventory.GetOwner() || !Inventory.CanMutate() || SourceId == DestinationId || !FoodContext(Inventory, WorldId, WorldSeconds))
	{
		return false;
	}

	auto Candidate = Inventory.List.Entries;
	auto* Source = Candidate.FindByPredicate([SourceId](const auto& Entry) { return Entry.Id == SourceId; });
	auto* Destination = Candidate.FindByPredicate([DestinationId](const auto& Entry) { return Entry.Id == DestinationId; });
	FString Error;
	if (!Source || !Destination || Source->Slot < 0 || Destination->Slot < 0 || Source->Definition != Destination->Definition
		|| !Destination->Definition || Source->Quantity > Destination->Definition->GetMaxStack() - Destination->Quantity
		|| Source->GetState<FCCLItemState>().Food.WorldId != WorldId || !CCLFood::Merge(Destination->GetState<FCCLItemState>().Food, Source->GetState<FCCLItemState>().Food, WorldSeconds, Error))
	{
		return false;
	}

	Destination->Quantity += Source->Quantity;
	Candidate.RemoveAll([SourceId](const auto& Entry) { return Entry.Id == SourceId; });
	return Inventory.Restore(Candidate);
}

FGuid FCCLInventoryFood::SplitFood(UCCLInventoryComponent& Inventory, FGuid SourceId, int32 Count, int32 Slot)
{
	FGuid WorldId;
	double WorldSeconds = 0.;
	const auto* Original = Inventory.Find(SourceId);
	if (!Inventory.GetOwner() || !Inventory.CanMutate() || !Original || Original->Slot < 0 || Count >= Original->Quantity
		|| Slot < 0 || Slot >= FMath::Clamp(Inventory.Capacity, 1, 128) || Inventory.FindSlot(Slot) || !FoodContext(Inventory, WorldId, WorldSeconds)
		|| Original->GetState<FCCLItemState>().Food.WorldId != WorldId)
	{
		return {};
	}

	auto Candidate = Inventory.List.Entries;
	auto* Source = Candidate.FindByPredicate([SourceId](const auto& Entry) { return Entry.Id == SourceId; });
	FKSIInventoryEntry Taken;
	Taken.Id = FGuid::NewGuid();
	Taken.Definition = Original->Definition;
	Taken.Quantity = Count;
	Taken.Slot = Slot;
	FString Error;
	if (!CCLFood::Split(Source->GetState<FCCLItemState>().Food, Count, WorldSeconds, Taken.GetState<FCCLItemState>().Food, Error))
	{
		return {};
	}

	Source->Quantity -= Count;
	Candidate.Add(Taken);
	return Inventory.Restore(Candidate) ? Taken.Id : FGuid();
}

bool FCCLInventoryFood::TransferFood(UCCLInventoryComponent& Inventory, FGuid SourceId, int32 Count, UCCLInventoryComponent* Destination)
{
	FGuid WorldId, TargetWorldId, AddedId;
	double WorldSeconds = 0., TargetSeconds = 0.;
	if (!Inventory.GetOwner() || !Inventory.CanMutate() || !Destination || Destination == &Inventory
		|| !Destination->GetOwner() || !Destination->CanMutate()
		|| Destination->GetWorld() != Inventory.GetWorld() || Inventory.bTransactionOpen || Destination->bTransactionOpen
		|| !FoodContext(Inventory, WorldId, WorldSeconds) || !FoodContext(*Destination, TargetWorldId, TargetSeconds)
		|| WorldId != TargetWorldId || WorldSeconds != TargetSeconds)
	{
		return false;
	}

	auto SourceEntries = Inventory.List.Entries;
	auto TargetEntries = Destination->List.Entries;
	auto* Source = SourceEntries.FindByPredicate([SourceId](const auto& Entry) { return Entry.Id == SourceId; });
	FCCLFoodState Taken;
	FString Error;
	if (!Source || Source->Slot < 0 || Source->GetState<FCCLItemState>().Food.WorldId != WorldId
		|| !CCLFood::Split(Source->GetState<FCCLItemState>().Food, Count, WorldSeconds, Taken, Error)
		|| !BuildFoodAddition(*Destination, TargetEntries, Source->Definition, Taken, WorldSeconds, AddedId))
	{
		return false;
	}

	if (Count == Source->Quantity && !Destination->Find(AddedId))
	{
		if (Destination->Find(SourceId))
		{
			return false;
		}

		auto* Added = TargetEntries.FindByPredicate([AddedId](const auto& Entry) { return Entry.Id == AddedId; });
		Added->Id = SourceId;
	}

	Source->Quantity -= Count;
	SourceEntries.RemoveAll([](const auto& Entry) { return Entry.Quantity == 0; });
	const auto Before = Inventory.List.Entries;
	const auto TargetBefore = Destination->List.Entries;
	Inventory.BeginTransaction();
	Destination->BeginTransaction();
	const bool bApplied = Inventory.Restore(SourceEntries) && Destination->Restore(TargetEntries);
	if (!bApplied)
	{
		Inventory.Restore(Before);
		Destination->Restore(TargetBefore);
	}

	// Both authoritative arrays are final before either observer is notified.
	Inventory.EndTransaction(bApplied);
	Destination->EndTransaction(bApplied);
	return bApplied;
}

bool FCCLInventoryFood::ChangeFoodStorage(UCCLInventoryComponent& Inventory, FGuid Id, const FCCLFoodStorage& Storage)
{
	if (Inventory.FoodEnvironment.IsDynamic())
	{
		return false;
	}
	FGuid WorldId;
	double WorldSeconds = 0.;
	if (!Inventory.GetOwner() || !Inventory.CanMutate() || !FoodContext(Inventory, WorldId, WorldSeconds))
	{
		return false;
	}

	auto Candidate = Inventory.List.Entries;
	auto* Entry = Candidate.FindByPredicate([Id](const auto& Value) { return Value.Id == Id; });
	FString Error;
	return Entry && Entry->GetState<FCCLItemState>().Food.WorldId == WorldId
		&& CCLFood::ChangeStorage(Entry->GetState<FCCLItemState>().Food, Storage, WorldSeconds, Error) && Inventory.Restore(Candidate);
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
	const bool bPersistentOwner = Inventory.HasPersistentOwner();
	if (!Inventory.GetOwner() || !Inventory.CanMutate() || !bPersistentOwner || Inventory.bTransactionOpen
		|| !Runtime || !Runtime->IsRunning())
	{
		Error = TEXT("Food environment requires an idle registered server inventory owner.");
		return false;
	}
	auto PreparedBinding = Binding;
	if (!CCLFoodEnvironment::Sample(PreparedBinding, Runtime->GetSurfaceSimulation(), Error))
	{
		return false;
	}
	auto Candidate = Inventory.List.Entries;
	for (auto& Entry : Candidate)
	{
		if (Entry.GetState<FCCLItemState>().Food.IsPresent() && (Entry.GetState<FCCLItemState>().Food.WorldId != Runtime->GetIdentity().WorldId
			|| !CCLFood::ChangeStorage(Entry.GetState<FCCLItemState>().Food, PreparedBinding.Conditions, Runtime->GetClock().GetWorldSeconds(), Error)))
		{
			return false;
		}
	}
	if (!Inventory.CanRestore(Candidate) || !Inventory.BeginTransaction())
	{
		return false;
	}
	Inventory.FoodEnvironment = PreparedBinding;
	verify(Inventory.Restore(Candidate));
	Inventory.EndTransaction(true);
	return true;
}
