#include "CCLCombatViewModel.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystem/CCLStaminaSet.h"

void UCCLCombatViewModel::BeginDestroy()
{
	Unbind();
	Super::BeginDestroy();
}

void UCCLCombatViewModel::Bind(UAbilitySystemComponent* InASC)
{
	if (ASC == InASC)
	{
		return;
	}

	Unbind();
	ASC = InASC;

	if (!ASC)
	{
		return;
	}

	const TArray<FGameplayAttribute> Attributes = {UCCLHealthSet::GetHealthAttribute(), UCCLHealthSet::GetMaxHealthAttribute(), UCCLStaminaSet::GetStaminaAttribute(), UCCLStaminaSet::GetMaxStaminaAttribute()};

	for (const auto& Attribute : Attributes)
	{
		Values.Add(Attribute, ASC->GetNumericAttribute(Attribute));
		Handles.Add(Attribute,
			ASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddWeakLambda(this,
				[this, Attribute](const FOnAttributeChangeData& Data)
				{
					Values.FindOrAdd(Attribute) = Data.NewValue;
				}));
	}
}

float UCCLCombatViewModel::GetValue(const FGameplayAttribute& Attribute) const
{
	const float* Value = Values.Find(Attribute);
	return Value ? *Value : 0.f;
}

void UCCLCombatViewModel::Unbind()
{
	if (ASC)
	{
		for (const auto& Entry : Handles)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(Entry.Key).Remove(Entry.Value);
		}
	}

	Handles.Reset();
	Values.Reset();
	ASC = nullptr;
}
