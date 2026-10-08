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
		PublishValues();
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
					PublishValues();
				}));
	}

	PublishValues();
}

void UCCLCombatViewModel::PublishValues()
{
	UE_MVVM_SET_PROPERTY_VALUE(Health, GetValue(UCCLHealthSet::GetHealthAttribute()));
	UE_MVVM_SET_PROPERTY_VALUE(MaxHealth, GetValue(UCCLHealthSet::GetMaxHealthAttribute()));
	UE_MVVM_SET_PROPERTY_VALUE(Stamina, GetValue(UCCLStaminaSet::GetStaminaAttribute()));
	UE_MVVM_SET_PROPERTY_VALUE(MaxStamina, GetValue(UCCLStaminaSet::GetMaxStaminaAttribute()));
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
