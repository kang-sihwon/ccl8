#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CCLCombatAssetLibrary.generated.h"

UCLASS()
class CCL_API UCCLCombatAssetLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "CCL|Editor")
	static bool CreateCombatAssets();

	UFUNCTION(BlueprintCallable, Category = "CCL|Editor")
	static bool ConfigureCombatWorld();
};
