#pragma once

#include "CoreMinimal.h"
#include "KSIItemDefinition.h"
#include "StructUtils/InstancedStruct.h"
#include "CCLItemFragments.h"
#include "CCLItemDefinition.generated.h"

class UCCLCombatDefinition;
class UCCLAbilitySet;
class UGameplayEffect;
class UTexture2D;

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
class CCL_API UCCLItemDefinition : public UKSIItemDefinition
{
	GENERATED_BODY()

public:
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;

public:
	UFUNCTION(BlueprintCallable, Category = "Item")
	bool ConfigurePerishable(double ShelfLifeSeconds, double MoldDelaySeconds, double MoldGrowthSeconds);

	virtual bool ValidateDefinition(TArray<FText>& Errors) const override;

public:
	// Read-only compatibility for assets saved before the struct migration.
	UPROPERTY(Instanced)
	TArray<TObjectPtr<UCCLItemFragment>> Fragments;

private:
	UPROPERTY()
	int32 FragmentSchemaVersion = 0;
};
