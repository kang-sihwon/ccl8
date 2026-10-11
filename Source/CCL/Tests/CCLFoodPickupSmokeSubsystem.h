#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "CCLFoodPickupSmokeSubsystem.generated.h"

class ACCLWorldPickup;
class UCCLInventoryComponent;

UCLASS()
class UCCLFoodPickupSmokeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
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
	UPROPERTY()
	TObjectPtr<ACCLWorldPickup> Pickup;

	UPROPERTY()
	TObjectPtr<UCCLInventoryComponent> Target;

	TArray<uint8> GroundSave;
	TArray<uint8> CollectedSave;
	FGuid SavedPickupId;
	FTransform SavedPickupTransform;
	double ResumeUntil = 0.;
	FGuid OriginalId;
	FGuid OriginalOrigin;
	double Created = 0.;
	double Started = 0.;
	double Next = 0.;
	int32 Step = 0;
	uint8 bComplete = 0;
};
