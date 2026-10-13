#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "KSIEnvironmentInputs.h"
#include "KSIEnvironmentView.h"
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
	bool BuildInputs(FKSIEnvironmentInputs& OutInputs, FString& Error) const;

	bool ValidateViewInputs(const FKSIEnvironmentInputs& Inputs, FString& Error) const;

	static const ACCLWorldEnvironmentConfig* Find(const UWorld* World);

public:
	ACCLWorldEnvironmentConfig();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
	static bool IsLaboratory(const UWorld* World);
	static bool AllowsLaboratory(const UWorld* World, int32 Zone, int32 Subject = -1);
	static FString LaboratoryMap(int32 Zone, int32 Subject = 0);
	static FString LaboratoryTitle(int32 Zone, int32 Subject = 0);
	static TArray<FString> LaboratoryVariants(int32 Zone, int32 Subject);
	int32 SampleCount() const { return FMath::Clamp(LaboratoryCount, 1, 16); }

	// -1 retains the combined experiment maps. Dedicated laboratories seed only this zone.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Laboratory", meta = (ClampMin = "-1", ClampMax = "11"))
	int32 LaboratoryZone = -1;

	// Zone 7: 0 vegetation, 1 life, 2 food decay.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Laboratory", meta = (ClampMin = "0", ClampMax = "2"))
	int32 LaboratorySubject = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Laboratory", meta = (ClampMin = "0", ClampMax = "8"))
	int32 LaboratoryVariant = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Laboratory", meta = (ClampMin = "1", ClampMax = "16"))
	int32 LaboratoryCount = 3;

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
	TObjectPtr<UKSICelestialDefinition> CelestialDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FKSICelestialObserver Observer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FKSISurfacePatch> Surfaces;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FKSISurfaceOpening> Openings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FKSIEnvironmentProbe> Probes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGuid> ViewOpeningIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGuid> ViewSurfaceIds;
};
