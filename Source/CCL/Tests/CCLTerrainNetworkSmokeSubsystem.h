#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "CCLTerrainNetworkSmokeSubsystem.generated.h"

UCLASS()
class UCCLTerrainNetworkSmokeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return !IsTemplate() && !bComplete; }
	virtual TStatId GetStatId() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }

private:
	void Finish(bool bSuccess, const FString& Message);

private:
	double Started = 0.;
	double FinishAt = 0.;
	int32 Step = 0;
	uint8 bComplete = 0;
	uint8 bCheckedPendingReplica = 0;
};
