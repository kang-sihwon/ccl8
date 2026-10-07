#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CCLUITypes.h"
#include "CCLUIRegistry.generated.h"

class UCCLScreen;
class UCCLUIContext;
class UInputMappingContext;

USTRUCT(BlueprintType)
struct CCL_API FCCLUIViewDefinition
{
	GENERATED_BODY()

	bool AcceptsContext(const UCCLUIContext* Context) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "UI.View"))
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftClassPtr<UCCLScreen> WidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UCCLUIContext> RequiredContextClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "UI.Layer"))
	FGameplayTag Layer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "UI.Extension"))
	FGameplayTag Extension;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "UI.Group"))
	FGameplayTagContainer Groups;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ECCLUIInstancePolicy InstancePolicy = ECCLUIInstancePolicy::Single;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ECCLUIScope Scope = ECCLUIScope::World;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ECCLUIInputPolicy InputPolicy = ECCLUIInputPolicy::Inherit;
};

UCLASS(BlueprintType)
class CCL_API UCCLUIRegistry : public UDataAsset
{
	GENERATED_BODY()

public:
	bool ValidateRegistry(TArray<FText>& Errors) const;
	bool ValidateView(const FCCLUIViewDefinition& View, TArray<FText>& Errors) const;
	const FCCLUIViewDefinition* FindView(FGameplayTag Tag) const;
	const FCCLUILayerDefinition* FindLayer(FGameplayTag Tag) const;
	const FCCLUIExtensionDefinition* FindExtension(FGameplayTag Tag) const;

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
	TArray<FCCLUILayerDefinition> Layers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (TitleProperty = "Tag"))
	TArray<FCCLUIExtensionDefinition> Extensions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (TitleProperty = "Tag"))
	TArray<FCCLUIViewDefinition> Views;
};
