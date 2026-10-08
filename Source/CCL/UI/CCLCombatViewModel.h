#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "GameplayEffectTypes.h"
#include "CCLCombatViewModel.generated.h"

class UAbilitySystemComponent;

UCLASS()
class CCL_API UCCLCombatViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	virtual void BeginDestroy() override;

public:
	void Bind(UAbilitySystemComponent* InASC);
	float GetValue(const FGameplayAttribute& Attribute) const;

private:
	void Unbind();
	void PublishValues();

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Vitals")
	float Health = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Vitals")
	float MaxHealth = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Vitals")
	float Stamina = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Vitals")
	float MaxStamina = 0.f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> ASC;

	TMap<FGameplayAttribute, float> Values;
	TMap<FGameplayAttribute, FDelegateHandle> Handles;
};
