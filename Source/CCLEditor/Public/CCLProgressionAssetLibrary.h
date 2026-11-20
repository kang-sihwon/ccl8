#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CCLProgressionAssetLibrary.generated.h"

class UCCLAbilitySet;
class UCCLCombatDefinition;
class UCCLItemDefinition;

UCLASS()
class CCLEDITOR_API UCCLProgressionAssetLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CCL|Editor")
	static bool CreateProgressionAssets();
	UFUNCTION(BlueprintCallable, Category = "CCL|Editor")
	static bool CreateEncounterAssets();
	UFUNCTION(BlueprintCallable, Category = "CCL|Editor")
	static bool CreateEquipmentAssets();

	UFUNCTION(BlueprintCallable, Category = "CCL|Editor")
	static bool CreateAttachmentProfileAssets();

	UFUNCTION(BlueprintCallable, Category = "CCL|Editor")
	static bool MigrateEquipmentTagAssets();

	UFUNCTION(BlueprintCallable, Category = "CCL|Editor")
	static bool CreateProjectileAssets();

	// 모든 아이템 정의를 이 라이브러리의 값으로 다시 만든다(결정 70). 맨손, 진행 보상, 장비, 총기, 음식이 대상이다.
	UFUNCTION(BlueprintCallable, Category = "CCL|Editor")
	static bool RebuildItemDefinitions();

	// 맨손 정의의 공통 값이다. 전투 에셋 생성과 재생성이 함께 쓴다.
	static void ConfigureUnarmedItem(UCCLItemDefinition& Item, UCCLCombatDefinition* Combat, UCCLAbilitySet* Abilities, const TCHAR* Label);

	UFUNCTION(BlueprintCallable, Category = "CCL|Editor")
	static bool CreateAgentAssets();

	UFUNCTION(BlueprintCallable, Category = "CCL|Editor")
	static bool MigrateAgentBiologyAssets();
};
