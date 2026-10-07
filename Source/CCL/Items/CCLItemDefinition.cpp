#include "CCLItemDefinition.h"

#include "Misc/DataValidation.h"

void UCCLItemDefinition::PostInitProperties()
{
	Super::PostInitProperties();
	// Keep the CDO at zero so new assets explicitly serialize version one.
	// Otherwise the serializer can omit it as a default and re-run migration on every load.
	FragmentSchemaVersion = HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad) ? 0 : 1;
}

void UCCLItemDefinition::PostLoad()
{
	Super::PostLoad();

	const bool bConvertFeatures = ItemFragments.IsEmpty();
	for (const UCCLItemFragment* Legacy : Fragments)
	{
		if (const auto* Display = Cast<UCCLItemFragment_Display>(Legacy))
		{
			if (ItemName.IsEmpty())
			{
				ItemName = Display->Label;
			}
		}
		else if (const auto* Stack = Cast<UCCLItemFragment_Stack>(Legacy))
		{
			MaxStackCount = Stack->MaxCount;
		}
		else if (bConvertFeatures)
		{
			if (const auto* Equip = Cast<UCCLItemFragment_Equipment>(Legacy))
			{
				FCCLItemFragment_Equip Value;
				Value.Effect = Equip->Effect;
				Value.Magnitude = Equip->Magnitude;
				ItemFragments.Add(FInstancedStruct::Make(Value));
			}
			else if (const auto* Use = Cast<UCCLItemFragment_Consumable>(Legacy))
			{
				FCCLItemFragment_ConsumableData Value;
				Value.Effect = Use->Effect;
				Value.Magnitude = Use->Magnitude;
				ItemFragments.Add(FInstancedStruct::Make(Value));
			}
			else if (const auto* Weapon = Cast<UCCLItemFragment_Combat>(Legacy))
			{
				FCCLItemFragment_MeleeWeapon Value;
				Value.Combat = Weapon->Combat;
				Value.Abilities = Weapon->Abilities;
				ItemFragments.Add(FInstancedStruct::Make(Value));
			}
		}
	}

	Fragments.Reset();

	if (FragmentSchemaVersion < 1)
	{
		bool bLegacyTwoHanded = false;
		for (auto& Fragment : ItemFragments)
		{
			if (auto* Equip = Fragment.GetMutablePtr<FCCLItemFragment_Equip>())
			{
				bLegacyTwoHanded = Equip->bTwoHanded != 0;
				Equip->DefaultSlotTag = CCLEquipment::SlotAt(static_cast<int32>(Equip->DefaultSlot));
				Equip->AllowedSlots.AddTag(Equip->DefaultSlotTag);
				if (CCLEquipment::IsHand(Equip->DefaultSlotTag))
				{
					Equip->AllowedSlots.AddTag(CCLItemTags::Slot_LeftHand);
					Equip->AllowedSlots.AddTag(CCLItemTags::Slot_RightHand);
				}
				else if (Equip->DefaultSlotTag == CCLItemTags::Slot_RingOne || Equip->DefaultSlotTag == CCLItemTags::Slot_RingTwo)
				{
					Equip->AllowedSlots.AddTag(CCLItemTags::Slot_RingOne);
					Equip->AllowedSlots.AddTag(CCLItemTags::Slot_RingTwo);
				}
			}

			if (auto* Visual = Fragment.GetMutablePtr<FCCLItemFragment_Visual>(); Visual && Visual->Attachments.IsEmpty())
			{
				// Only migrate the known old defaults. Custom sockets require an explicit profile mapping.
				for (int32 Hand = 0; Hand < 2; ++Hand)
				{
					const FName Socket = Hand == 0 ? Visual->LeftSocket : Visual->RightSocket;
					if (Socket == (Hand == 0 ? FName(TEXT("hand_l")) : FName(TEXT("hand_r"))))
					{
						FCCLItemAttachment Binding;
						Binding.Slot = CCLEquipment::SlotAt(Hand);
						Binding.Point = Hand == 0 ? CCLItemTags::Attachment_GripLeft : CCLItemTags::Attachment_GripRight;
						Binding.Offset = Visual->EquippedTransform;
						Visual->Attachments.Add(Binding);
					}
				}
			}
		}

		for (auto& Fragment : ItemFragments)
		{
			if (auto* Weapon = Fragment.GetMutablePtr<FCCLItemFragment_Weapon>())
			{
				Weapon->HandUsage = bLegacyTwoHanded ? CCLItemTags::HandUsage_TwoHanded : CCLItemTags::HandUsage_OneHanded;
			}
		}

		FragmentSchemaVersion = 1;
	}
}

