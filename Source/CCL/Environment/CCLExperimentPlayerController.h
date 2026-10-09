#pragma once

#include "CoreMinimal.h"
#include "CCLPlayerController.h"
#include "CCLExperimentDefinition.h"
#include "CCLExperimentPlayerController.generated.h"

class UCCLTerrainReplication;
class UCCLSurfaceReplication;
class UInputAction;
class UInputMappingContext;

UCLASS()
class CCL_API ACCLExperimentPlayerController : public ACCLPlayerController
{
	GENERATED_BODY()

public:
	ACCLExperimentPlayerController();
	virtual void PlayerTick(float DeltaTime) override;

protected:
	virtual void SetupInputComponent() override;
	virtual void EndPlay(EEndPlayReason::Type Reason) override;

public:
	UFUNCTION(Exec)
	void CCLExperiment();

	void Submit(ECCLExperimentAction Action, FName CaseId = NAME_None);

	UFUNCTION(Server, Reliable)
	void ServerExperiment(ECCLExperimentAction Action, FName CaseId, FGuid Generation, FGuid RunId, uint64 TerrainSerial = 0);

	UFUNCTION(Client, Reliable)
	void ClientExperimentResponse(const FString& Message);

	const FString& GetExperimentMessage() const { return LastMessage; }
	FCCLUIViewHandle GetExperimentView() const { return ExperimentView; }

private:
	UPROPERTY()
	TObjectPtr<UCCLTerrainReplication> TerrainReplication;

	UPROPERTY()
	TObjectPtr<UCCLSurfaceReplication> SurfaceReplication;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ToggleAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> ExperimentInput;

	FCCLUIRegistrationHandle Registration;
	FCCLUIViewHandle ExperimentView;
	FString LastMessage;
	uint8 bOpenedOnce = 0;
};
