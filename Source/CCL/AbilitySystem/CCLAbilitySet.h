#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayAbilitySpec.h"
#include "AttributeSet.h"
#include "CCLAbilitySet.generated.h"

class UCCLAbilitySystemComponent;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct FCCLAbilityGrant
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Ability")
	TSubclassOf<UGameplayAbility> Ability;

	UPROPERTY(EditAnywhere, Category = "Ability")
	FGameplayTag InputTag;
};

UCLASS()
class CCL_API UCCLAbilitySet : public UDataAsset
{
	GENERATED_BODY()

public:
	void GrantTo(UCCLAbilitySystemComponent& ASC, UObject* Source) const;

public:
	UPROPERTY(EditAnywhere, Category = "Abilities")
	TArray<FCCLAbilityGrant> Abilities;

	UPROPERTY(EditAnywhere, Category = "Attributes")
	TArray<TSubclassOf<UAttributeSet>> AttributeSets;

	UPROPERTY(EditAnywhere, Category = "Effects")
	TArray<TSubclassOf<UGameplayEffect>> InitialEffects;
};
