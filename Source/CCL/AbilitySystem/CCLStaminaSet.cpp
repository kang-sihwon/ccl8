#include "CCLStaminaSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

void UCCLStaminaSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UCCLStaminaSet, Stamina, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCCLStaminaSet, MaxStamina, COND_OwnerOnly, REPNOTIFY_Always);
}

void UCCLStaminaSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	Clamp(Attribute, NewValue);
}

void UCCLStaminaSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	Clamp(Attribute, NewValue);
}

void UCCLStaminaSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	SetStamina(FMath::Clamp(GetStamina(), 0.f, GetMaxStamina()));
}

void UCCLStaminaSet::Clamp(const FGameplayAttribute& Attribute, float& Value) const
{
	if (!FMath::IsFinite(Value))
	{
		Value = 0.f;
	}

	if (Attribute == GetMaxStaminaAttribute())
	{
		Value = FMath::Max(Value, 0.f);
	}

	if (Attribute == GetStaminaAttribute())
	{
		Value = FMath::Clamp(Value, 0.f, GetMaxStamina());
	}
}

void UCCLStaminaSet::OnRep_Stamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCCLStaminaSet, Stamina, OldValue);
}

void UCCLStaminaSet::OnRep_MaxStamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCCLStaminaSet, MaxStamina, OldValue);
}

void UCCLStaminaSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxStaminaAttribute() && GetStamina() > NewValue)
	{
		SetStamina(FMath::Max(0.f, NewValue));
	}
}
