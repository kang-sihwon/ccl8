#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CCLItemDefinition.generated.h"

class UCCLCombatDefinition;
class UCCLAbilitySet;

UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class CCL_API UCCLItemFragment : public UObject
{
	GENERATED_BODY()
};

UCLASS(EditInlineNew)
class CCL_API UCCLItemFragment_Combat : public UCCLItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UCCLCombatDefinition> Combat;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UCCLAbilitySet> Abilities;
};

UCLASS(BlueprintType)
class CCL_API UCCLItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	const UCCLItemFragment* FindFragment(TSubclassOf<UCCLItemFragment> Type) const;

public:
	UPROPERTY(EditAnywhere, Instanced, Category = "Item")
	TArray<TObjectPtr<UCCLItemFragment>> Fragments;
};
