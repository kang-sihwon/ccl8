#include "CCLInventoryComponent.h"

#include "CCLItemDefinition.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Engine/World.h"

bool UCCLInventoryComponent::FoodContext(FGuid& WorldId, double& WorldSeconds) const
{
	const auto* Runtime = GetWorld() ? GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>() : nullptr;
	if (!Runtime || !Runtime->IsRunning())
	{
		return false;
	}

	WorldId = Runtime->GetIdentity().WorldId;
	WorldSeconds = Runtime->GetClock().GetWorldSeconds();
	return true;
}

bool UCCLInventoryComponent::BuildFoodAddition(TArray<FCCLInventoryEntry>& Entries, UCCLItemDefinition* Definition,
	const FCCLFoodState& SourceFood, double WorldSeconds, FGuid& AddedId) const
{
	FString Error;
	auto Food = SourceFood;
	if (FoodEnvironment.IsDynamic() && !CCLFood::ChangeStorage(Food, FoodEnvironment.Conditions, WorldSeconds, Error))
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
			FCCLFoodState Merged = Entry.Food;
			if (CCLFood::Merge(Merged, Food, WorldSeconds, Error))
			{
				Entry.Food = MoveTemp(Merged);
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

	if (Slot >= FMath::Clamp(Capacity, 1, 128))
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
	Entry.Food = MoveTemp(Settled);
	AddedId = Entry.Id;
	return true;
}

FGuid UCCLInventoryComponent::AddFood(UCCLItemDefinition* Definition, const FCCLFoodState& Food)
{
	FGuid WorldId, AddedId;
	double WorldSeconds = 0.;
	if (!GetOwner() || !GetOwner()->HasAuthority() || !FoodContext(WorldId, WorldSeconds) || Food.WorldId != WorldId)
	{
		return {};
	}

	auto Candidate = List.Entries;
	return BuildFoodAddition(Candidate, Definition, Food, WorldSeconds, AddedId) && Restore(Candidate) ? AddedId : FGuid();
}

bool UCCLInventoryComponent::MergeFood(FGuid SourceId, FGuid DestinationId)
{
	FGuid WorldId;
	double WorldSeconds = 0.;
	if (!GetOwner() || !GetOwner()->HasAuthority() || SourceId == DestinationId || !FoodContext(WorldId, WorldSeconds))
	{
		return false;
	}

	auto Candidate = List.Entries;
	auto* Source = Candidate.FindByPredicate([SourceId](const auto& Entry) { return Entry.Id == SourceId; });
	auto* Destination = Candidate.FindByPredicate([DestinationId](const auto& Entry) { return Entry.Id == DestinationId; });
	FString Error;
	if (!Source || !Destination || Source->Slot < 0 || Destination->Slot < 0 || Source->Definition != Destination->Definition
		|| !Destination->Definition || Source->Quantity > Destination->Definition->GetMaxStack() - Destination->Quantity
		|| Source->Food.WorldId != WorldId || !CCLFood::Merge(Destination->Food, Source->Food, WorldSeconds, Error))
	{
		return false;
	}

	Destination->Quantity += Source->Quantity;
	Candidate.RemoveAll([SourceId](const auto& Entry) { return Entry.Id == SourceId; });
	return Restore(Candidate);
}

FGuid UCCLInventoryComponent::SplitFood(FGuid SourceId, int32 Count, int32 Slot)
{
	FGuid WorldId;
	double WorldSeconds = 0.;
	const auto* Original = Find(SourceId);
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Original || Original->Slot < 0 || Count >= Original->Quantity
		|| Slot < 0 || Slot >= FMath::Clamp(Capacity, 1, 128) || FindSlot(Slot) || !FoodContext(WorldId, WorldSeconds)
		|| Original->Food.WorldId != WorldId)
	{
		return {};
	}

	auto Candidate = List.Entries;
	auto* Source = Candidate.FindByPredicate([SourceId](const auto& Entry) { return Entry.Id == SourceId; });
	FCCLInventoryEntry Taken;
	Taken.Id = FGuid::NewGuid();
	Taken.Definition = Original->Definition;
	Taken.Quantity = Count;
	Taken.Slot = Slot;
	FString Error;
	if (!CCLFood::Split(Source->Food, Count, WorldSeconds, Taken.Food, Error))
	{
		return {};
	}

	Source->Quantity -= Count;
	Candidate.Add(Taken);
	return Restore(Candidate) ? Taken.Id : FGuid();
}

