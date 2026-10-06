#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "CCLAbilitySystemComponent.generated.h"

UCLASS()
class CCL_API UCCLAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UCCLAbilitySystemComponent();

public:
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);
	void ReleaseAllInputs();
	FActiveGameplayEffectHandle ApplyEffect(TSubclassOf<UGameplayEffect> Effect, float Magnitude = 0.f, float Duration = -1.f);
};
