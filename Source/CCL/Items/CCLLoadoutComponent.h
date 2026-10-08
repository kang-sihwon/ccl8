#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActiveGameplayEffectHandle.h"
#include "CCLItemFragments.h"
#include "GameplayTagContainer.h"
#include "CCLLoadoutComponent.generated.h"

class UCCLInventoryComponent;
class UCCLAbilitySystemComponent;
class UCCLItemDefinition;
class UCCLSkillDefinition;

UCLASS(ClassGroup = Progression, meta = (BlueprintSpawnableComponent))
class CCL_API UCCLLoadoutComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCCLLoadoutComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	bool Restore(const TArray<FCCLEquippedSlot>& Equipment, const TArray<UCCLSkillDefinition*>& Skills, int32 UnspentPoints);
	bool Equip(FGuid Id, FGameplayTag Slot = FGameplayTag());
	bool Unequip(FGuid Id, int32 BagSlot = INDEX_NONE);
	bool Use(FGuid Id);
	bool Learn(UCCLSkillDefinition* Definition);
	void SyncAvatar(bool bResetHealth = false);
	void GrantPoints(int32 Amount);
	void ShowNotice(const FString& Message);
	FGameplayTag PrepareHandAction(FGameplayTag Hand);
	bool RequestCustomHandAction(FGameplayTag Hand, bool bPressed);
	void RequestReload();
	UCCLItemDefinition* GetEquippedItem(FGameplayTag Slot = CCLItemTags::Slot_RightHand) const;
	FGuid GetEquippedId(FGameplayTag Slot = CCLItemTags::Slot_RightHand) const;
	bool IsEquipped(FGuid Id) const;
	const TArray<FCCLEquippedSlot>& GetEquipment() const { return EquipmentSlots; }
	ECCLHandAction GetHandAction(FGameplayTag Hand) const;
	int32 GetPoints() const { return Points; }
	bool IsLearned(const UCCLSkillDefinition* Definition) const;
	const TArray<TObjectPtr<UCCLSkillDefinition>>& GetSkills() const { return AvailableSkills; }
	const FString& GetResult() const { return LastResult; }

private:
	UFUNCTION(Server, Reliable)
	void ServerCustomHandAction(FGameplayTag Hand, bool bPressed);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	bool ExecuteCustom(FGuid Id, FGameplayTag Action, bool bPressed);

	UFUNCTION(Server, Reliable)
	void ServerPrepareHandAction(FGameplayTag Hand);
	void OnInventoryChanged();
	bool Report(bool bSuccess, const TCHAR* Message);
	bool CanAct() const;
	UCCLAbilitySystemComponent* GetASC() const;
	UCCLInventoryComponent* GetInventory() const;

private:
	UPROPERTY(Replicated)
	TArray<FCCLEquippedSlot> EquipmentSlots;

	UPROPERTY(Replicated)
	int32 Points = 1;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<UCCLSkillDefinition>> Learned;

	UPROPERTY()
	TArray<TObjectPtr<UCCLSkillDefinition>> AvailableSkills;

	UPROPERTY(Replicated)
	FString LastResult;

	TMap<FGuid, FActiveGameplayEffectHandle> EquipmentEffects;
	TArray<FActiveGameplayEffectHandle> SkillEffects;
	TSet<FGuid> ActionSources;
};
