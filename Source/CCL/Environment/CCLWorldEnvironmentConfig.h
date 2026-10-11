#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "CCLEnvironmentInputs.h"
#include "CCLEnvironmentView.h"
#include "Agents/CCLAgentEmbodiment.h"
#include "CCLWorldEnvironmentConfig.generated.h"

class ACharacter;
class UAnimInstance;

// Which persistent world store a map uses. Matches ECCLWorldDomain.
UENUM()
enum class ECCLWorldContentDomain : uint8
{
	Campaign,
	Playground,
	Scenario
};

UCLASS()
class CCL_API ACCLWorldEnvironmentConfig : public AInfo, public ICCLAgentWorldHost
{
	GENERATED_BODY()

	// 부모 인터페이스 함수
public:
	virtual bool BuildInitialPopulation(FCCLSimulationSnapshot& OutInitial, FString& Error) const override;

	// 내 클래스 함수
public:
	bool BuildInputs(FCCLEnvironmentInputs& OutInputs, FString& Error) const;

	bool ValidateViewInputs(const FCCLEnvironmentInputs& Inputs, FString& Error) const;

	static const ACCLWorldEnvironmentConfig* Find(const UWorld* World);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECCLWorldContentDomain Domain = ECCLWorldContentDomain::Campaign;

	// Seed of the generated merchant population in a new experiment world.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PopulationSeed = 42;

	// Applied to characters with deformable-surface movement, for example foot placement on deep snow.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<UAnimInstance> CharacterPostProcessAnim;

	// Body that experiments use to show individual Agent records. It must implement ICCLAgentEmbodiment.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<ACharacter> ExperimentResidentClass;

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
