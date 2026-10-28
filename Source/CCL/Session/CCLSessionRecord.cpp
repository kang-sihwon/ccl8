#include "CCLSessionRecord.h"

#include "Items/CCLItemCatalog.h"
#include "Items/CCLItemDefinition.h"
#include "Items/CCLSkillDefinition.h"
#include "Actions/CCLWeaponAbility.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/Crc.h"
#include "Misc/Base64.h"
#include "Agents/CCLLifeSimulation.h"
#include "Environment/CCLWorldSnapshot.h"
#include "Items/CCLInventorySnapshot.h"
#include "CCLPlayerSnapshot.h"

UKSIItemDefinition* FCCLSessionCodec::Item(FName Id)
{
	const auto* Catalog = UCCLItemCatalog::Get();
	return Catalog ? Catalog->FindItem(Id) : nullptr;
}

UCCLSkillDefinition* FCCLSessionCodec::Skill(FName Id)
{
	const auto* Catalog = UCCLItemCatalog::Get();
	return Catalog ? Catalog->FindSkill(Id) : nullptr;
}

namespace
{
// Format budgets, not content rules. The campaign director validates the guard mask against its road.
constexpr int32 MaxRemainingSupplies = 64;
constexpr int32 MaxSkills = 16;

bool ValidateRecord(const FCCLSessionRecord& R, bool bProfile, FGuid ProfileWorld, double ProfileTime, int32 Capacity)
{
	if (bProfile && ((R.Version != 7 && R.Version != 8) || !R.AccountId.IsValid() || !R.AgentSimulation.IsEmpty() || R.Coins != 0 ||
		R.Equipped.IsValid() || R.Victory || R.DefeatedGuards || !R.RemainingSupplies.IsEmpty() || !R.SupplyPlacements.IsEmpty() ||
		!ProfileWorld.IsValid() || !FMath::IsFinite(ProfileTime) || ProfileTime < 0. || Capacity < 1 || Capacity > 128))
	{
		return false;
	}
	if (R.Version < 1 || R.Version > 8 || R.Items.Num() > (bProfile ? Capacity + 8 : (R.Version >= 3 ? 24 : 16)) || R.Skills.Num() > MaxSkills || R.Points < 0 ||
		R.Points > 1000 || R.Coins < 0 || R.Coins > 1000000 || R.Quest > 2 || R.Victory > 1 ||
		(R.Victory && R.DefeatedGuards == 0) || (!bProfile && R.Quest == 2 && !R.Victory) ||
		R.RemainingSupplies.Num() > (R.Version >= 5 ? MaxRemainingSupplies : 2))
	{
		return false;
	}

	if (!bProfile && R.AccountId.IsValid() && (R.AgentSimulation.IsEmpty() || R.Coins != 0))
	{
		return false;
	}

	TSet<FGuid> WorldItemIds;
	FGuid FoodWorldId = ProfileWorld;
	double FoodWorldSeconds = ProfileTime;
	if (!R.AgentSimulation.IsEmpty())
	{
		TArray<uint8> State;
		FCCLLifeSimulation Validation;
		FString Error;
		if (R.Version < 5 || R.AgentSimulation.Len() > 48 * 1024 * 1024 || !FBase64::Decode(R.AgentSimulation, State))
		{
			return false;
		}

		if (R.Version >= 6)
		{
			FCCLWorldSnapshot World;
			if (!FCCLWorldSnapshotCodec::Decode(State, World, Error) || World.Identity.Domain != ECCLWorldDomain::Campaign)
			{
				return false;
			}

			TArray<FCCLInventorySnapshot> Containers;
			TArray<FCCLPlayerSnapshot> Profiles;
			if (!CCLInventorySnapshot::ReadWorld(World, Containers, Error) || !CCLPlayerSnapshot::ReadWorld(World, Profiles, Error))
			{
				return false;
			}

			for (const auto& Container : Containers)
			{
				for (const auto& Stored : Container.Items)
				{
					WorldItemIds.Add(Stored.Id);
				}
			}
			for (const auto& Profile : Profiles)
			{
				if (Profile.AccountId == R.AccountId)
				{
					return false;
				}
				for (const auto& Stored : Profile.Items)
				{
					WorldItemIds.Add(Stored.Id);
				}
			}
			FoodWorldId = World.Identity.WorldId;
			FoodWorldSeconds = World.Clock.WorldSeconds;
			State = MoveTemp(World.Life);
		}

		if (!Validation.Load(State, Error) ||
			(R.AccountId.IsValid() && !Validation.GetEconomy().Accounts.Contains(R.AccountId)))
		{
			return false;
		}
	}

	TSet<FGuid> Ids;
	TSet<int32> BagSlots;
	bool bLegacyEquipmentFound = !R.Equipped.IsValid();
	for (const auto& Entry : R.Items)
	{
		const auto* Definition = FCCLSessionCodec::Item(Entry.Definition);
		if (!Entry.Id.IsValid() || Ids.Contains(Entry.Id) || WorldItemIds.Contains(Entry.Id) || !Definition || Entry.Quantity <= 0 ||
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

		FCCLFoodState Food;
		FString FoodError;
		if (!CCLFood::Decode(Entry.Food, Food, FoodError)
			|| !CCLFood::Validate(Food, Food.IsPresent() ? Entry.Quantity : 0, FoodError)
			|| (Food.IsPresent() && (R.Version < 7 || !Definition->FindFragment<FCCLItemFragment_Perishable>()
				|| Food.WorldId != FoodWorldId || Food.UpdatedWorldSeconds > FoodWorldSeconds)))
		{
			return false;
		}

		Ids.Add(Entry.Id);
		if (R.Version >= 2)
		{
			if (Entry.Slot < (R.Version >= 3 ? -1 : 0) || Entry.Slot >= Capacity || (Entry.Slot >= 0 && BagSlots.Contains(Entry.Slot)))
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
			const auto* Definition = Entry ? FCCLSessionCodec::Item(Entry->Definition) : nullptr;
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
			if (!Slot || !CCLEquipment::GetOccupiedSlots(FCCLSessionCodec::Item(Entry.Definition), Slot->Slot, ExpectedSlots) ||
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
		if (!FCCLSessionCodec::Skill(Id) || Skills.Contains(Id))
		{
			return false;
		}

		Skills.Add(Id);
	}

	if ((R.Version < 8 && !R.SupplyPlacements.IsEmpty()) ||
		(R.Version >= 8 && !R.RemainingSupplies.IsEmpty()) || R.SupplyPlacements.Num() > MaxRemainingSupplies)
	{
		return false;
	}

	TSet<FGuid> Placements;
	for (const auto& Supply : R.SupplyPlacements)
	{
		const auto* Definition = FCCLSessionCodec::Item(Supply.Definition);
		if (!Supply.PlacementId.IsValid() || Placements.Contains(Supply.PlacementId) || !Definition ||
			Supply.Quantity < 1 || Supply.Quantity > 1000000 || Definition->FindFragment<FCCLItemFragment_Perishable>())
		{
			return false;
		}

		Placements.Add(Supply.PlacementId);
	}

	// Legacy records preserve names until the campaign can resolve their placements.
	for (FName Id : R.RemainingSupplies)
	{
		if (!FCCLSessionCodec::Item(Id))
		{
			return false;
		}
	}

	return true;
}
} // namespace

bool FCCLSessionCodec::Validate(const FCCLSessionRecord& Record)
{
	return ValidateRecord(Record, false, {}, 0., 16);
}

bool FCCLSessionCodec::ValidateProfile(const FCCLSessionRecord& Record, FGuid WorldId, double WorldSeconds, int32 Capacity)
{
	return ValidateRecord(Record, true, WorldId, WorldSeconds, Capacity);
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
	if (Utf8.Length() > 64 * 1024 * 1024 - 8)
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
	if (Bytes.Num() < 12 || Bytes.Num() > 64 * 1024 * 1024 || FMemory::Memcmp(Bytes.GetData(), "CCL1", 4) != 0)
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
	if (!Number(Json, TEXT("version"), 1, 8) || !Number(Json, TEXT("points"), 0, 1000) || !Number(Json, TEXT("coins"), 0, 1000000) ||
		!Number(Json, TEXT("quest"), 0, 2) || !Number(Json, TEXT("defeatedGuards"), 0, 255) || !Number(Json, TEXT("victory"), 0, 1))
	{
		return false;
	}

	if (Json->GetNumberField(TEXT("version")) >= 8)
	{
		const TArray<TSharedPtr<FJsonValue>>* Supplies;
		if (!Json->TryGetArrayField(TEXT("supplyPlacements"), Supplies) || Supplies->Num() > MaxRemainingSupplies)
		{
			return false;
		}

		for (const auto& Value : *Supplies)
		{
			if (!Value || Value->Type != EJson::Object || !Number(Value->AsObject(), TEXT("quantity"), 1, 1000000))
			{
				return false;
			}
		}
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
		if (Json->GetNumberField(TEXT("version")) < 7)
		{
			FString UnexpectedFood;
			if (Value->AsObject()->HasField(TEXT("food"))
				&& (!Value->AsObject()->TryGetStringField(TEXT("food"), UnexpectedFood) || !UnexpectedFood.IsEmpty()))
			{
				return false;
			}

			Value->AsObject()->SetStringField(TEXT("food"), TEXT(""));
		}

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

	if (!Json->HasField(TEXT("agentSimulation")))
	{
		Json->SetStringField(TEXT("agentSimulation"), TEXT(""));
	}

	if (!Json->HasField(TEXT("accountId")))
	{
		Json->SetStringField(TEXT("accountId"), FGuid().ToString());
	}

	if (Version < 8 && !Json->HasField(TEXT("supplyPlacements")))
	{
		Json->SetArrayField(TEXT("supplyPlacements"), {});
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

	if (Candidate.Version < 6 && !Candidate.AgentSimulation.IsEmpty())
	{
		TArray<uint8> Legacy;
		TArray<uint8> Upgraded;
		FCCLWorldSnapshot Snapshot;
		FString Error;
		if (!FBase64::Decode(Candidate.AgentSimulation, Legacy) ||
			!FCCLWorldSnapshotCodec::MigrateLegacy(Legacy, ECCLWorldDomain::Campaign, Snapshot, Error) ||
			!FCCLWorldSnapshotCodec::Encode(Snapshot, Upgraded, Error))
		{
			return false;
		}

		Candidate.AgentSimulation = FBase64::Encode(Upgraded);
	}

	Candidate.Version = FMath::Max(Candidate.Version, 7);
	if (!Validate(Candidate))
	{
		return false;
	}

	Record = MoveTemp(Candidate);
	return true;
}
