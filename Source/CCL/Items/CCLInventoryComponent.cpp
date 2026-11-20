#include "CCLInventoryComponent.h"

#include "CCLItemDefinition.h"
#include "CCLInventoryPersistenceSubsystem.h"
#include "Engine/World.h"
#include "CCLInventoryWeapon.h"
#include "CCLInventoryFood.h"
#include "KSIInventoryEdit.h"
#include "Net/UnrealNetwork.h"

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

bool UCCLInventoryComponent::CommitWithFoodEnvironment(FKSIInventoryEdit& Edit, const FCCLFoodEnvironmentBinding& Binding)
{
	if (Edit.GetInventory() != this || !Edit.Commit())
	{
		return false;
	}

	FoodEnvironment = Binding;
	return true;
}

bool UCCLInventoryComponent::CanAdd(const UKSIItemDefinition* Definition, int32 Quantity) const
{
	if (!Definition || Quantity <= 0 || Quantity > Definition->GetMaxStack())
	{
		return false;
	}

	const bool bPerishable = Definition->FindFragment<FCCLItemFragment_Perishable>() != nullptr;
	for (const FKSIInventoryEntry& Entry : GetEntries())
	{
		if (Entry.Slot < 0 || Entry.Definition != Definition || Quantity > Definition->GetMaxStack() - Entry.Quantity)
		{
			continue;
		}

		const FCCLFoodState& Food = Entry.GetState<FCCLItemState>().Food;
		if (bPerishable ? Food.IsPresent() && Food.Lots.Num() < CCLFood::MaxLots : CanStackEntry(Entry))
		{
			return true;
		}
	}
	return FKSIInventoryEdit::FindFreeSlot(GetEntries(), GetSlotCount()) != INDEX_NONE;
}

bool UCCLInventoryComponent::StageAdd(TArray<FKSIInventoryEntry>& Entries, UKSIItemDefinition* Definition, int32 Quantity, FGuid& OutId) const
{
	const FCCLItemFragment_Perishable* Fragment = Definition ? Definition->FindFragment<FCCLItemFragment_Perishable>() : nullptr;
	if (!Fragment)
	{
		return Super::StageAdd(Entries, Definition, Quantity, OutId);
	}

	if (Quantity <= 0 || Quantity > Definition->GetMaxStack())
	{
		return false;
	}

	FGuid WorldId;
	double WorldSeconds = 0.;
	FCCLFoodState Food;
	FString Error;
	return FCCLInventoryFood::FoodContext(*this, WorldId, WorldSeconds) &&
		   CCLFood::Create(WorldId, FGuid::NewGuid(), Quantity, WorldSeconds, Fragment->Policy, {}, Food, Error) &&
		   FCCLInventoryFood::BuildFoodAddition(*this, Entries, Definition, Food, WorldSeconds, OutId);
}

bool UCCLInventoryComponent::StageRemove(TArray<FKSIInventoryEntry>& Entries, FGuid Id, int32 Quantity) const
{
	if (!Id.IsValid() || Quantity <= 0)
	{
		return false;
	}

	FKSIInventoryEntry* Entry = Entries.FindByPredicate([Id](const FKSIInventoryEntry& Value)
														{ return Value.Id == Id; });
	if (!Entry || Entry->Quantity < Quantity)
	{
		return false;
	}

	// 읽기는 상수 접근으로 해서 상태가 없는 항목에 빈 상태를 만들지 않는다.
	const FCCLFoodState& CurrentFood = static_cast<const FKSIInventoryEntry&>(*Entry).GetState<FCCLItemState>().Food;
	if (CurrentFood.IsPresent())
	{
		FGuid WorldId;
		double WorldSeconds = 0.;
		FCCLFoodState Candidate = CurrentFood;
		FCCLFoodState Removed;
		FString Error;
		if (!FCCLInventoryFood::FoodContext(*this, WorldId, WorldSeconds) || WorldId != Candidate.WorldId ||
			!CCLFood::Split(Candidate, Quantity, WorldSeconds, Removed, Error))
		{
			return false;
		}

		Entry->GetState<FCCLItemState>().Food = MoveTemp(Candidate);
	}

	Entry->Quantity -= Quantity;
	if (Entry->Quantity == 0)
	{
		Entries.RemoveAll([Id](const FKSIInventoryEntry& Value)
						  { return Value.Id == Id; });
	}
	return true;
}

bool UCCLInventoryComponent::CanUsePlainTransfer(const FKSIInventoryEntry& Entry) const
{
	return Entry.Definition && !Entry.GetState<FCCLItemState>().Food.IsPresent() && Entry.GetState<FCCLItemState>().LoadedAmmo == 0 &&
		   !Entry.Definition->FindFragment<FCCLItemFragment_Perishable>();
}

bool UCCLInventoryComponent::CanStackEntry(const FKSIInventoryEntry& Entry) const
{
	return !Entry.GetState<FCCLItemState>().Food.IsPresent() && Entry.GetState<FCCLItemState>().LoadedAmmo == 0;
}
