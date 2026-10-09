#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "CCLExperimentSmokeSubsystem.generated.h"

class SWidget;

UCLASS()
class UCCLExperimentSmokeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return !IsTemplate() && !bComplete; }
	virtual TStatId GetStatId() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }

private:
	void TickMenuInput(double Now);
	void TickLab(double Now);
	void TickTravel(double Now);
	void TickEnvironment(double Now);
	bool CheckReplicatedEnvironment();
	bool ClickControl(TSharedPtr<SWidget> Control);
	bool Check(bool bCondition, const TCHAR* Message);
	void Capture(const TCHAR* Name);
	void Finish(const TCHAR* Role);

private:
	double Started = 0;
	double Next = 0;
	int32 Step = 0;
	uint8 bComplete = 0;
	FGuid HubWorldId;
	double HubGameSeconds = 0;
	FGuid PreviousGeneration;
	FGuid PreviousRun;
	double PreviousSpinPhase = 0.;
	uint64 PreviousTerrainRevision = 0;
	FVector2D LabCursor = FVector2D::ZeroVector;
	FVector LabTarget = FVector::ZeroVector;
};
