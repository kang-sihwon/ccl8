#include "CCLInventoryWeapon.h"

#include "CCLInventoryComponent.h"
#include "CCLItemDefinition.h"
#include "Actions/CCLWeaponAbility.h"

bool FCCLInventoryWeapon::ConsumeShot(UCCLInventoryComponent& Inventory, FGuid Id)
{
	if (!Inventory.CanMutate() || !CanFire(Inventory, Id))
	{
		return false;
	}

	auto* Entry = Inventory.List.Entries.FindByPredicate([Id](const FKSIInventoryEntry& E) { return E.Id == Id; });
	--Entry->GetState<FCCLItemState>().LoadedAmmo;
	Inventory.List.MarkItemDirty(*Entry);
	Inventory.NotifyChanged();
	return true;
}

bool FCCLInventoryWeapon::Reload(UCCLInventoryComponent& Inventory, FGuid Id)
{
	if (!Inventory.CanMutate() || !CanReload(Inventory, Id))
	{
		return false;
	}

	TArray<FKSIInventoryEntry> Candidate = Inventory.List.Entries;
	auto* Weapon = Candidate.FindByPredicate([Id](const FKSIInventoryEntry& E) { return E.Id == Id; });
	const auto* Profile = Weapon->Definition->FindFragment<FCCLItemFragment_ProjectileWeapon>()->Profile.Get();
	int32 Remaining = Profile->MagazineSize - Weapon->GetState<FCCLItemState>().LoadedAmmo;
	for (auto& Entry : Candidate)
	{
		if (Entry.Definition == Profile->Ammunition && Entry.Slot >= 0 && Entry.Id != Id)
		{
			const int32 Used = FMath::Min(Entry.Quantity, Remaining);
			Entry.Quantity -= Used;
			Weapon->GetState<FCCLItemState>().LoadedAmmo += Used;
			Remaining -= Used;
		}
	}

	Candidate.RemoveAll([](const FKSIInventoryEntry& E) { return E.Quantity == 0; });
	return Inventory.Restore(Candidate);
}

bool FCCLInventoryWeapon::CanFire(const UCCLInventoryComponent& Inventory, FGuid Id)
{
	const auto* Entry = Inventory.Find(Id);
	return Entry && Entry->GetState<FCCLItemState>().LoadedAmmo > 0;
}

bool FCCLInventoryWeapon::CanReload(const UCCLInventoryComponent& Inventory, FGuid Id)
{
	const auto* Entry = Inventory.Find(Id);
	const auto* Weapon = Entry && Entry->Definition ? Entry->Definition->FindFragment<FCCLItemFragment_ProjectileWeapon>() : nullptr;
	const auto* Profile = Weapon ? Weapon->Profile.Get() : nullptr;
	return Profile && Profile->MagazineSize > Entry->GetState<FCCLItemState>().LoadedAmmo && Profile->Ammunition &&
		Inventory.List.Entries.ContainsByPredicate([Profile, Id](const FKSIInventoryEntry& E)
		{
			return E.Id != Id && E.Definition == Profile->Ammunition && E.Slot >= 0 && E.Quantity > 0;
		});
}

bool FCCLInventoryWeapon::Validate(const FKSIInventoryEntry& Entry)
{
	const auto* Weapon = Entry.Definition ? Entry.Definition->FindFragment<FCCLItemFragment_ProjectileWeapon>() : nullptr;
	const int32 Maximum = Weapon && Weapon->Profile ? Weapon->Profile->MagazineSize : 0;
	return Entry.GetState<FCCLItemState>().LoadedAmmo >= 0 && Entry.GetState<FCCLItemState>().LoadedAmmo <= Maximum;
}
