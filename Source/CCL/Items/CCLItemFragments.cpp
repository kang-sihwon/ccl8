#include "CCLItemFragments.h"

bool FCCLItemFragment_Equip::Allows(FGameplayTag Slot) const
{
	return CCLEquipment::IsSlot(Slot) && AllowedSlots.HasTagExact(Slot);
}

const FCCLItemAttachment* FCCLItemFragment_Visual::FindAttachment(FGameplayTag Slot) const
{
	return Attachments.FindByPredicate([Slot](const auto& Value) { return Value.Slot == Slot; });
}
