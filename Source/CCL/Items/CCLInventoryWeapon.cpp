#include "CCLInventoryWeapon.h"

#include "CCLInventoryComponent.h"
#include "CCLItemDefinition.h"
#include "KSIInventoryEdit.h"
#include "Actions/CCLWeaponAbility.h"

bool FCCLInventoryWeapon::ConsumeShot(UCCLInventoryComponent& Inventory, FGuid Id)
{
	if (!CanFire(Inventory, Id))
	{
		return false;
	}

	FKSIInventoryEdit Edit(Inventory);
	FKSIInventoryEntry* Entry = Edit.Find(Id);
	if (!Entry)
	{
		return false;
	}

	--Entry->GetState<FCCLItemState>().LoadedAmmo;
	return Edit.Commit();
}

bool FCCLInventoryWeapon::Reload(UCCLInventoryComponent& Inventory, FGuid Id)
{
	if (!CanReload(Inventory, Id))
	{
		return false;
	}

	FKSIInventoryEdit Edit(Inventory);
	FKSIInventoryEntry* Weapon = Edit.Find(Id);
	if (!Weapon)
	{
		return false;
	}

	const UCCLProjectileProfile* Profile = Weapon->Definition->FindFragment<FCCLItemFragment_ProjectileWeapon>()->Profile.Get();
	int32& LoadedAmmo = Weapon->GetState<FCCLItemState>().LoadedAmmo;
	int32 Remaining = Profile->MagazineSize - LoadedAmmo;
	for (FKSIInventoryEntry& Entry : Edit.GetEntries())
	{
		if (Entry.Definition == Profile->Ammunition && Entry.Slot >= 0 && Entry.Id != Id)
		{
			const int32 Used = FMath::Min(Entry.Quantity, Remaining);
			Entry.Quantity -= Used;
			LoadedAmmo += Used;
			Remaining -= Used;
		}
	}

	Edit.GetEntries().RemoveAll([](const FKSIInventoryEntry& Entry)
								{ return Entry.Quantity == 0; });
	return Edit.Commit();
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
		   Inventory.GetEntries().ContainsByPredicate([Profile, Id](const FKSIInventoryEntry& E)
													  { return E.Id != Id && E.Definition == Profile->Ammunition && E.Slot >= 0 && E.Quantity > 0; });
}

bool FCCLInventoryWeapon::Validate(const FKSIInventoryEntry& Entry)
{
	const auto* Weapon = Entry.Definition ? Entry.Definition->FindFragment<FCCLItemFragment_ProjectileWeapon>() : nullptr;
	const int32 Maximum = Weapon && Weapon->Profile ? Weapon->Profile->MagazineSize : 0;
	return Entry.GetState<FCCLItemState>().LoadedAmmo >= 0 && Entry.GetState<FCCLItemState>().LoadedAmmo <= Maximum;
}
