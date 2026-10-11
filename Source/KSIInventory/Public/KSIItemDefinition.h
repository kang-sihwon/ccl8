#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StructUtils/InstancedStruct.h"
#include "KSIItemDefinition.generated.h"
class UTexture2D;

USTRUCT(BlueprintType)
struct KSIINVENTORY_API FKSIItemFragmentData
{
    GENERATED_BODY()
};

UCLASS(BlueprintType)
class KSIINVENTORY_API UKSIItemDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

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
    virtual bool ValidateDefinition(TArray<FText>& Errors) const;
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
    UPROPERTY(EditAnywhere, Category = "Item", meta = (BaseStruct = "/Script/KSIInventory.KSIItemFragmentData", ExcludeBaseStruct))
    TArray<FInstancedStruct> ItemFragments;
};
