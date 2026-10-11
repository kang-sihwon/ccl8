#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

class UKSIItemDefinition;
class UCCLItemDefinition;

namespace CCLItemTags
{
CCL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_LeftHand);
CCL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_RightHand);
CCL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Armor);
CCL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Boots);
CCL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Cloak);
CCL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_Necklace);
CCL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_RingOne);
CCL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Slot_RingTwo);
CCL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HandUsage_OneHanded);
CCL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HandUsage_TwoHanded);
CCL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attachment_GripLeft);
CCL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attachment_GripRight);
CCL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attachment_ShieldLeft);
CCL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attachment_ShieldRight);
CCL_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attachment_StowBack);
} // namespace CCLItemTags

namespace CCLEquipment
{
CCL_API const TArray<FGameplayTag>& Slots();
CCL_API FGameplayTag SlotAt(int32 Index);
CCL_API bool IsSlot(FGameplayTag Slot);
CCL_API bool IsHand(FGameplayTag Slot);
CCL_API bool IsTwoHanded(const UKSIItemDefinition* Item);
CCL_API bool GetOccupiedSlots(const UKSIItemDefinition* Item, FGameplayTag RequestedSlot, TArray<FGameplayTag>& OutSlots);
} // namespace CCLEquipment
