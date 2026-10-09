#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "CCLTerrainSmokeSubsystem.generated.h"

class ACCLTerrainRegion;
class ACharacter;
class UCCLTerrainChunkComponent;
class FCCLTerrainSmokeParticipant;

UCLASS()
class UCCLTerrainSmokeSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return !IsTemplate() && !bComplete; }
	virtual TStatId GetStatId() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }

private:
	bool Check(bool bCondition, const TCHAR* Message);
	bool CheckHeight(double X, double Y, double ExpectedMeters, double Tolerance = 0.08);
	bool Queue(bool bDeposit, double X, uint64 Sequence);

private:
	UPROPERTY()
	TObjectPtr<ACCLTerrainRegion> Region;

	UPROPERTY()
	TObjectPtr<ACharacter> Occupant;

	TWeakObjectPtr<UCCLTerrainChunkComponent> CancelledChunk;
	TSharedPtr<FCCLTerrainSmokeParticipant> Participant;
	FGuid Principal = FGuid(100, 200, 300, 400);
	double Started = 0.;
	double Next = 0.;
	int32 Step = 0;
	uint8 bComplete = 0;
};
