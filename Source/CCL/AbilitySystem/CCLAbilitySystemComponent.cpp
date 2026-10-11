#include "CCLAbilitySystemComponent.h"

#include "CCLGameplayTags.h"

bool UCCLAbilitySystemComponent::CanProcessAbilityInput(FGameplayTag InputTag) const
{
    return Super::CanProcessAbilityInput(InputTag) && !HasMatchingGameplayTag(CCLTags::State_Dead);
}

FActiveGameplayEffectHandle UCCLAbilitySystemComponent::ApplyEffect(TSubclassOf<UGameplayEffect> Effect, float Magnitude, float Duration)
{
    return ApplyEffectWithMagnitude(Effect, CCLTags::Data_Magnitude, Magnitude, Duration);
}
