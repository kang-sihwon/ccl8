#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "CCLFoodZoneSmokeSubsystem.generated.h"

UCLASS()
class UCCLFoodZoneSmokeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
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
	TArray<uint8> Saved;
	TArray<uint8> GroundSaved;
	TArray<uint8> IndoorSaved;
	FGuid GroundId;
	TWeakObjectPtr<class ACCLWorldPickup> GroundPickup;
	TArray<FString> FoodBefore;
	int32 Step = 0;
	uint8 bComplete = 0;
};
