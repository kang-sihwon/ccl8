#include "KSIAbilitySystemComponent.h"

#include "Abilities/GameplayAbility.h"

UKSIAbilitySystemComponent::UKSIAbilitySystemComponent()
{
	SetIsReplicatedByDefault(true);
	SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

void UKSIAbilitySystemComponent::AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (!InputTag.IsValid() || !CanProcessAbilityInput(InputTag))
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

void UKSIAbilitySystemComponent::AbilityInputTagReleased(FGameplayTag InputTag)
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

void UKSIAbilitySystemComponent::ReleaseAllInputs()
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

FActiveGameplayEffectHandle UKSIAbilitySystemComponent::ApplyEffectWithMagnitude(TSubclassOf<UGameplayEffect> Effect, FGameplayTag MagnitudeTag, float Magnitude, float Duration)
{
	FGameplayEffectSpecHandle Spec = MakeOutgoingSpec(Effect, 1.f, MakeEffectContext());

	if (!Spec.IsValid())
	{
		return {};
	}

	if (MagnitudeTag.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(MagnitudeTag, Magnitude);
	}

	if (Duration >= 0.f)
	{
		Spec.Data->SetDuration(Duration, true);
	}

	return ApplyGameplayEffectSpecToSelf(*Spec.Data.Get(), ScopedPredictionKey);
}

bool UKSIAbilitySystemComponent::CanProcessAbilityInput(FGameplayTag InputTag) const
{
    return AbilityActorInfo.IsValid() && IsValid(GetAvatarActor());
}
