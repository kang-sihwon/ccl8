#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CCLItemDefinition.generated.h"

class UCCLCombatDefinition;
class UCCLAbilitySet;
class UGameplayEffect;

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

UCLASS(EditInlineNew)
class CCL_API UCCLItemFragment_Display : public UCCLItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Display")
	FText Label;
};

UCLASS(EditInlineNew)
class CCL_API UCCLItemFragment_Stack : public UCCLItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Stack", meta = (ClampMin = "1", ClampMax = "1000"))
	int32 MaxCount = 1;
};

UCLASS(EditInlineNew)
class CCL_API UCCLItemFragment_Equipment : public UCCLItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Equipment")
	TSubclassOf<UGameplayEffect> Effect;

	UPROPERTY(EditAnywhere, Category = "Equipment")
	float Magnitude = 0.f;
};

UCLASS(EditInlineNew)
class CCL_API UCCLItemFragment_Consumable : public UCCLItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Use")
	TSubclassOf<UGameplayEffect> Effect;

	UPROPERTY(EditAnywhere, Category = "Use")
	float Magnitude = 50.f;
};

UCLASS(BlueprintType)
class CCL_API UCCLItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	const UCCLItemFragment* FindFragment(TSubclassOf<UCCLItemFragment> Type) const;
	FText GetLabel() const;
	int32 GetMaxStack() const;

public:
	UPROPERTY(EditAnywhere, Instanced, Category = "Item")
	TArray<TObjectPtr<UCCLItemFragment>> Fragments;
};
