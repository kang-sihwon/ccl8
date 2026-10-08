#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CCLIntegrationSmokeSubsystem.generated.h"

UCLASS()
class UCCLIntegrationSmokeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
private:
	bool Check(bool Condition, const TCHAR* Message);
	void Capture(const FString& Name);
	int32 Step = 0;
	float Elapsed = 0;
	uint8 bPassed = 1;
};
