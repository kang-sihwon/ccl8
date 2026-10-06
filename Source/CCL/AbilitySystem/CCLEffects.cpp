#include "CCLEffects.h"

#include "CCLGameplayTags.h"
#include "CCLHealthSet.h"
#include "CCLStaminaSet.h"
#include "CCLOffenseSet.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"

namespace
{
	void GrantTags(UTargetTagsGameplayEffectComponent& Component, FGameplayTag Tag)
	{
		FInheritedTagContainer Tags;
		Tags.AddTag(CCLTags::Effect_Life);

		if (Tag.IsValid())
		{
			Tags.AddTag(Tag);
		}

		Component.SetAndApplyTargetTagChanges(Tags);
	}

	void SetModifier(UGameplayEffect& Effect, FGameplayAttribute Attribute)
	{
		FSetByCallerFloat Magnitude;
		Magnitude.DataTag = CCLTags::Data_Magnitude;
		FGameplayModifierInfo Modifier;
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Magnitude);
		Effect.Modifiers.Add(Modifier);
	}
}

UCCLHealthChangeEffect::UCCLHealthChangeEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	SetModifier(*this, UCCLHealthSet::GetHealthAttribute());
}

UCCLPersistentPowerEffect::UCCLPersistentPowerEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	SetModifier(*this, UCCLOffenseSet::GetAttackBonusAttribute());
}

UCCLPersistentVitalityEffect::UCCLPersistentVitalityEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	SetModifier(*this, UCCLHealthSet::GetMaxHealthAttribute());
}

UCCLStaminaChangeEffect::UCCLStaminaChangeEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	SetModifier(*this, UCCLStaminaSet::GetStaminaAttribute());
}

UCCLBusyEffect::UCCLBusyEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTags);
	GrantTags(*TargetTags, CCLTags::State_Busy);
}

UCCLGuardEffect::UCCLGuardEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTags);
	GrantTags(*TargetTags, CCLTags::State_Guard);
}

UCCLParryEffect::UCCLParryEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTags);
	GrantTags(*TargetTags, CCLTags::State_Parry);
}

UCCLInvulnerableEffect::UCCLInvulnerableEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTags);
	GrantTags(*TargetTags, CCLTags::State_Invulnerable);
}

UCCLStaggerEffect::UCCLStaggerEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(1.f);
	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTags);
	GrantTags(*TargetTags, CCLTags::State_Stagger);
}

UCCLDeadEffect::UCCLDeadEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTags);
	GrantTags(*TargetTags, CCLTags::State_Dead);
}

UCCLRecoveryDelayEffect::UCCLRecoveryDelayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(1.f);
	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTags);
	GrantTags(*TargetTags, CCLTags::State_RecoveryDelay);
}

UCCLStaminaRegenEffect::UCCLStaminaRegenEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = 0.1f;
	bExecutePeriodicEffectOnApplication = false;
	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTags);
	GrantTags(*TargetTags, FGameplayTag());
	SetModifier(*this, UCCLStaminaSet::GetStaminaAttribute());
	UTargetTagRequirementsGameplayEffectComponent* Requirements = CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("TargetTagRequirements"));
	GEComponents.Add(Requirements);
	Requirements->OngoingTagRequirements.IgnoreTags.AddTag(CCLTags::State_Dead);
	Requirements->OngoingTagRequirements.IgnoreTags.AddTag(CCLTags::State_Guard);
	Requirements->OngoingTagRequirements.IgnoreTags.AddTag(CCLTags::State_Stagger);
	Requirements->OngoingTagRequirements.IgnoreTags.AddTag(CCLTags::State_RecoveryDelay);
}
