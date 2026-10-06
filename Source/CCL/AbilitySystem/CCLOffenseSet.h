#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "CCLOffenseSet.generated.h"

UCLASS()
class CCL_API UCCLOffenseSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

public:
	ATTRIBUTE_ACCESSORS_BASIC(UCCLOffenseSet, AttackBonus);

private:
	UFUNCTION()
	void OnRep_AttackBonus(const FGameplayAttributeData& OldValue);

public:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackBonus, Category = "Attributes")
	FGameplayAttributeData AttackBonus = 0.f;
};
