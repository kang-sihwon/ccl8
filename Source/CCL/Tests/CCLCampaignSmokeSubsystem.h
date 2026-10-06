#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CCLCampaignSmokeSubsystem.generated.h"

class ACCLPlayerController;
class ACCLCharacter;
class ACCLEnemyCharacter;

UCLASS()
class CCL_API UCCLCampaignSmokeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

public:
	void RegisterDriver(ACCLPlayerController* Controller);
	void ExecuteClientStep(int32 Step);
	void ExecuteProgressionStep(int32 Step, FGuid EntryId);

private:
	void PrepareEnemy(ACCLEnemyCharacter* Enemy);
	void TickProgression();
	bool CheckProgressionPersistence();
	float ProbeDamage();
	bool Check(bool bCondition, const TCHAR* Description);

private:
	TWeakObjectPtr<ACCLPlayerController> Driver;
	TWeakObjectPtr<ACCLEnemyCharacter> Target;
	TWeakObjectPtr<ACCLCharacter> OldPawn;
	FVector NavigationStart = FVector::ZeroVector;
	double Started = 0.;
	double NextInputAt = 0.;
	double NextStageAt = 0.;
	int32 Stage = 0;
	uint8 bRegistered = 0;
	uint8 bAttacking = 0;
	uint8 bClientReported = 0;
	uint8 bComplete = 0;
	uint8 bFailed = 0;
	int32 CapturedPhase = -1;
	int32 ProgressionStep = 0;
	FGuid EquipmentId;
	FGuid PotionId;
	uint8 bProgressionReady = 0;
};