bool UCCLInventoryComponent::TransferFood(FGuid SourceId, int32 Count, UCCLInventoryComponent* Destination)
{
	FGuid WorldId, TargetWorldId, AddedId;
	double WorldSeconds = 0., TargetSeconds = 0.;
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Destination || Destination == this
		|| !Destination->GetOwner() || !Destination->GetOwner()->HasAuthority()
		|| Destination->GetWorld() != GetWorld() || bTransactionOpen || Destination->bTransactionOpen
		|| !FoodContext(WorldId, WorldSeconds) || !Destination->FoodContext(TargetWorldId, TargetSeconds)
		|| WorldId != TargetWorldId || WorldSeconds != TargetSeconds)
	{
		return false;
	}

	auto SourceEntries = List.Entries;
	auto TargetEntries = Destination->List.Entries;
	auto* Source = SourceEntries.FindByPredicate([SourceId](const auto& Entry) { return Entry.Id == SourceId; });
	FCCLFoodState Taken;
	FString Error;
	if (!Source || Source->Slot < 0 || Source->Food.WorldId != WorldId
		|| !CCLFood::Split(Source->Food, Count, WorldSeconds, Taken, Error)
		|| !Destination->BuildFoodAddition(TargetEntries, Source->Definition, Taken, WorldSeconds, AddedId))
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
	const auto Before = List.Entries;
	const auto TargetBefore = Destination->List.Entries;
	BeginTransaction();
	Destination->BeginTransaction();
	const bool bApplied = Restore(SourceEntries) && Destination->Restore(TargetEntries);
	if (!bApplied)
	{
		Restore(Before);
		Destination->Restore(TargetBefore);
	}

	// Both authoritative arrays are final before either observer is notified.
	EndTransaction(bApplied);
	Destination->EndTransaction(bApplied);
	return bApplied;
}

bool UCCLInventoryComponent::ChangeFoodStorage(FGuid Id, const FCCLFoodStorage& Storage)
{
	if (FoodEnvironment.IsDynamic())
	{
		return false;
	}
	FGuid WorldId;
	double WorldSeconds = 0.;
	if (!GetOwner() || !GetOwner()->HasAuthority() || !FoodContext(WorldId, WorldSeconds))
	{
		return false;
	}

	auto Candidate = List.Entries;
	auto* Entry = Candidate.FindByPredicate([Id](const auto& Value) { return Value.Id == Id; });
	FString Error;
	return Entry && Entry->Food.WorldId == WorldId
		&& CCLFood::ChangeStorage(Entry->Food, Storage, WorldSeconds, Error) && Restore(Candidate);
}

bool UCCLInventoryComponent::GetFoodQuality(FGuid Id, FCCLFoodQuality& Quality) const
{
	FGuid WorldId;
	double WorldSeconds = 0.;
	const auto* Entry = Find(Id);
	FString Error;
	return Entry && FoodContext(WorldId, WorldSeconds) && Entry->Food.WorldId == WorldId
		&& CCLFood::Evaluate(Entry->Food, WorldSeconds, Quality, Error);
}


bool UCCLInventoryComponent::ChangeFoodEnvironment(const FCCLFoodEnvironmentBinding& Binding, FString& Error)
{
	auto* Runtime = GetWorld() ? GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>() : nullptr;
	const bool bPersistentOwner = HasPersistentOwner();
	if (!GetOwner() || !GetOwner()->HasAuthority() || !bPersistentOwner || bTransactionOpen
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
	auto Candidate = List.Entries;
	for (auto& Entry : Candidate)
	{
		if (Entry.Food.IsPresent() && (Entry.Food.WorldId != Runtime->GetIdentity().WorldId
			|| !CCLFood::ChangeStorage(Entry.Food, PreparedBinding.Conditions, Runtime->GetClock().GetWorldSeconds(), Error)))
		{
			return false;
		}
	}
	if (!CanRestore(Candidate) || !BeginTransaction())
	{
		return false;
	}
	FoodEnvironment = PreparedBinding;
	verify(Restore(Candidate));
	EndTransaction(true);
	return true;
}
