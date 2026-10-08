#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CCLProjectileSmokeSubsystem.generated.h"

class ACCLHealthTarget;

UCLASS()
class UCCLProjectileSmokeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual TStatId GetStatId() const override;
	virtual void Tick(float DeltaTime) override;

private:
	void Check(bool bPassed, const TCHAR* Message);

private:
	UPROPERTY()
	TObjectPtr<ACCLHealthTarget> Source;

	UPROPERTY()
	TObjectPtr<ACCLHealthTarget> Target;

	float Elapsed = 0;
	uint8 bStarted = 0;
	uint8 bFinished = 0;
	uint8 bFailed = 0;
};
