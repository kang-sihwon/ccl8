#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
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
class CCL_API UCCLItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;

public:
	template <class T> const T* FindFragment() const
	{
		for (const auto& Fragment : ItemFragments)
		{
			if (const T* Value = Fragment.GetPtr<T>())
			{
				return Value;
			}
		}
		return nullptr;
	}
	FText GetLabel() const;
	int32 GetMaxStack() const;
	bool ValidateDefinition(TArray<FText>& Errors) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

public:
	UPROPERTY(EditAnywhere, Category = "Item")
	FText ItemName;

	UPROPERTY(EditAnywhere, Category = "Item", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, Category = "Item")
	TObjectPtr<UTexture2D> IconTexture;

	UPROPERTY(EditAnywhere, Category = "Item", meta = (ClampMin = "1", ClampMax = "1000"))
	int32 MaxStackCount = 1;

	UPROPERTY(EditAnywhere, Category = "Item", meta = (BaseStruct = "/Script/CCL.CCLItemFragmentData", ExcludeBaseStruct))
	TArray<FInstancedStruct> ItemFragments;

	// Read-only compatibility for assets saved before the struct migration.
	UPROPERTY(Instanced)
	TArray<TObjectPtr<UCCLItemFragment>> Fragments;

private:
	UPROPERTY()
	int32 FragmentSchemaVersion = 0;
};
