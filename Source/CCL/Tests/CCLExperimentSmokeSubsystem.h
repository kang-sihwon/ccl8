#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "CCLExperimentSmokeSubsystem.generated.h"

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
	void TickTravel(double Now);
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
};
