#pragma once

#include "CoreMinimal.h"
#include "KSIAbilitySystemComponent.h"
#include "CCLAbilitySystemComponent.generated.h"

UCLASS()
class CCL_API UCCLAbilitySystemComponent : public UKSIAbilitySystemComponent
{
    GENERATED_BODY()

protected:
    virtual bool CanProcessAbilityInput(FGameplayTag InputTag) const override;

public:
    FActiveGameplayEffectHandle ApplyEffect(TSubclassOf<UGameplayEffect> Effect, float Magnitude = 0.f, float Duration = -1.f);
};
