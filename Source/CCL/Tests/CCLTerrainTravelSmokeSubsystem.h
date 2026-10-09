#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "Environment/CCLWorldGenerationStore.h"
#include "CCLTerrainTravelSmokeSubsystem.generated.h"

UCLASS()
class UCCLTerrainTravelSmokeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return !IsTemplate() && !bComplete; }
	virtual TStatId GetStatId() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }

private:
	void FreezeClock(UWorld* World, ELevelTick TickType, float DeltaTime);
	void Finish(bool bSuccess, const FString& Message);

private:
	TMap<ECCLWorldDomain, FCCLWorldGenerationBundle> Expected;
	TMap<ECCLWorldDomain, FGuid> PreviousEpochs;
	TWeakObjectPtr<UWorld> DepartedWorld;
	double Started = 0.;
	int32 Step = 0;
	uint8 bComplete = 0;
	uint8 bAnnounced = 0;
};
