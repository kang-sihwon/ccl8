#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "KSIEcologySimulation.h"
#include "CCLEcologySmokeSubsystem.generated.h"

UCLASS()
class UCCLEcologySmokeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
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
	FKSIEcologyLease Lease;
	TArray<uint8> Saved;
	TArray<uint8> EcologyBefore;
	double Started = 0.;
	double Next = 0.;
	double InitialTime = 0.;
	int64 BornBefore = 0;
	int32 Step = 0;
	uint8 bComplete = 0;
};
