#include "CCLSessionRecord.h"
#include "Items/CCLItemDefinition.h"
#include "Items/CCLSkillDefinition.h"
#include "Actions/CCLWeaponAbility.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/Crc.h"
UCCLItemDefinition* FCCLSessionCodec::Item(FName Id)
{
	static const TSet<FName> Known = {TEXT("DA_IronGauntlets"),	 TEXT("DA_RecoveryPotion"), TEXT("DA_TrainingSword"),
									  TEXT("DA_TrainingShield"), TEXT("DA_TrainingStaff"),	TEXT("DA_TrainingArmor"),
									  TEXT("DA_TrainingBoots"),	 TEXT("DA_TrainingCloak"),	TEXT("DA_TrainingNecklace"),
									  TEXT("DA_TrainingRing"), TEXT("DA_Pistol"), TEXT("DA_Rifle"), TEXT("DA_Bullets")};
	if (!Known.Contains(Id))
	{
		return nullptr;
	}

	return LoadObject<UCCLItemDefinition>(nullptr, *FString::Printf(TEXT("/Game/Progression/%s.%s"), *Id.ToString(), *Id.ToString()));
}

UCCLSkillDefinition* FCCLSessionCodec::Skill(FName Id)
{
	if (Id != TEXT("DA_PowerTraining") && Id != TEXT("DA_VitalityTraining"))
	{
		return nullptr;
	}

	return LoadObject<UCCLSkillDefinition>(nullptr, *FString::Printf(TEXT("/Game/Progression/%s.%s"), *Id.ToString(), *Id.ToString()));
}

bool FCCLSessionCodec::Validate(const FCCLSessionRecord& R)
{
	if (R.Version < 1 || R.Version > 5 || R.Items.Num() > (R.Version >= 3 ? 24 : 16) || R.Skills.Num() > 2 || R.Points < 0 ||
		R.Points > 1000 || R.Coins < 0 || R.Coins > 1000000 || R.Quest > 2 || R.DefeatedGuards > 3 || R.Victory > 1 ||
		(R.Victory && R.DefeatedGuards != 3) || (R.Quest == 2 && !R.Victory) || R.RemainingSupplies.Num() > 2)
	{
		return false;
	}

	TSet<FGuid> Ids;
	TSet<int32> BagSlots;
	bool bLegacyEquipmentFound = !R.Equipped.IsValid();
	for (const auto& Entry : R.Items)
	{
		const auto* Definition = Item(Entry.Definition);
		if (!Entry.Id.IsValid() || Ids.Contains(Entry.Id) || !Definition || Entry.Quantity <= 0 ||
			Entry.Quantity > Definition->GetMaxStack())
		{
			return false;
		}

		const auto* Weapon = Definition->FindFragment<FCCLItemFragment_ProjectileWeapon>();
		const int32 Maximum = Weapon && Weapon->Profile ? Weapon->Profile->MagazineSize : 0;
		if (Entry.LoadedAmmo < 0 || Entry.LoadedAmmo > Maximum)
		{
			return false;
		}

		Ids.Add(Entry.Id);
		if (R.Version >= 2)
		{
			if (Entry.Slot < (R.Version >= 3 ? -1 : 0) || Entry.Slot >= 16 || (Entry.Slot >= 0 && BagSlots.Contains(Entry.Slot)))
			{
				return false;
			}

			if (Entry.Slot >= 0)
			{
				BagSlots.Add(Entry.Slot);
			}
		}

		if (R.Equipped == Entry.Id)
		{
			bLegacyEquipmentFound = Definition->FindFragment<FCCLItemFragment_Equip>() != nullptr;
		}
	}

	if (R.Version < 3 && !bLegacyEquipmentFound)
	{
		return false;
	}

	if (R.Version >= 3)
	{
		if (R.EquipmentSlots.Num() > 8)
		{
			return false;
		}

		TSet<FGameplayTag> Used;
		TMap<FGuid, TArray<FGameplayTag>> Occupancy;
		for (const auto& Slot : R.EquipmentSlots)
		{
			if (!CCLEquipment::IsSlot(Slot.Slot) || Used.Contains(Slot.Slot))
			{
				return false;
			}

			const auto* Entry = R.Items.FindByPredicate([&](const auto& Value) { return Value.Id == Slot.Id; });
			const auto* Definition = Entry ? Item(Entry->Definition) : nullptr;
			const auto* Equip = Definition ? Definition->FindFragment<FCCLItemFragment_Equip>() : nullptr;
			if (!Entry || Entry->Slot != INDEX_NONE || Entry->Quantity != 1 || !Equip || !Equip->Allows(Slot.Slot))
			{
				return false;
			}

			Used.Add(Slot.Slot);
			Occupancy.FindOrAdd(Slot.Id).Add(Slot.Slot);
		}

		for (const auto& Entry : R.Items)
		{
			if (Entry.Slot != INDEX_NONE)
			{
				continue;
			}

			const auto* ActualSlots = Occupancy.Find(Entry.Id);
			if (!ActualSlots)
			{
				return false;
			}

			TArray<FGameplayTag> ExpectedSlots;
			const auto* Slot = R.EquipmentSlots.FindByPredicate([&](const auto& Value) { return Value.Id == Entry.Id; });
			if (!Slot || !CCLEquipment::GetOccupiedSlots(Item(Entry.Definition), Slot->Slot, ExpectedSlots) ||
				ExpectedSlots.Num() != ActualSlots->Num() ||
				ExpectedSlots.ContainsByPredicate([&](FGameplayTag Tag) { return !ActualSlots->Contains(Tag); }))
			{
				return false;
			}
		}
	}

	TSet<FName> Skills;
	for (FName Id : R.Skills)
	{
		if (!Skill(Id) || Skills.Contains(Id))
		{
			return false;
		}

		Skills.Add(Id);
	}

	TSet<FName> Supplies;
	for (FName Id : R.RemainingSupplies)
	{
		if (!Item(Id) || Supplies.Contains(Id))
		{
			return false;
		}

		Supplies.Add(Id);
	}

	return true;
}

