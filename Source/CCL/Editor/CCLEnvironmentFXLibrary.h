#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CCLEnvironmentFXLibrary.generated.h"

class UMaterialInterface;
class UNiagaraSystem;

// Editor authoring for the stateless Niagara systems used by weather and fire presentation.
// Validation scripts call these through Python; game builds return false.
UCLASS()
class CCL_API UCCLEnvironmentFXLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	// 내 클래스 함수
public:
	UFUNCTION(BlueprintCallable, Category = "CCL|Environment")
	static bool ConfigurePrecipitationSystem(UNiagaraSystem* System, bool bSnow);

	UFUNCTION(BlueprintCallable, Category = "CCL|Environment")
	static bool ConfigureFireSystem(UNiagaraSystem* System, bool bSmoke, UMaterialInterface* SpriteMaterial);
};
