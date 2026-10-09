#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CCLExperimentDefinition.h"
#include "CCLExperimentDirector.generated.h"

class APlayerController;
class APlayerState;

UCLASS()
class CCL_API ACCLExperimentDirector : public AActor
{
	GENERATED_BODY()

public:
	ACCLExperimentDirector();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(EEndPlayReason::Type Reason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	void AssignOperator(APlayerController* Controller);
	void ReleaseOperator(APlayerController* Controller);
	bool Execute(APlayerController* Requester, ECCLExperimentAction Action, FName CaseId,
		FGuid ExpectedGeneration, FGuid ExpectedRun, FString& Message);
	bool ResetExperiment(FString& Error);
	bool CanOperate(const APlayerController* Requester) const;
	bool CanStart(FName CaseId, FString& Error) const;
	const UCCLExperimentDefinition* FindDefinition(FName CaseId) const;
	const FCCLExperimentResult* FindResult(FName CaseId) const;
	const FGuid& GetGeneration() const { return Generation; }
	bool IsReady() const { return HasAuthority() ? !InitialSnapshot.IsEmpty() : Generation.IsValid() && Results.Num() == 12; }
	const TArray<FCCLExperimentResult>& GetResults() const { return Results; }
	static ACCLExperimentDirector* Find(const UWorld* World);

private:
	bool StartCase(FName CaseId, FString& Error);
	void CompleteCase(FName CaseId, FGuid RunId, FGuid Token);
	void Finish(FCCLExperimentResult& Result, bool bPassed, const FString& Detail);
	bool RunClock(FString& Error);
	bool RunSnapshot(FGuid RunId, FString& Error);
	bool SaveCheckpoint(FString& Error);
	bool LoadCheckpoint(FString& Error);
	bool MoveToSafety(APlayerController* OnlyPlayer, int32 Zone, FString& Error);
	FString SlotName() const;

public:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	TArray<TObjectPtr<UCCLExperimentDefinition>> Definitions;

private:
	UPROPERTY(Replicated)
	TArray<FCCLExperimentResult> Results;

	UPROPERTY(Replicated)
	FGuid Generation;

	UPROPERTY(Replicated)
	TObjectPtr<APlayerState> Operator;

	TArray<uint8> InitialSnapshot;
	FTimerHandle RunTimer;
	UPROPERTY(Replicated)
	FName ActiveCase;
	double RunStarted = 0;
};
