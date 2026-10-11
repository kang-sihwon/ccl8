#pragma once

#include "CoreMinimal.h"
#include "KSIItemDefinition.h"
#include "CCLItemTags.h"
#include "CCLFoodState.h"
#include "CCLItemFragments.generated.h"

class UKSIActionSet;
class UCCLProjectileProfile;

class UCCLCombatDefinition;
class UCCLAbilitySet;
class UGameplayEffect;
class UStaticMesh;
class USkeletalMesh;
class UAnimInstance;

UENUM(BlueprintType)
enum class ECCLEquipmentSlot : uint8
{
	LeftHand,
	RightHand,
	Armor,
	Boots,
	Cloak,
	Necklace,
	RingOne,
	RingTwo,
	Count
};

UENUM(BlueprintType)
enum class ECCLHandAction : uint8
{
	None,
	Attack,
	Guard,
	Parry
};


USTRUCT(BlueprintType)
struct CCL_API FCCLItemFragment_Equip : public FKSIItemFragmentData
{
	GENERATED_BODY()

	bool Allows(FGameplayTag Slot) const;

	UPROPERTY(EditAnywhere, meta = (Categories = "Equipment.Slot"))
	FGameplayTagContainer AllowedSlots;

	UPROPERTY(EditAnywhere, meta = (Categories = "Equipment.Slot"))
	FGameplayTag DefaultSlotTag;

	// Serialized legacy fields, hidden from new authoring.
	UPROPERTY()
	ECCLEquipmentSlot DefaultSlot = ECCLEquipmentSlot::RightHand;

	UPROPERTY()
	uint8 bTwoHanded = 0;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> Effect;

	UPROPERTY(EditAnywhere)
	float Magnitude = 0.f;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLItemFragment_ConsumableData : public FKSIItemFragmentData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> Effect;

	UPROPERTY(EditAnywhere)
	float Magnitude = 50.f;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLItemAttachment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (Categories = "Equipment.Slot"))
	FGameplayTag Slot;

	UPROPERTY(EditAnywhere, meta = (Categories = "Attachment"))
	FGameplayTag Point;

	UPROPERTY(EditAnywhere)
	FTransform Offset;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLItemFragment_Visual : public FKSIItemFragmentData
{
	GENERATED_BODY()

	const FCCLItemAttachment* FindAttachment(FGameplayTag Slot) const;

	UPROPERTY(EditAnywhere)
	TArray<FCCLItemAttachment> Attachments;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UStaticMesh> DroppedMesh;

	// Read only during migration of the previous struct layout.
	UPROPERTY()
	FName LeftSocket = TEXT("hand_l");

	UPROPERTY()
	FName RightSocket = TEXT("hand_r");

	UPROPERTY()
	FTransform EquippedTransform;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLItemFragment_SkeletalVisual : public FCCLItemFragment_Visual
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TObjectPtr<USkeletalMesh> EquippedMesh;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UAnimInstance> AnimClass;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLItemFragment_Weapon : public FKSIItemFragmentData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TObjectPtr<UKSIActionSet> ActionSet;

	UPROPERTY(EditAnywhere)
	TMap<FGameplayTag, FGameplayTag> HandActions;

	UPROPERTY(EditAnywhere, meta = (Categories = "Weapon.HandUsage"))
	FGameplayTag HandUsage = CCLItemTags::HandUsage_OneHanded;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UCCLCombatDefinition> Combat;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UCCLAbilitySet> Abilities;

	UPROPERTY(EditAnywhere)
	ECCLHandAction LeftAction = ECCLHandAction::Attack;

	UPROPERTY(EditAnywhere)
	ECCLHandAction RightAction = ECCLHandAction::Attack;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLItemFragment_MeleeWeapon : public FCCLItemFragment_Weapon
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct CCL_API FCCLItemFragment_ProjectileWeapon : public FCCLItemFragment_Weapon
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TObjectPtr<UCCLProjectileProfile> Profile;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> ProjectileClass;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float LaunchSpeed = 2000.f;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "0"))
	float ReloadSeconds = 2.f;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLItemFragment_HarvestTool : public FKSIItemFragmentData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FName ResourceType;

	UPROPERTY(EditAnywhere, meta = (ClampMin = "1"))
	int32 Tier = 1;
};

USTRUCT()
struct CCL_API FCCLEquippedSlot
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Slot;

	UPROPERTY()
	FGuid Id;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLItemFragment_Perishable : public FKSIItemFragmentData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FCCLFoodPolicy Policy;
};
