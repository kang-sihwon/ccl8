#include "CCLAbilitySet.h"

#include "CCLAbilitySystemComponent.h"

void UCCLAbilitySet::GrantTo(UCCLAbilitySystemComponent& ASC, UObject* Source) const
{
	if (!ASC.IsOwnerActorAuthoritative())
	{
		return;
	}

	for (const auto& SetClass : AttributeSets)
	{
		if (SetClass && !ASC.GetSpawnedAttributes().ContainsByPredicate([SetClass](const UAttributeSet* Set)
		{
			return Set && Set->IsA(SetClass);
		}))
		{
			ASC.AddAttributeSetSubobject(NewObject<UAttributeSet>(ASC.GetOwner(), SetClass));
		}
	}

	for (const auto& Grant : Abilities)
	{
		if (!Grant.Ability || ASC.FindAbilitySpecFromClass(Grant.Ability))
		{
			continue;
		}

		FGameplayAbilitySpec Spec(Grant.Ability, 1, INDEX_NONE, Source);
		Spec.GetDynamicSpecSourceTags().AddTag(Grant.InputTag);
		ASC.GiveAbility(Spec);
	}

	for (const auto& Effect : InitialEffects)
	{
		if (Effect)
		{
			ASC.ApplyEffect(Effect);
		}
	}
}
