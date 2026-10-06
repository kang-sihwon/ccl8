#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayEffectTypes.h"
#include "CCLCombatViewModel.generated.h"

class UAbilitySystemComponent;

UCLASS()
class CCL_API UCCLCombatViewModel : public UObject
{
	GENERATED_BODY()

public:
	virtual void BeginDestroy() override;

public:
	void Bind(UAbilitySystemComponent* InASC);
	float GetValue(const FGameplayAttribute& Attribute) const;

private:
	void Unbind();

private:
	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> ASC;

	TMap<FGameplayAttribute, float> Values;
	TMap<FGameplayAttribute, FDelegateHandle> Handles;
};
