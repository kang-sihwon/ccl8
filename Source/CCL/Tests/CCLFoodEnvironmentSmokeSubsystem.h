#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "CCLFoodEnvironmentSmokeSubsystem.generated.h"

UCLASS()
class UCCLFoodEnvironmentSmokeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
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
	double Next = 0.;
	double TargetWorld = 0.;
	double FrozenSpoilage = 0.;
	double InitialRoomTemperature = 0.;
	double ExpectedRoomTemperature = 0.;
	double ExpectedRoomSampleTime = 0.;
	double SavedWorldTime = 0.;
	TArray<uint8> SavedInventory;
	int32 Step = 0;
	uint8 bComplete = 0;
};
