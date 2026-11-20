#include "CCLItemFragments.h"

namespace
{
	void Fail(const FKSIItemValidation& Context, const TCHAR* Message)
	{
		Context.Error(FText::FromString(Message));
	}
} // namespace

void FCCLEquipmentFragment::Validate(const FKSIItemValidation& Context) const
{
	const UKSIItemDefinition* Item = &Context.Item;
	if (Item->MaxStackCount != 1)
	{
		Fail(Context, TEXT("Equipment must have MaxStackCount 1."));
	}

	if (!Allows(DefaultSlot))
	{
		Fail(Context, TEXT("Default slot must be an exact member of AllowedSlots."));
	}

	const bool bTwoHanded = CCLEquipment::IsTwoHanded(Item);
	const bool bVisible = Item->FindFragment<FCCLVisualFragment>() != nullptr;
	for (FGameplayTag Slot : AllowedSlots)
	{
		TArray<FGameplayTag> Occupied;
		if (!CCLEquipment::GetOccupiedSlots(Item, Slot, Occupied))
		{
			Fail(Context, TEXT("Invalid allowed slot or hand occupancy."));
		}

		// 양손 장비는 오른손에만 메시를 표시한다.
		if (bVisible && (!bTwoHanded || Slot == CCLItemTags::Slot_RightHand) && !FindAttachment(Slot))
		{
			Fail(Context, TEXT("A visible item needs an attachment for every rendered equipment slot."));
		}
	}

	TSet<FGameplayTag> Seen;
	const FGameplayTag Root = FGameplayTag::RequestGameplayTag(TEXT("Attachment"));
	for (const FCCLItemAttachment& Binding : Attachments)
	{
		if (!Allows(Binding.Slot) || Seen.Contains(Binding.Slot) || !Binding.Point.MatchesTag(Root) || Binding.Point == Root ||
			Binding.Offset.ContainsNaN())
		{
			Fail(Context, TEXT("Invalid or duplicate attachment binding."));
		}

		Seen.Add(Binding.Slot);
	}
}

bool FCCLEquipmentFragment::Allows(FGameplayTag Slot) const
{
	return CCLEquipment::IsSlot(Slot) && AllowedSlots.HasTagExact(Slot);
}

const FCCLItemAttachment* FCCLEquipmentFragment::FindAttachment(FGameplayTag Slot) const
{
	return Attachments.FindByPredicate([Slot](const FCCLItemAttachment& Value)
									   { return Value.Slot == Slot; });
}

void FCCLHandEquipmentFragment::Validate(const FKSIItemValidation& Context) const
{
	Super::Validate(Context);
	if (HandUsage != CCLItemTags::HandUsage_OneHanded && HandUsage != CCLItemTags::HandUsage_TwoHanded)
	{
		Fail(Context, TEXT("HandUsage must be OneHanded or TwoHanded."));
	}
}

void FCCLRangedWeaponFragment::Validate(const FKSIItemValidation& Context) const
{
	Super::Validate(Context);
	if (!Profile)
	{
		Fail(Context, TEXT("A ranged weapon needs a projectile profile."));
	}
}

void FCCLPerishableFragment::Validate(const FKSIItemValidation& Context) const
{
	if (!CCLFood::ValidatePolicy(Policy))
	{
		Fail(Context, TEXT("Invalid perishable food policy."));
	}
}
