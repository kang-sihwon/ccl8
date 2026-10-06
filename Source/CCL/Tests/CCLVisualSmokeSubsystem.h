#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CCLVisualSmokeSubsystem.generated.h"

class ACCLCharacter;

/** Opt-in rendered combat snapshots; never created in Shipping or Test. */
UCLASS()
class UCCLVisualSmokeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	void Capture(const TCHAR* Name);

private:
	TWeakObjectPtr<ACCLCharacter> Character;
	double NextStepAt = 0.;
	int32 Step = 0;
};
