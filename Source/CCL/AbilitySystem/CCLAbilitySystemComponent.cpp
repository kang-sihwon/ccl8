#include "CCLAbilitySystemComponent.h"

#include "CCLEffects.h"
#include "CCLGameplayTags.h"
#include "CCLHealthSet.h"
#include "CCLStaminaSet.h"
#include "Abilities/GameplayAbility.h"

UCCLAbilitySystemComponent::UCCLAbilitySystemComponent()
{
	SetIsReplicatedByDefault(true);
	SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

void UCCLAbilitySystemComponent::AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (!AbilityActorInfo.IsValid() || !GetAvatarActor() || HasMatchingGameplayTag(CCLTags::State_Dead))
	{
		return;
	}

	ABILITYLIST_SCOPE_LOCK();

	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (!Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}

		Spec.InputPressed = true;

		if (!Spec.IsActive())
		{
			TryActivateAbility(Spec.Handle);
		}
	}
}

void UCCLAbilitySystemComponent::AbilityInputTagReleased(FGameplayTag InputTag)
{
	ABILITYLIST_SCOPE_LOCK();

	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (!Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}

		Spec.InputPressed = false;

		if (Spec.IsActive())
		{
			const UGameplayAbility* Instance = Spec.GetPrimaryInstance();
			const FPredictionKey Key = Instance ? Instance->GetCurrentActivationInfo().GetActivationPredictionKey() : FPredictionKey();
			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, Key);
		}
	}
}

void UCCLAbilitySystemComponent::ReleaseAllInputs()
{
	TArray<FGameplayTag> Tags;

	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		for (FGameplayTag Tag : Spec.GetDynamicSpecSourceTags())
		{
			Tags.AddUnique(Tag);
		}
	}

	for (FGameplayTag Tag : Tags)
	{
		AbilityInputTagReleased(Tag);
	}
}

FActiveGameplayEffectHandle UCCLAbilitySystemComponent::ApplyEffect(TSubclassOf<UGameplayEffect> Effect, float Magnitude, float Duration)
{
	FGameplayEffectSpecHandle Spec = MakeOutgoingSpec(Effect, 1.f, MakeEffectContext());

	if (!Spec.IsValid())
	{
		return {};
	}

	Spec.Data->SetSetByCallerMagnitude(CCLTags::Data_Magnitude, Magnitude);

	if (Duration >= 0.f)
	{
		Spec.Data->SetDuration(Duration, true);
	}

	return ApplyGameplayEffectSpecToSelf(*Spec.Data.Get(), ScopedPredictionKey);
}
