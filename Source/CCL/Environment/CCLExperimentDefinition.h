#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CCLExperimentDefinition.generated.h"

UENUM(BlueprintType)
enum class ECCLExperimentKind : uint8
{
	Guide,
	Clock,
	Snapshot,
	Reserved,
	Celestial,
	Shelter
};

UENUM(BlueprintType)
enum class ECCLExperimentStatus : uint8
{
	NotImplemented,
	Ready,
	Running,
	Passed,
	Failed,
	ManualReviewNeeded
};

UENUM(BlueprintType)
enum class ECCLExperimentAction : uint8
{
	Start,
	Stop,
	Reset,
	Save,
	Load,
	Teleport,
	ScaleZero,
	ScaleOne,
	ScaleSixty,
	TravelHub,
	TravelScenario,
	TravelCombat,
	TravelMultiplayer,
	NextLatitude,
	NextObliquity,
	RotateQuarter,
	OrbitQuarter,
	CycleOpening
};

USTRUCT()
struct FCCLExperimentResult
{
	GENERATED_BODY()

	UPROPERTY()
	FName CaseId;

	UPROPERTY()
	FGuid RunId;

	UPROPERTY()
	FGuid Generation;

	UPROPERTY()
	ECCLExperimentStatus Status = ECCLExperimentStatus::NotImplemented;

	UPROPERTY()
	FString Detail;

	UPROPERTY()
	double CompletedWorldSeconds = 0;
};

UCLASS(BlueprintType)
class CCL_API UCCLExperimentDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, CallInEditor)
	void ConfigureZone(int32 Index);

	bool Validate(FString& Error) const;
	bool IsImplemented() const { return Kind != ECCLExperimentKind::Reserved; }
	static FVector ZoneCenter(int32 Index);
	static FString StatusText(ECCLExperimentStatus Status);

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName CaseId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Zone = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Instructions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Expected;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ECCLExperimentKind Kind = ECCLExperimentKind::Reserved;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Seed = 42;
};
