#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

class UCCLItemDefinition;

namespace CCLItemTags
{
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_LeftHand);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_RightHand);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Armor);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Boots);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Cloak);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Necklace);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_RingOne);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_RingTwo);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(HandUsage_OneHanded);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(HandUsage_TwoHanded);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attachment_GripLeft);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attachment_GripRight);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attachment_ShieldLeft);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attachment_ShieldRight);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attachment_StowBack);
} // namespace CCLItemTags

namespace CCLEquipment
{
CCL_API const TArray<FGameplayTag>& Slots();
CCL_API FGameplayTag SlotAt(int32 Index);
CCL_API bool IsSlot(FGameplayTag Slot);
CCL_API bool IsHand(FGameplayTag Slot);
CCL_API bool IsTwoHanded(const UCCLItemDefinition* Item);
CCL_API bool GetOccupiedSlots(const UCCLItemDefinition* Item, FGameplayTag RequestedSlot, TArray<FGameplayTag>& OutSlots);
} // namespace CCLEquipment
