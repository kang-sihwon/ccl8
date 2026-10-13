#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CCLTestResourceLibrary.generated.h"

UCLASS()
class CCLEDITOR_API UCCLTestResourceLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Copies an explicit dependency closure. Sources are preserved; existing destinations are rejected.
	UFUNCTION(BlueprintCallable, Category = "CCL|Tests")
	static bool CopyResources(const TMap<FString, FString>& Packages);

	// Repoints only the supplied object and its owned subobjects, never all loaded source packs.
	UFUNCTION(BlueprintCallable, Category = "CCL|Tests")
	static bool RepointResources(UObject* Object, const TMap<FString, FString>& Packages);
};
