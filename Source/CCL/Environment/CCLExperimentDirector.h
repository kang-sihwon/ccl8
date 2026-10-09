#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CCLExperimentDefinition.h"
#include "CCLSurfaceSimulation.h"
#include "CCLExperimentDirector.generated.h"

struct FCCLTerrainSaveContext;
class ACCLTerrainRegion;
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
		FGuid ExpectedGeneration, FGuid ExpectedRun, FString& Message, uint64 ExpectedTerrainSerial = 0, const FVector* TerrainTarget = nullptr);
	ACCLTerrainRegion* GetTerrainRegion() const { return TerrainRegion; }
	bool ResetExperiment(FString& Error);
	bool CanOperate(const APlayerController* Requester) const;
	bool IsRestoring() const { return bTerrainRestorePending != 0; }
	const FString& GetStorageMessage() const { return StorageMessage; }
	double GetSavedGameSeconds() const { return SavedGameSeconds; }
	double GetSavedWorldSeconds() const { return SavedWorldSeconds; }
	uint64 GetSavedTerrainRevision() const { return SavedTerrainRevision; }
	bool CanStart(FName CaseId, FString& Error) const;
	const UCCLExperimentDefinition* FindDefinition(FName CaseId) const;
	const FCCLExperimentResult* FindResult(FName CaseId) const;
	const FGuid& GetGeneration() const { return Generation; }
	bool IsReady() const { return HasAuthority() ? !InitialSnapshot.IsEmpty() && bWaterReady : Generation.IsValid() && Results.Num() == 12; }
	const TArray<FCCLExperimentResult>& GetResults() const { return Results; }
	static FGuid SnowRegionId() { return FGuid(301, 302, 303, 304); }
	static FCCLSurfaceGrid SnowGrid();
	static FGuid WaterRegionId() { return FGuid(401, 402, 403, 404); }
	static FGuid TerrainWaterRegionId() { return FGuid(451, 452, 453, 454); }
	static ACCLExperimentDirector* Find(const UWorld* World);

private:
	void InitializeWaterExperiment();
	void TickWaterExperiment();
	void TickSnowExperiment();
	bool StartSnowExperiment(FString& Error);
	bool SnowAction(ECCLExperimentAction Action, FName CaseId, FString& Error);
	bool StartWaterExperiment(FString& Error);
	bool WaterAction(ECCLExperimentAction Action, FName CaseId, FString& Error);
	void InitializeTerrainExperiment();
	void TickTerrainExperiment();
	bool TerrainAction(APlayerController* Requester, ECCLExperimentAction Action, FString& Error, const FVector* Target = nullptr);
	bool ResetTerrain(FString& Error);
	bool SaveTerrainWorld(FString& Error);
	bool LoadTerrainWorld(FString& Error);
	bool QueueWorldRestore(const TArray<uint8>& TerrainBytes, const FCCLTerrainSaveContext& Context, const TArray<uint8>& WorldBytes, FString& Error);
	FString GenerationRoot() const;
	bool StartCase(FName CaseId, FString& Error);
	void CompleteCase(FName CaseId, FGuid RunId, FGuid Token);
	void Finish(FCCLExperimentResult& Result, bool bPassed, const FString& Detail);
	bool RunClock(FString& Error);
	bool RunSnapshot(FGuid RunId, FString& Error);
	bool RunCelestials(FString& Error);
	bool RunShelter(FString& Error);
	bool ChangeEnvironment(ECCLExperimentAction Action, FString& Error);
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

	UPROPERTY(Replicated)
	TObjectPtr<ACCLTerrainRegion> TerrainRegion;

	TWeakObjectPtr<APawn> SnowWalker;
	FVector SnowWalkStart = FVector::ZeroVector;
	double SnowStartVolume = 0.;
	FCCLSurfaceForcing WaterCaseForcing;
	double WaterCaseTime = 0.;
	double WaterCaseRain = 0.;
	double WaterCaseIce = 0.;
	int32 WaterCaseStep = 0;
	uint8 bWaterReady = 0;
	uint8 bWaterParticipantBound = 0;
	uint64 TerrainAuthoritySequence = 0;
	int32 TerrainCaseStep = 0;
	UPROPERTY(Replicated)
	uint8 bTerrainRestorePending = 0;

	UPROPERTY(Replicated)
	FString StorageMessage = TEXT("이 실행에서는 아직 저장하지 않았다. 이전 실행의 저장은 불러올 수 있다.");

	UPROPERTY(Replicated)
	double SavedGameSeconds = -1.;

	UPROPERTY(Replicated)
	double SavedWorldSeconds = -1.;

	UPROPERTY(Replicated)
	uint64 SavedTerrainRevision = 0;

	uint8 bTravelPending = 0;
	TArray<uint8> InitialSnapshot;
	FTimerHandle RunTimer;
	UPROPERTY(Replicated)
	FName ActiveCase;
	double RunStarted = 0;
};
