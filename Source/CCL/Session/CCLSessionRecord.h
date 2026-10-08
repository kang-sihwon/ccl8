#pragma once
#include "CoreMinimal.h"
#include "Items/CCLItemFragments.h"
#include "CCLSessionRecord.generated.h"
class UCCLItemDefinition;
class UCCLSkillDefinition;
USTRUCT()
struct FCCLSavedItem
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid Id;

	UPROPERTY()
	FName Definition;

	UPROPERTY()
	int32 Quantity = 0;

	UPROPERTY()
	int32 Slot = INDEX_NONE;

	UPROPERTY()
	int32 LoadedAmmo = 0;
};
USTRUCT()
struct FCCLSessionRecord
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Version = 5;

	UPROPERTY()
	FString AgentSimulation;

	UPROPERTY()
	FGuid AccountId;

	UPROPERTY()
	TArray<FCCLSavedItem> Items;

	UPROPERTY()
	FGuid Equipped;

	UPROPERTY()
	TArray<FCCLEquippedSlot> EquipmentSlots;

	UPROPERTY()
	TArray<FName> Skills;

	UPROPERTY()
	int32 Points = 1;

	UPROPERTY()
	int32 Coins = 30;

	UPROPERTY()
	uint8 Quest = 0;

	UPROPERTY()
	uint8 DefeatedGuards = 0;

	UPROPERTY()
	uint8 Victory = 0;

	UPROPERTY()
	TArray<FName> RemainingSupplies;
};
class CCL_API FCCLSessionCodec
{
public:
	static UCCLItemDefinition* Item(FName Id);
	static UCCLSkillDefinition* Skill(FName Id);
	static bool Validate(const FCCLSessionRecord& Record);
	static bool Encode(const FCCLSessionRecord& Record, TArray<uint8>& Bytes);
	static bool Decode(const TArray<uint8>& Bytes, FCCLSessionRecord& Record);
};
