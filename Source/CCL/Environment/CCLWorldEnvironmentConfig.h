#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "CCLEnvironmentInputs.h"
#include "CCLEnvironmentView.h"
#include "CCLWorldEnvironmentConfig.generated.h"

UCLASS()
class CCL_API ACCLWorldEnvironmentConfig : public AInfo
{
	GENERATED_BODY()

public:
	bool BuildInputs(FCCLEnvironmentInputs& OutInputs, FString& Error) const;

	bool ValidateViewInputs(const FCCLEnvironmentInputs& Inputs, FString& Error) const;

	static const ACCLWorldEnvironmentConfig* Find(const UWorld* World);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCCLCelestialDefinition> CelestialDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCCLCelestialObserver Observer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCCLSurfacePatch> Surfaces;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCCLSurfaceOpening> Openings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCCLEnvironmentProbe> Probes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGuid> ViewOpeningIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGuid> ViewSurfaceIds;
};