bool FCCLSessionCodec::Encode(const FCCLSessionRecord& Record, TArray<uint8>& Bytes)
{
	FString Json;
	if (!Validate(Record))
	{
		return false;
	}

	const auto Object = FJsonObjectConverter::UStructToJsonObject(Record);
	if (!Object)
	{
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> Equipment;
	for (const auto& Slot : Record.EquipmentSlots)
	{
		auto Value = MakeShared<FJsonObject>();
		if (Record.Version >= 4)
		{
			Value->SetStringField(TEXT("slot"), Slot.Slot.ToString());
		}
		else
		{
			Value->SetNumberField(TEXT("slot"), CCLEquipment::Slots().IndexOfByKey(Slot.Slot));
		}

		Value->SetStringField(TEXT("id"), Slot.Id.ToString());
		Equipment.Add(MakeShared<FJsonValueObject>(Value));
	}

	Object->SetArrayField(TEXT("equipmentSlots"), Equipment);
	if (!FJsonSerializer::Serialize(Object.ToSharedRef(), TJsonWriterFactory<>::Create(&Json)))
	{
		return false;
	}

	FTCHARToUTF8 Utf8(*Json);
	if (Utf8.Length() > 65528)
	{
		return false;
	}

	Bytes.Reset();
	Bytes.Append(reinterpret_cast<const uint8*>("CCL1"), 4);
	const uint32 Checksum = FCrc::MemCrc32(Utf8.Get(), Utf8.Length());
	Bytes.Append(reinterpret_cast<const uint8*>(&Checksum), 4);
	Bytes.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
	return true;
}

bool FCCLSessionCodec::Decode(const TArray<uint8>& Bytes, FCCLSessionRecord& Record)
{
	if (Bytes.Num() < 12 || Bytes.Num() > 65536 || FMemory::Memcmp(Bytes.GetData(), "CCL1", 4) != 0)
	{
		return false;
	}

	uint32 Expected;
	FMemory::Memcpy(&Expected, Bytes.GetData() + 4, 4);
	if (Expected != FCrc::MemCrc32(Bytes.GetData() + 8, Bytes.Num() - 8))
	{
		return false;
	}

	FUTF8ToTCHAR Text(reinterpret_cast<const ANSICHAR*>(Bytes.GetData() + 8), Bytes.Num() - 8);
	FCCLSessionRecord Candidate;
	TSharedPtr<FJsonObject> Json;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(FString(Text.Length(), Text.Get())), Json) || !Json)
	{
		return false;
	}

	auto Number = [](const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, double Min, double Max)
	{
		double Value;
		return Object && Object->TryGetNumberField(Key, Value) && FMath::IsFinite(Value) && Value >= Min && Value <= Max &&
			   FMath::FloorToDouble(Value) == Value;
	};
	if (!Number(Json, TEXT("version"), 1, 5) || !Number(Json, TEXT("points"), 0, 1000) || !Number(Json, TEXT("coins"), 0, 1000000) ||
		!Number(Json, TEXT("quest"), 0, 2) || !Number(Json, TEXT("defeatedGuards"), 0, 3) || !Number(Json, TEXT("victory"), 0, 1))
	{
		return false;
	}

	const bool bHasSlots = Json->GetNumberField(TEXT("version")) >= 2;
	const TArray<TSharedPtr<FJsonValue>>* Items;
	if (!Json->TryGetArrayField(TEXT("items"), Items) || Items->Num() > 24)
	{
		return false;
	}

	for (const auto& Value : *Items)
	{
		if (!Value || Value->Type != EJson::Object || !Number(Value->AsObject(), TEXT("quantity"), 1, 1000) ||
			(bHasSlots && !Number(Value->AsObject(), TEXT("slot"), Json->GetNumberField(TEXT("version")) >= 3 ? -1 : 0, 15)))
		{
			return false;
		}
	}

	for (const auto& Value : *Items)
	{
		if (Json->GetNumberField(TEXT("version")) < 5)
		{
			Value->AsObject()->SetNumberField(TEXT("loadedAmmo"), 0);
		}
		else if (!Number(Value->AsObject(), TEXT("loadedAmmo"), 0, 1000))
		{
			return false;
		}
	}

	if (!bHasSlots)
	{
		for (int32 Index = 0; Index < Items->Num(); ++Index)
		{
			(*Items)[Index]->AsObject()->SetNumberField(TEXT("slot"), Index);
		}
	}

	TArray<FCCLEquippedSlot> ParsedEquipment;
	const int32 Version = static_cast<int32>(Json->GetNumberField(TEXT("version")));
	if (Version >= 3)
	{
		const TArray<TSharedPtr<FJsonValue>>* Equipment;
		if (!Json->TryGetArrayField(TEXT("equipmentSlots"), Equipment) || Equipment->Num() > 8)
		{
			return false;
		}

		for (const auto& Value : *Equipment)
		{
			if (!Value || Value->Type != EJson::Object)
			{
				return false;
			}

			const auto Object = Value->AsObject();
			const TSharedPtr<FJsonValue> SlotValue = Object->TryGetField(TEXT("slot"));
			FString Name;
			FCCLEquippedSlot Slot;
			if (Version >= 4)
			{
				if (!SlotValue || SlotValue->Type != EJson::String || !Object->TryGetStringField(TEXT("slot"), Name))
				{
					return false;
				}

				Slot.Slot = FGameplayTag::RequestGameplayTag(FName(*Name), false);
			}
			else if (SlotValue && SlotValue->Type == EJson::String && Object->TryGetStringField(TEXT("slot"), Name))
			{
				Slot.Slot = CCLEquipment::SlotAt(static_cast<int32>(StaticEnum<ECCLEquipmentSlot>()->GetValueByNameString(Name)));
			}
			else if (Number(Object, TEXT("slot"), 0, 7))
			{
				Slot.Slot = CCLEquipment::SlotAt(static_cast<int32>(Object->GetNumberField(TEXT("slot"))));
			}

			FString Id;
			if (!CCLEquipment::IsSlot(Slot.Slot) || !Object->TryGetStringField(TEXT("id"), Id) || !FGuid::Parse(Id, Slot.Id))
			{
				return false;
			}

			ParsedEquipment.Add(Slot);
		}
	}

	Json->SetArrayField(TEXT("equipmentSlots"), {});
	if (!FJsonObjectConverter::JsonObjectToUStruct(Json.ToSharedRef(), &Candidate, 0, 0, true))
	{
		return false;
	}

	Candidate.EquipmentSlots = MoveTemp(ParsedEquipment);
	if (!Validate(Candidate))
	{
		return false;
	}

	if (Candidate.Version == 1)
	{
		for (int32 Index = 0; Index < Candidate.Items.Num(); ++Index)
		{
			Candidate.Items[Index].Slot = Index;
		}

		Candidate.Version = 2;
	}

	if (Candidate.Version < 3)
	{
		if (Candidate.Equipped.IsValid())
		{
			auto* Entry = Candidate.Items.FindByPredicate([&](const auto& Value) { return Value.Id == Candidate.Equipped; });
			if (!Entry)
			{
				return false;
			}

			const auto* Equip = Item(Entry->Definition)->FindFragment<FCCLItemFragment_Equip>();
			TArray<FGameplayTag> Slots;
			if (!Equip || !CCLEquipment::GetOccupiedSlots(Item(Entry->Definition), Equip->DefaultSlotTag, Slots))
			{
				return false;
			}

			for (FGameplayTag Tag : Slots)
			{
				FCCLEquippedSlot Slot;
				Slot.Slot = Tag;
				Slot.Id = Entry->Id;
				Candidate.EquipmentSlots.Add(Slot);
			}

			Entry->Slot = INDEX_NONE;
		}
	}

	Candidate.Version = 5;
	if (!Validate(Candidate))
	{
		return false;
	}

	Record = MoveTemp(Candidate);
	return true;
}
