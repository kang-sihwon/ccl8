#include "CCLItemTags.h"

#include "CCLItemDefinition.h"

namespace CCLItemTags
{
UE_DEFINE_GAMEPLAY_TAG(Slot_LeftHand, "Equipment.Slot.Hand.Left");
UE_DEFINE_GAMEPLAY_TAG(Slot_RightHand, "Equipment.Slot.Hand.Right");
UE_DEFINE_GAMEPLAY_TAG(Slot_Armor, "Equipment.Slot.Armor");
UE_DEFINE_GAMEPLAY_TAG(Slot_Boots, "Equipment.Slot.Boots");
UE_DEFINE_GAMEPLAY_TAG(Slot_Cloak, "Equipment.Slot.Cloak");
UE_DEFINE_GAMEPLAY_TAG(Slot_Necklace, "Equipment.Slot.Necklace");
UE_DEFINE_GAMEPLAY_TAG(Slot_RingOne, "Equipment.Slot.Ring.One");
UE_DEFINE_GAMEPLAY_TAG(Slot_RingTwo, "Equipment.Slot.Ring.Two");
UE_DEFINE_GAMEPLAY_TAG(HandUsage_OneHanded, "Weapon.HandUsage.OneHanded");
UE_DEFINE_GAMEPLAY_TAG(HandUsage_TwoHanded, "Weapon.HandUsage.TwoHanded");
UE_DEFINE_GAMEPLAY_TAG(Attachment_GripLeft, "Attachment.Grip.Left");
UE_DEFINE_GAMEPLAY_TAG(Attachment_GripRight, "Attachment.Grip.Right");
UE_DEFINE_GAMEPLAY_TAG(Attachment_ShieldLeft, "Attachment.Shield.Left");
UE_DEFINE_GAMEPLAY_TAG(Attachment_ShieldRight, "Attachment.Shield.Right");
UE_DEFINE_GAMEPLAY_TAG(Attachment_StowBack, "Attachment.Stow.Back");
} // namespace CCLItemTags

const TArray<FGameplayTag>& CCLEquipment::Slots()
{
	// Stable UI order and legacy save mapping, never a runtime slot identifier.
	static const TArray<FGameplayTag> Values = {CCLItemTags::Slot_LeftHand, CCLItemTags::Slot_RightHand, CCLItemTags::Slot_Armor,
												CCLItemTags::Slot_Boots,	CCLItemTags::Slot_Cloak,	 CCLItemTags::Slot_Necklace,
												CCLItemTags::Slot_RingOne,	CCLItemTags::Slot_RingTwo};
	return Values;
}

FGameplayTag CCLEquipment::SlotAt(int32 Index)
{
	return Slots().IsValidIndex(Index) ? Slots()[Index] : FGameplayTag();
}

bool CCLEquipment::IsSlot(FGameplayTag Slot)
{
	return Slots().Contains(Slot);
}

bool CCLEquipment::IsHand(FGameplayTag Slot)
{
	return Slot == CCLItemTags::Slot_LeftHand || Slot == CCLItemTags::Slot_RightHand;
}

bool CCLEquipment::IsTwoHanded(const UCCLItemDefinition* Item)
{
	const auto* Weapon = Item ? Item->FindFragment<FCCLItemFragment_Weapon>() : nullptr;
	return Weapon && Weapon->HandUsage == CCLItemTags::HandUsage_TwoHanded;
}

bool CCLEquipment::GetOccupiedSlots(const UCCLItemDefinition* Item, FGameplayTag RequestedSlot, TArray<FGameplayTag>& OutSlots)
{
	OutSlots.Reset();
	const auto* Equip = Item ? Item->FindFragment<FCCLItemFragment_Equip>() : nullptr;
	if (!Equip || !Equip->Allows(RequestedSlot))
	{
		return false;
	}

	const auto* Weapon = Item->FindFragment<FCCLItemFragment_Weapon>();
	if (Weapon && (!IsHand(RequestedSlot) ||
				   (Weapon->HandUsage != CCLItemTags::HandUsage_OneHanded && Weapon->HandUsage != CCLItemTags::HandUsage_TwoHanded)))
	{
		return false;
	}

	if (IsTwoHanded(Item))
	{
		if (!Equip->Allows(CCLItemTags::Slot_LeftHand) || !Equip->Allows(CCLItemTags::Slot_RightHand))
		{
			return false;
		}

		OutSlots = {CCLItemTags::Slot_LeftHand, CCLItemTags::Slot_RightHand};
	}
	else
	{
		OutSlots.Add(RequestedSlot);
	}

	return true;
}
