#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Combat/CCLFighterComponent.h"
#include "CCLCombatAbility.generated.h"

UCLASS(Abstract)
class CCL_API UCCLCombatAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UCCLCombatAbility();
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	float GetCost(const FGameplayAbilityActorInfo* ActorInfo) const;
	bool HasCurrentAvatar() const;

	UFUNCTION()
	void BeginWindow();

	UFUNCTION()
	void EndWindow();

	UFUNCTION()
	void Finish();

	UFUNCTION()
	void Released(float TimeHeld);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Action")
	ECCLCombatAction Action = ECCLCombatAction::None;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCCLFighterComponent> Fighter;

	TWeakObjectPtr<AActor> ActivatedAvatar;
	FActiveGameplayEffectHandle BusyEffect;
	FActiveGameplayEffectHandle WindowEffect;
};

UCLASS()
class CCL_API UCCLAttackAbility : public UCCLCombatAbility
{
	GENERATED_BODY()

public:
	UCCLAttackAbility();
};

UCLASS()
class CCL_API UCCLDodgeAbility : public UCCLCombatAbility
{
	GENERATED_BODY()

public:
	UCCLDodgeAbility();
};

UCLASS()
class CCL_API UCCLGuardAbility : public UCCLCombatAbility
{
	GENERATED_BODY()

public:
	UCCLGuardAbility();
};

UCLASS()
class CCL_API UCCLParryAbility : public UCCLCombatAbility
{
	GENERATED_BODY()

public:
	UCCLParryAbility();
};
