#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActiveGameplayEffectHandle.h"
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
	bool Equip(FGuid Id);
	bool Use(FGuid Id);
	bool Learn(UCCLSkillDefinition* Definition);
	void SyncAvatar(bool bResetHealth = false);
	void GrantPoints(int32 Amount);
	void ShowNotice(const FString& Message);
	UCCLItemDefinition* GetEquippedItem() const;
	FGuid GetEquippedId() const { return EquippedId; }
	int32 GetPoints() const { return Points; }
	bool IsLearned(const UCCLSkillDefinition* Definition) const;
	const TArray<TObjectPtr<UCCLSkillDefinition>>& GetSkills() const { return AvailableSkills; }
	const FString& GetResult() const { return LastResult; }

private:
	void OnInventoryChanged();
	bool Report(bool bSuccess, const TCHAR* Message);
	bool CanAct() const;
	UCCLAbilitySystemComponent* GetASC() const;
	UCCLInventoryComponent* GetInventory() const;

private:
	UPROPERTY(Replicated)
	FGuid EquippedId;

	UPROPERTY(Replicated)
	int32 Points = 1;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<UCCLSkillDefinition>> Learned;

	UPROPERTY()
	TArray<TObjectPtr<UCCLSkillDefinition>> AvailableSkills;

	UPROPERTY(Replicated)
	FString LastResult;

	FActiveGameplayEffectHandle EquipmentEffect;
	TArray<FActiveGameplayEffectHandle> SkillEffects;
};
