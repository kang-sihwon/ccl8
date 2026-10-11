#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayAbilitySpec.h"
#include "AttributeSet.h"
#include "KSIAbilitySet.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct KSICOMBAT_API FKSIAbilityGrant
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Ability")
	TSubclassOf<UGameplayAbility> Ability;

	UPROPERTY(EditAnywhere, Category = "Ability")
	FGameplayTag InputTag;
};

UCLASS()
class KSICOMBAT_API UKSIAbilitySet : public UDataAsset
{
	GENERATED_BODY()

public:
	void GrantTo(UAbilitySystemComponent& ASC, UObject* Source) const;

public:
	UPROPERTY(EditAnywhere, Category = "Abilities")
	TArray<FKSIAbilityGrant> Abilities;

	UPROPERTY(EditAnywhere, Category = "Attributes")
	TArray<TSubclassOf<UAttributeSet>> AttributeSets;

	UPROPERTY(EditAnywhere, Category = "Effects")
	TArray<TSubclassOf<UGameplayEffect>> InitialEffects;

    UPROPERTY(EditAnywhere, Category = "Effects")
    FGameplayTag InitialMagnitudeTag;

};
