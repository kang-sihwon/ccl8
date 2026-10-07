#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CCLProgressionAssetLibrary.generated.h"

UCLASS()
class CCL_API UCCLProgressionAssetLibrary : public UBlueprintFunctionLibrary
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
};
