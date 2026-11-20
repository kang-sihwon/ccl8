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
enum class ECCLHandAction : uint8
{
	None,
	Attack,
	Guard,
	Parry
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

// 월드에 놓인 아이템과 정적 메시로 장착한 아이템의 표시다.
USTRUCT(BlueprintType)
struct CCL_API FCCLVisualFragment : public FKSIItemFragmentData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TObjectPtr<UStaticMesh> WorldMesh;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLSkeletalMeshFragment : public FCCLVisualFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TObjectPtr<USkeletalMesh> EquippedMesh;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UAnimInstance> AnimClass;
};

// 방어구를 포함한 모든 장비다. 장착 부위, 부위별 부착 위치, 장착 효과를 가진다.
USTRUCT(BlueprintType)
struct CCL_API FCCLEquipmentFragment : public FKSIItemFragmentData
{
	GENERATED_BODY()

	virtual void Validate(const FKSIItemValidation& Context) const override;

	bool Allows(FGameplayTag Slot) const;
	const FCCLItemAttachment* FindAttachment(FGameplayTag Slot) const;

	UPROPERTY(EditAnywhere, meta = (Categories = "Equipment.Slot"))
	FGameplayTagContainer AllowedSlots;

	UPROPERTY(EditAnywhere, meta = (Categories = "Equipment.Slot"))
	FGameplayTag DefaultSlot;

	// 표시가 있는 아이템은 표시되는 부위마다 부착 위치가 하나씩 있어야 한다.
	UPROPERTY(EditAnywhere)
	TArray<FCCLItemAttachment> Attachments;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> Effect;

	UPROPERTY(EditAnywhere)
	float Magnitude = 0.f;
};

// 손 행동의 임시 자료다. 결정 69의 무브셋(재설계 단계 3)이 대체한다.
USTRUCT(BlueprintType)
struct CCL_API FCCLHandActions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TObjectPtr<UCCLCombatDefinition> Combat;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UCCLAbilitySet> Abilities;

	UPROPERTY(EditAnywhere)
	TObjectPtr<UKSIActionSet> ActionSet;

	UPROPERTY(EditAnywhere)
	TMap<FGameplayTag, FGameplayTag> HandActions;

	UPROPERTY(EditAnywhere)
	ECCLHandAction LeftAction = ECCLHandAction::Attack;

	UPROPERTY(EditAnywhere)
	ECCLHandAction RightAction = ECCLHandAction::Attack;
};

// 손을 점유하는 장비다. 방패는 여기까지, 무기는 Weapon 아래를 쓴다.
USTRUCT(BlueprintType)
struct CCL_API FCCLHandEquipmentFragment : public FCCLEquipmentFragment
{
	GENERATED_BODY()

	virtual void Validate(const FKSIItemValidation& Context) const override;

	bool IsTwoHanded() const { return HandUsage == CCLItemTags::HandUsage_TwoHanded; }

	UPROPERTY(EditAnywhere, meta = (Categories = "Weapon.HandUsage"))
	FGameplayTag HandUsage = CCLItemTags::HandUsage_OneHanded;

	UPROPERTY(EditAnywhere)
	FCCLHandActions Actions;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLWeaponFragment : public FCCLHandEquipmentFragment
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct CCL_API FCCLMeleeWeaponFragment : public FCCLWeaponFragment
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct CCL_API FCCLRangedWeaponFragment : public FCCLWeaponFragment
{
	GENERATED_BODY()

	virtual void Validate(const FKSIItemValidation& Context) const override;

	// 투사체, 속도, 장전 시간, 탄창 크기의 유일한 소유자다.
	UPROPERTY(EditAnywhere)
	TObjectPtr<UCCLProjectileProfile> Profile;
};

// 회복 효과를 바로 적용하는 사용 아이템이다. 재설계 단계 4에서 Usable로 바뀐다.
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
struct CCL_API FCCLPerishableFragment : public FKSIItemFragmentData
{
	GENERATED_BODY()

	virtual void Validate(const FKSIItemValidation& Context) const override;

	UPROPERTY(EditAnywhere)
	FCCLFoodPolicy Policy;
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
