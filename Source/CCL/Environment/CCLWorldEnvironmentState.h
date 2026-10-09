#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CCLWorldEnvironmentState.generated.h"

USTRUCT()
struct FCCLReplicatedWorldTime
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid WorldId;

	// Changes on restore/reset even when the saved step number moves backwards.
	UPROPERTY()
	FGuid Epoch;

	UPROPERTY()
	double GameSeconds = 0;

	UPROPERTY()
	double WorldSeconds = 0;

	UPROPERTY()
	double TimeScale = 60;

	UPROPERTY()
	double PendingGameSeconds = 0;

	UPROPERTY()
	uint64 CompletedStepId = 0;

	UPROPERTY()
	uint8 bAdvanceFailed = 0;
};

UCLASS()
class CCL_API ACCLWorldEnvironmentState : public AActor
{
	GENERATED_BODY()

public:
	ACCLWorldEnvironmentState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	void Publish(const FCCLReplicatedWorldTime& Value);
	const FCCLReplicatedWorldTime& GetTime() const { return Time; }

private:
	UPROPERTY(Replicated)
	FCCLReplicatedWorldTime Time;
};
