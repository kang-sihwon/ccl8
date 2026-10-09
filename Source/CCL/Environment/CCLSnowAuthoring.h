#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CCLSnowAuthoring.generated.h"

UCLASS()
class UCCLSnowAuthoring : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="CCL|Editor")
	static bool BuildPostProcessAsset();
};
