#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CCLCombatSmokeSubsystem.generated.h"

class ACCLPlayerController;
class ACCLCharacter;
class ACCLEnemyCharacter;
class UCCLAbilitySystemComponent;

UCLASS()
class CCL_API UCCLCombatSmokeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

public:
	void RegisterDriver(ACCLPlayerController* Controller);
	void ExecuteClientStep(int32 Step);

private:
	void PrepareStep();
	void Advance();
	void Check(bool bCondition, const TCHAR* Message);
	void RunPolicyChecks();

private:
	TWeakObjectPtr<ACCLPlayerController> Driver;
	TWeakObjectPtr<ACCLEnemyCharacter> Enemy;
	TWeakObjectPtr<ACCLCharacter> OldPawn;
	TWeakObjectPtr<UCCLAbilitySystemComponent> OriginalASC;
	TWeakObjectPtr<APawn> WitnessPawn;
	double Started = 0.;
	double StepStarted = 0.;
	double NextStepAt = 0.;
	int32 Step = 0;
	int32 Substep = 0;
	int32 AbilityCount = 0;
	uint8 bRegistered = 0;
	uint8 bFailed = 0;
	uint8 bComplete = 0;
};
