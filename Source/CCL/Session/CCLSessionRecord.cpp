#include "CCLSessionRecord.h"
#include "Items/CCLItemDefinition.h"
#include "Items/CCLSkillDefinition.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/Crc.h"
UCCLItemDefinition* FCCLSessionCodec::Item(FName Id)
{
	if (Id != TEXT("DA_IronGauntlets") && Id != TEXT("DA_RecoveryPotion")) { return nullptr; }
	return LoadObject<UCCLItemDefinition>(nullptr, *FString::Printf(TEXT("/Game/Progression/%s.%s"), *Id.ToString(), *Id.ToString()));
}
UCCLSkillDefinition* FCCLSessionCodec::Skill(FName Id)
{
	if (Id != TEXT("DA_PowerTraining") && Id != TEXT("DA_VitalityTraining")) { return nullptr; }
	return LoadObject<UCCLSkillDefinition>(nullptr, *FString::Printf(TEXT("/Game/Progression/%s.%s"), *Id.ToString(), *Id.ToString()));
}
bool FCCLSessionCodec::Validate(const FCCLSessionRecord& R)
{
	if (R.Version != 1 || R.Items.Num() > 16 || R.Skills.Num() > 2 || R.Points < 0 || R.Points > 1000 || R.Coins < 0 || R.Coins > 1000000 ||
		R.Quest > 2 || R.DefeatedGuards > 3 || R.Victory > 1 || (R.Victory && R.DefeatedGuards != 3) || (R.Quest == 2 && !R.Victory) || R.RemainingSupplies.Num() > 2) { return false; }
	TSet<FGuid> Ids;
	bool bEquipmentFound = !R.Equipped.IsValid();
	for (const auto& Entry : R.Items)
	{
		const auto* Definition = Item(Entry.Definition);
		if (!Entry.Id.IsValid() || Ids.Contains(Entry.Id) || !Definition || Entry.Quantity <= 0 || Entry.Quantity > Definition->GetMaxStack()) { return false; }
		Ids.Add(Entry.Id);
		if (R.Equipped == Entry.Id) { bEquipmentFound = Definition->FindFragment(UCCLItemFragment_Equipment::StaticClass()) != nullptr; }
	}
	TSet<FName> Skills;
	for (FName Id : R.Skills) { if (!Skill(Id) || Skills.Contains(Id)) { return false; } Skills.Add(Id); }
	TSet<FName> Supplies;
	for (FName Id : R.RemainingSupplies) { if (!Item(Id) || Supplies.Contains(Id)) { return false; } Supplies.Add(Id); }
	return bEquipmentFound;
}
bool FCCLSessionCodec::Encode(const FCCLSessionRecord& Record, TArray<uint8>& Bytes)
{
	FString Json;
	if (!Validate(Record) || !FJsonObjectConverter::UStructToJsonObjectString(Record, Json)) { return false; }
	FTCHARToUTF8 Utf8(*Json);
	if (Utf8.Length() > 65528) { return false; }
	Bytes.Reset();
	Bytes.Append(reinterpret_cast<const uint8*>("CCL1"), 4);
	const uint32 Checksum = FCrc::MemCrc32(Utf8.Get(), Utf8.Length());
	Bytes.Append(reinterpret_cast<const uint8*>(&Checksum), 4);
	Bytes.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
	return true;
}
bool FCCLSessionCodec::Decode(const TArray<uint8>& Bytes, FCCLSessionRecord& Record)
{
	if (Bytes.Num() < 12 || Bytes.Num() > 65536 || FMemory::Memcmp(Bytes.GetData(), "CCL1", 4) != 0) { return false; }
	uint32 Expected;
	FMemory::Memcpy(&Expected, Bytes.GetData() + 4, 4);
	if (Expected != FCrc::MemCrc32(Bytes.GetData() + 8, Bytes.Num() - 8)) { return false; }
	FUTF8ToTCHAR Text(reinterpret_cast<const ANSICHAR*>(Bytes.GetData() + 8), Bytes.Num() - 8);
	FCCLSessionRecord Candidate;
		TSharedPtr<FJsonObject> Json;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(FString(Text.Length(), Text.Get())), Json) || !Json) { return false; }
	auto Number = [](const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, double Min, double Max)
	{
		double Value;
		return Object && Object->TryGetNumberField(Key, Value) && FMath::IsFinite(Value) && Value >= Min && Value <= Max && FMath::FloorToDouble(Value) == Value;
	};
	if (!Number(Json, TEXT("version"), 1, 1) || !Number(Json, TEXT("points"), 0, 1000) || !Number(Json, TEXT("coins"), 0, 1000000) ||
		!Number(Json, TEXT("quest"), 0, 2) || !Number(Json, TEXT("defeatedGuards"), 0, 3) || !Number(Json, TEXT("victory"), 0, 1)) { return false; }
	const TArray<TSharedPtr<FJsonValue>>* Items;
	if (!Json->TryGetArrayField(TEXT("items"), Items) || Items->Num() > 16) { return false; }
	for (const auto& Value : *Items)
	{
		if (!Value || Value->Type != EJson::Object || !Number(Value->AsObject(), TEXT("quantity"), 1, 1000)) { return false; }
	}
	if (!FJsonObjectConverter::JsonObjectToUStruct(Json.ToSharedRef(), &Candidate, 0, 0, true) || !Validate(Candidate)) { return false; }
	Record = MoveTemp(Candidate);
	return true;
}
