#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "KSIAbilitySystemComponent.generated.h"

UCLASS()
class KSICOMBAT_API UKSIAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UKSIAbilitySystemComponent();

public:
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);
	void ReleaseAllInputs();
	FActiveGameplayEffectHandle ApplyEffectWithMagnitude(TSubclassOf<UGameplayEffect> Effect, FGameplayTag MagnitudeTag, float Magnitude = 0.f, float Duration = -1.f);
protected:
	virtual bool CanProcessAbilityInput(FGameplayTag InputTag) const;
};
