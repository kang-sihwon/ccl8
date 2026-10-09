#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "CCLSnowSmokeSubsystem.generated.h"

class ACameraActor;

UCLASS()
class UCCLSnowSmokeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float Dt) override;
	virtual bool IsTickable() const override { return !IsTemplate() && !bComplete; }
	virtual TStatId GetStatId() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
private:
	void Finish(bool bSuccess, const FString& Message);
	TWeakObjectPtr<ACameraActor> Camera;
	TArray<uint8> Saved;
	double Started = 0.;
	double Next = 0.;
	double MaxDepth = 0.;
	uint32 PeakReplays = 0;
	uint32 PeakCorrections = 0;
	double MinFootHeight = 1000.;
	double MaxFootHeight = -1000.;
	uint8 bPowderObserved = 0;
	uint8 bFeetCaptured = 0;
	uint8 bShowSnowNextTick = 0;
	uint8 bHistoryPaused = 0;
	int32 Step = 0;
	uint8 bBaseReported = 0;
	uint8 bWalkCaptured = 0;
	uint8 bComplete = 0;
};
