#include "CCLInventoryComponent.h"

#include "CCLItemDefinition.h"
#include "CCLInventoryPersistenceSubsystem.h"
#include "Engine/World.h"
#include "CCLInventoryWeapon.h"
#include "CCLInventoryFood.h"
#include "Net/UnrealNetwork.h"

FGuid UCCLInventoryComponent::Add(UKSIItemDefinition* Definition, int32 Quantity)
{
	if (!CanMutate() || !CanAdd(Definition, Quantity))
	{
		return FGuid();
	}
	if (const auto* Fragment = Definition->FindFragment<FCCLItemFragment_Perishable>())
	{
		FGuid WorldId;
		double WorldSeconds = 0.;
		FCCLFoodState Food;
		FString Error;
		return FCCLInventoryFood::FoodContext(*this, WorldId, WorldSeconds)
			&& CCLFood::Create(WorldId, FGuid::NewGuid(), Quantity, WorldSeconds, Fragment->Policy, {}, Food, Error)
			? FCCLInventoryFood::AddFood(*this, Definition, Food) : FGuid();
	}

    return Super::Add(Definition, Quantity);
}

bool UCCLInventoryComponent::Remove(FGuid Id, int32 Quantity)
{
	if (!CanMutate() || !Id.IsValid() || Quantity <= 0)
	{
		return false;
	}
	const int32 Index = List.Entries.IndexOfByPredicate([Id](const FKSIInventoryEntry& Entry) { return Entry.Id == Id; });
	if (Index == INDEX_NONE || List.Entries[Index].Quantity < Quantity)
	{
		return false;
	}
	const auto& Current = List.Entries[Index];
    if (Current.GetState<FCCLItemState>().Food.IsPresent())
	{
		FGuid WorldId;
		double WorldSeconds = 0.;
		FCCLFoodState Candidate = Current.GetState<FCCLItemState>().Food;
		FCCLFoodState Removed;
		FString Error;
		if (!FCCLInventoryFood::FoodContext(*this, WorldId, WorldSeconds) || WorldId != Candidate.WorldId
			|| !CCLFood::Split(Candidate, Quantity, WorldSeconds, Removed, Error))
		{
			return false;
		}

		List.Entries[Index].GetState<FCCLItemState>().Food = MoveTemp(Candidate);
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

bool UCCLInventoryComponent::CanAdd(const UKSIItemDefinition* Definition, int32 Quantity) const
{
	if (!Definition || Quantity <= 0 || Quantity > Definition->GetMaxStack())
	{
		return false;
	}
	for (const auto& Entry : List.Entries)
	{
		if (Entry.Slot >= 0 && Entry.Definition == Definition && Quantity <= Definition->GetMaxStack() - Entry.Quantity
			&& (Definition->FindFragment<FCCLItemFragment_Perishable>() ? Entry.GetState<FCCLItemState>().Food.IsPresent()
				&& Entry.GetState<FCCLItemState>().Food.Lots.Num() < CCLFood::MaxLots : CanStackEntry(Entry)))
		{
			return true;
		}
	}
	return List.Entries.FilterByPredicate([](const FKSIInventoryEntry& Entry) { return Entry.Slot >= 0; }).Num() <
	       FMath::Clamp(Capacity, 1, 128);
}

bool UCCLInventoryComponent::CanRestore(const TArray<FKSIInventoryEntry>& Entries) const
{
	return GetOwner() && GetOwner()->HasAuthority() && ValidateEntries(Entries, Capacity);
}

bool UCCLInventoryComponent::ValidateEntries(const TArray<FKSIInventoryEntry>& Entries, int32 Capacity)
{
	if (Capacity < 1 || Capacity > 128 || Entries.Num() > Capacity + 128)
	{
		return false;
	}

	TSet<FGuid> Seen;
	TSet<int32> Slots;
	for (const auto& Entry : Entries)
	{
		if ((Entry.State.IsValid() && !Entry.State.GetPtr<FCCLItemState>()) || !Entry.Id.IsValid() || Seen.Contains(Entry.Id) || !Entry.Definition || Entry.Quantity <= 0 || Entry.Quantity > Entry.Definition->GetMaxStack())
		{
			return false;
		}

		if (Entry.Slot < INDEX_NONE || Entry.Slot >= FMath::Clamp(Capacity, 1, 128) || (Entry.Slot >= 0 && Slots.Contains(Entry.Slot)))
		{
			return false;
		}

		if (!FCCLInventoryWeapon::Validate(Entry))
		{
			return false;
		}

		FString FoodError;
		if (!CCLFood::Validate(Entry.GetState<FCCLItemState>().Food, Entry.GetState<FCCLItemState>().Food.IsPresent() ? Entry.Quantity : 0, FoodError)
			|| (Entry.GetState<FCCLItemState>().Food.IsPresent() && !Entry.Definition->FindFragment<FCCLItemFragment_Perishable>()))
		{
			return false;
		}

		Slots.Add(Entry.Slot);
		Seen.Add(Entry.Id);
	}
	return true;
}

void UCCLInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (auto* Store = GetWorld()->GetSubsystem<UCCLInventoryPersistenceSubsystem>())
	{
		Store->Unbind(*this);
	}
	Super::EndPlay(EndPlayReason);
}

bool UCCLInventoryComponent::BindPersistence(FGuid ContainerId, FString& Error)
{
	auto* Store = GetWorld() ? GetWorld()->GetSubsystem<UCCLInventoryPersistenceSubsystem>() : nullptr;
	return Store && Store->Bind(*this, ContainerId, Error);
}

bool UCCLInventoryComponent::CanUsePlainTransfer(const FKSIInventoryEntry& Entry) const
{
    return Entry.Definition && !Entry.GetState<FCCLItemState>().Food.IsPresent() && Entry.GetState<FCCLItemState>().LoadedAmmo == 0
        && !Entry.Definition->FindFragment<FCCLItemFragment_Perishable>();
}

bool UCCLInventoryComponent::CanStackEntry(const FKSIInventoryEntry& Entry) const
{
    return !Entry.GetState<FCCLItemState>().Food.IsPresent() && Entry.GetState<FCCLItemState>().LoadedAmmo == 0;
}
