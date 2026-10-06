#include "CCLOffenseSet.h"

#include "Net/UnrealNetwork.h"

void UCCLOffenseSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UCCLOffenseSet, AttackBonus, COND_None, REPNOTIFY_Always);
}

void UCCLOffenseSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	NewValue = FMath::IsFinite(NewValue) ? FMath::Clamp(NewValue, 0.f, 10000.f) : 0.f;
}

void UCCLOffenseSet::OnRep_AttackBonus(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCCLOffenseSet, AttackBonus, OldValue);
}
