#include "CCLHealthSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

void UCCLHealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UCCLHealthSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCCLHealthSet, MaxHealth, COND_None, REPNOTIFY_Always);
}

void UCCLHealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	Clamp(Attribute, NewValue);
}

void UCCLHealthSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	Clamp(Attribute, NewValue);
}

void UCCLHealthSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
}

void UCCLHealthSet::Clamp(const FGameplayAttribute& Attribute, float& Value) const
{
	if (!FMath::IsFinite(Value))
	{
		Value = 0.f;
	}

	if (Attribute == GetMaxHealthAttribute())
	{
		Value = FMath::Max(Value, 0.f);
	}

	if (Attribute == GetHealthAttribute())
	{
		Value = FMath::Clamp(Value, 0.f, GetMaxHealth());
	}
}

void UCCLHealthSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCCLHealthSet, Health, OldValue);
}

void UCCLHealthSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCCLHealthSet, MaxHealth, OldValue);
}

void UCCLHealthSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxHealthAttribute() && GetHealth() > NewValue)
	{
		SetHealth(FMath::Max(0.f, NewValue));
	}
}
