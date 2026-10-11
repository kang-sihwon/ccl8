#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DeveloperSettings.h"
#include "CCLItemCatalog.generated.h"

class UKSIItemDefinition;
class UCCLItemDefinition;
class UCCLSkillDefinition;

// Content-owned list of definitions that saves may refer to by asset name.
// Saves never load an arbitrary path: an unlisted name resolves to null and the save is rejected.
UCLASS(BlueprintType)
class CCL_API UCCLItemCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()

	// 내 클래스 함수
public:
	bool ValidateCatalog(TArray<FText>& Errors) const;
	UKSIItemDefinition* FindItem(FName Name) const;
	UCCLSkillDefinition* FindSkill(FName Name) const;

	// The catalog configured in the project settings, or null when it is missing.
	static const UCCLItemCatalog* Get();

	// 에디터 전용
#if WITH_EDITOR
public:
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	// 프로퍼티
public:
	UPROPERTY(EditAnywhere, Category = "Catalog")
	TArray<TObjectPtr<UKSIItemDefinition>> Items;

	// Training offered to players and accepted in saves.
	UPROPERTY(EditAnywhere, Category = "Catalog")
	TArray<TObjectPtr<UCCLSkillDefinition>> TrainingSkills;
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "CCL Item Catalog"))
class CCL_API UCCLItemCatalogSettings : public UDeveloperSettings
{
	GENERATED_BODY()

	// 프로퍼티
public:
	UPROPERTY(Config, EditAnywhere, Category = "Items")
	TSoftObjectPtr<UCCLItemCatalog> Catalog;
};
