#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CCLUISmokeSubsystem.generated.h"

class ACCLPlayerController;

UCLASS()
class CCL_API UCCLUISmokeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	bool Check(bool Condition, const TCHAR* Message);
	void Capture(const TCHAR* Name);
	bool CheckEquipment(ACCLPlayerController* PC);
	int32 Step = 0;
	FGuid Equipment;
	FGuid Potion;
	double Next = 0.;
	double Started = 0.;
	uint8 bComplete = 0;
};
