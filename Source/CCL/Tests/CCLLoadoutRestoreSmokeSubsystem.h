#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "CCLLoadoutRestoreSmokeSubsystem.generated.h"

UCLASS()
class UCCLLoadoutRestoreSmokeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return !IsTemplate() && !bComplete; }
	virtual TStatId GetStatId() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }

private:
	void Finish(bool bSuccess, const TCHAR* Message);
	bool CheckBatch(class ACCLPlayerState& First);
	bool CheckProfiles(class ACCLPlayerState& First);
	bool CheckPlayerFood(class ACCLPlayerState& First);
	bool CheckFreshProfiles(class ACCLPlayerState& First);
	double Started = 0.;
	uint8 bComplete = 0;
	uint8 bBaseComplete = 0;
	uint8 bProfilesComplete = 0;
	uint8 bProfileRetry = 0;
	uint8 FoodStep = 0;
	double FoodStartTime = 0.;
	TArray<uint8> ReaderWorld;
};