FText UCCLItemDefinition::GetLabel() const
{
	return ItemName.IsEmpty() ? FText::FromString(GetName()) : ItemName;
}

int32 UCCLItemDefinition::GetMaxStack() const
{
	return FMath::Clamp(MaxStackCount, 1, 1000);
}

bool UCCLItemDefinition::ValidateDefinition(TArray<FText>& Errors) const
{
	auto Fail = [&Errors](const TCHAR* Message) { Errors.Add(FText::FromString(Message)); };
	const auto* Equip = FindFragment<FCCLItemFragment_Equip>();
	const auto* Weapon = FindFragment<FCCLItemFragment_Weapon>();
	const auto* Visual = FindFragment<FCCLItemFragment_Visual>();
	int32 EquipCount = 0, WeaponCount = 0, VisualCount = 0;
	for (const auto& Fragment : ItemFragments)
	{
		if (!Fragment.IsValid())
		{
			Fail(TEXT("Empty fragment."));
		}

		EquipCount += Fragment.GetPtr<FCCLItemFragment_Equip>() != nullptr;
		WeaponCount += Fragment.GetPtr<FCCLItemFragment_Weapon>() != nullptr;
		VisualCount += Fragment.GetPtr<FCCLItemFragment_Visual>() != nullptr;
	}

	if (EquipCount > 1 || WeaponCount > 1 || VisualCount > 1)
	{
		Fail(TEXT("Equip, Weapon and Visual each allow one fragment, including derived types."));
	}

	if (MaxStackCount < 1 || MaxStackCount > 1000 || (Equip && MaxStackCount != 1))
	{
		Fail(TEXT("Invalid stack count; equipment must have MaxStackCount 1."));
	}

	if (Weapon && Weapon->HandUsage != CCLItemTags::HandUsage_OneHanded && Weapon->HandUsage != CCLItemTags::HandUsage_TwoHanded)
	{
		Fail(TEXT("Weapon.HandUsage must be OneHanded or TwoHanded."));
	}

	if (Equip)
	{
		if (!Equip->Allows(Equip->DefaultSlotTag))
		{
			Fail(TEXT("Default slot must be an exact member of AllowedSlots."));
		}

		for (FGameplayTag Slot : Equip->AllowedSlots)
		{
			TArray<FGameplayTag> Occupied;
			if (!CCLEquipment::GetOccupiedSlots(this, Slot, Occupied))
			{
				Fail(TEXT("Invalid allowed slot or weapon hand occupancy."));
			}

			if (Visual && (!CCLEquipment::IsTwoHanded(this) || Slot == CCLItemTags::Slot_RightHand) && !Visual->FindAttachment(Slot))
			{
				Fail(TEXT("Visual needs an attachment for every rendered equipment slot."));
			}
		}
	}

	if (Visual)
	{
		TSet<FGameplayTag> Seen;
		const FGameplayTag Root = FGameplayTag::RequestGameplayTag(TEXT("Attachment"));
		for (const auto& Binding : Visual->Attachments)
		{
			if (!Equip || !Equip->Allows(Binding.Slot) || Seen.Contains(Binding.Slot) || !Binding.Point.MatchesTag(Root) ||
				Binding.Point == Root || Binding.Offset.ContainsNaN())
			{
				Fail(TEXT("Invalid or duplicate attachment binding."));
			}

			Seen.Add(Binding.Slot);
		}
	}

	return Errors.IsEmpty();
}

#if WITH_EDITOR
EDataValidationResult UCCLItemDefinition::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Parent = Super::IsDataValid(Context);
	TArray<FText> Errors;
	ValidateDefinition(Errors);
	for (const FText& Error : Errors)
	{
		Context.AddError(Error);
	}

	return Errors.IsEmpty() && Parent != EDataValidationResult::Invalid ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif
