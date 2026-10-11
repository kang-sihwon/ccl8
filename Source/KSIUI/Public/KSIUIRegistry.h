#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "KSIUITypes.h"
#include "KSIUIRegistry.generated.h"

class UKSIScreen;
class UKSIUIContext;
class UInputMappingContext;

USTRUCT(BlueprintType)
struct KSIUI_API FKSIUIViewDefinition
{
	GENERATED_BODY()

	bool AcceptsContext(const UKSIUIContext* Context) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "UI.View"))
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftClassPtr<UKSIScreen> WidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UKSIUIContext> RequiredContextClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "UI.Layer"))
	FGameplayTag Layer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "UI.Extension"))
	FGameplayTag Extension;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "UI.Group"))
	FGameplayTagContainer Groups;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EKSIUIInstancePolicy InstancePolicy = EKSIUIInstancePolicy::Single;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EKSIUIScope Scope = EKSIUIScope::World;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EKSIUIInputPolicy InputPolicy = EKSIUIInputPolicy::Inherit;
};

UCLASS(BlueprintType)
class KSIUI_API UKSIUIRegistry : public UDataAsset
{
	GENERATED_BODY()

public:
	bool ValidateRegistry(TArray<FText>& Errors) const;
	bool ValidateView(const FKSIUIViewDefinition& View, TArray<FText>& Errors) const;
	const FKSIUIViewDefinition* FindView(FGameplayTag Tag) const;
	const FKSIUILayerDefinition* FindLayer(FGameplayTag Tag) const;
	const FKSIUIExtensionDefinition* FindExtension(FGameplayTag Tag) const;

#if WITH_EDITOR
public:
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UInputMappingContext> InputMapping;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 InputMappingPriority = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (TitleProperty = "Tag"))
	TArray<FKSIUILayerDefinition> Layers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (TitleProperty = "Tag"))
	TArray<FKSIUIExtensionDefinition> Extensions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (TitleProperty = "Tag"))
	TArray<FKSIUIViewDefinition> Views;
};
