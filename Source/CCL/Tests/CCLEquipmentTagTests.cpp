#include "Items/CCLItemDefinition.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Items/CCLAttachmentProfile.h"
#include "Session/CCLSessionRecord.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/AutomationTest.h"
#include "Misc/Crc.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCCLEquipmentTagTest, "CCL.Equipment.TagContracts",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCCLEquipmentTagTest::RunTest(const FString& Parameters)
{
	auto* Item = NewObject<UCCLItemDefinition>();
	FCCLItemFragment_Equip Equip;
	Equip.DefaultSlotTag = CCLItemTags::Slot_RightHand;
	Equip.AllowedSlots.AddTag(CCLItemTags::Slot_LeftHand);
	Equip.AllowedSlots.AddTag(CCLItemTags::Slot_RightHand);
	Item->ItemFragments.Add(FInstancedStruct::Make(Equip));
	FCCLItemFragment_MeleeWeapon Weapon;
	Weapon.HandUsage = CCLItemTags::HandUsage_TwoHanded;
	Item->ItemFragments.Add(FInstancedStruct::Make(Weapon));
	TArray<FGameplayTag> Slots;
	TestTrue(TEXT("two-hand occupies both slots"),
			 CCLEquipment::GetOccupiedSlots(Item, CCLItemTags::Slot_LeftHand, Slots) && Slots.Num() == 2);
	TestFalse(TEXT("parent slot is not a concrete slot"),
			  CCLEquipment::GetOccupiedSlots(Item, FGameplayTag::RequestGameplayTag(TEXT("Equipment.Slot.Hand")), Slots));
	TestFalse(TEXT("attachment tag is not an equipment slot"),
			  CCLEquipment::GetOccupiedSlots(Item, CCLItemTags::Attachment_GripRight, Slots));
	Item->ItemFragments[0].GetMutable<FCCLItemFragment_Equip>().AllowedSlots.RemoveTag(CCLItemTags::Slot_LeftHand);
	TestFalse(TEXT("two-hand requires both allowed slots"), CCLEquipment::GetOccupiedSlots(Item, CCLItemTags::Slot_RightHand, Slots));
	Item->ItemFragments[0] = FInstancedStruct::Make(Equip);
	Item->ItemFragments[1].GetMutable<FCCLItemFragment_MeleeWeapon>().HandUsage = CCLItemTags::Slot_Armor;
	TArray<FText> Errors;
	TestFalse(TEXT("wrong hand-usage namespace rejected"), Item->ValidateDefinition(Errors));
	Item->ItemFragments[1] = FInstancedStruct::Make(Weapon);
	Item->ItemFragments.Add(FInstancedStruct::Make(Equip));
	Errors.Reset();
	TestFalse(TEXT("duplicate fragments rejected"), Item->ValidateDefinition(Errors));

	auto* Profile = LoadObject<UCCLAttachmentProfile>(nullptr, TEXT("/Game/Progression/DA_HumanoidAttachments.DA_HumanoidAttachments"));
	if (!TestNotNull(TEXT("generated attachment profile"), Profile))
	{
		return false;
	}

	Errors.Reset();
	TestTrue(TEXT("profile mappings valid"), Profile->ValidateProfile(Errors));
	FString Error;
	FName Socket;
	TestTrue(TEXT("right grip resolves explicit socket"),
			 Profile->Resolve(Profile->ReferenceMesh, CCLItemTags::Attachment_GripRight, Socket, Error) && Socket == TEXT("CCL_Grip_R"));
	TestFalse(TEXT("unmapped parent attachment rejected"),
			  Profile->Resolve(Profile->ReferenceMesh, FGameplayTag::RequestGameplayTag(TEXT("Attachment.Grip")), Socket, Error));
	auto* Broken = DuplicateObject<UCCLAttachmentProfile>(Profile, GetTransientPackage());
	Broken->Bindings[0].Socket = TEXT("hand_l");
	TestFalse(TEXT("a bone name is not an explicit socket"),
			  Broken->Resolve(Broken->ReferenceMesh, Broken->Bindings[0].Point, Socket, Error));
	Broken->Bindings = Profile->Bindings;
	const FCCLAttachmentSocket DuplicateBinding = Broken->Bindings[0];
	Broken->Bindings.Add(DuplicateBinding);
	Errors.Reset();
	TestFalse(TEXT("duplicate mappings rejected"), Broken->ValidateProfile(Errors));
	auto* OtherMesh = NewObject<USkeletalMesh>();
	TestFalse(TEXT("incompatible skeleton rejected"), Profile->Resolve(OtherMesh, CCLItemTags::Attachment_GripRight, Socket, Error));
#if WITH_EDITOR
	TestTrue(TEXT("socket dropdown contains explicit sockets only"),
			 Profile->GetSocketNames().Contains(TEXT("CCL_Grip_R")) && !Profile->GetSocketNames().Contains(TEXT("hand_r")));
#endif
	for (const TCHAR* Name : {TEXT("DA_TrainingSword"), TEXT("DA_TrainingShield"), TEXT("DA_TrainingStaff"), TEXT("DA_TrainingArmor"),
							  TEXT("DA_TrainingBoots"), TEXT("DA_TrainingCloak"), TEXT("DA_TrainingNecklace"), TEXT("DA_TrainingRing"),
							  TEXT("DA_IronGauntlets"), TEXT("DA_RecoveryPotion")})
	{
		TestTrue(FString::Printf(TEXT("item profile validation %s"), Name),
				 Profile->ValidateItem(FCCLSessionCodec::Item(Name), Profile->ReferenceMesh, Error));
	}

	FCCLSessionRecord Record;
	FCCLSavedItem Entry;
	Entry.Id = FGuid::NewGuid();
	Entry.Definition = TEXT("DA_TrainingStaff");
	Entry.Quantity = 1;
	Entry.Slot = INDEX_NONE;
	Record.Items.Add(Entry);
	for (FGameplayTag Hand : {FGameplayTag(CCLItemTags::Slot_LeftHand), FGameplayTag(CCLItemTags::Slot_RightHand)})
	{
		FCCLEquippedSlot Value;
		Value.Slot = Hand;
		Value.Id = Entry.Id;
		Record.EquipmentSlots.Add(Value);
	}

	FCCLSessionRecord Decoded;
	TArray<uint8> Bytes;
	TestTrue(TEXT("v5 two-hand round trip"), FCCLSessionCodec::Encode(Record, Bytes) && FCCLSessionCodec::Decode(Bytes, Decoded) &&
												 Decoded.Version == 5 && Decoded.EquipmentSlots.Num() == 2);
	Record.Version = 3;
	TestTrue(TEXT("v3 numeric slots upgrade"), FCCLSessionCodec::Encode(Record, Bytes) && FCCLSessionCodec::Decode(Bytes, Decoded) &&
												   Decoded.Version == 5 && Decoded.EquipmentSlots[0].Slot == CCLItemTags::Slot_LeftHand);

	auto ChangeWireSlot = [](TArray<uint8>& Data, const FString& Name)
	{
		FUTF8ToTCHAR Text(reinterpret_cast<const ANSICHAR*>(Data.GetData() + 8), Data.Num() - 8);
		TSharedPtr<FJsonObject> Object;
		FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(FString(Text.Length(), Text.Get())), Object);
		Object->GetArrayField(TEXT("equipmentSlots"))[0]->AsObject()->SetStringField(TEXT("slot"), Name);
		FString Json;
		FJsonSerializer::Serialize(Object.ToSharedRef(), TJsonWriterFactory<>::Create(&Json));
		FTCHARToUTF8 Utf8(*Json);
		Data.SetNum(8);
		Data.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
		const uint32 CRC = FCrc::MemCrc32(Data.GetData() + 8, Data.Num() - 8);
		FMemory::Memcpy(Data.GetData() + 4, &CRC, 4);
	};
	ChangeWireSlot(Bytes, TEXT("LeftHand"));
	TestTrue(TEXT("v3 enum name slots upgrade"), FCCLSessionCodec::Decode(Bytes, Decoded) && Decoded.Version == 5);
	Record.Version = 4;
	for (const TCHAR* Invalid : {TEXT("Equipment.Slot.Hand"), TEXT("Equipment.Slot.Unknown"), TEXT("Attachment.Grip.Left")})
	{
		if (!FCCLSessionCodec::Encode(Record, Bytes))
		{
			return false;
		}

		ChangeWireSlot(Bytes, Invalid);
		TestFalse(FString::Printf(TEXT("v4 rejects invalid slot %s"), Invalid), FCCLSessionCodec::Decode(Bytes, Decoded));
	}

	for (int32 Version : {1, 2})
	{
		Record.Version = Version;
		Record.EquipmentSlots.Reset();
		Record.Items[0].Slot = 0;
		Record.Equipped = Entry.Id;
		TestTrue(FString::Printf(TEXT("v%d equipped item upgrade"), Version),
				 FCCLSessionCodec::Encode(Record, Bytes) && FCCLSessionCodec::Decode(Bytes, Decoded) && Decoded.Version == 5 &&
					 Decoded.EquipmentSlots.Num() == 2 && Decoded.Items[0].Slot == INDEX_NONE);
	}

	return true;
}
#endif
