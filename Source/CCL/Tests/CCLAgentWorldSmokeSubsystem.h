#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CCLAgentWorldSmokeSubsystem.generated.h"

UCLASS()
class UCCLAgentWorldSmokeSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual TStatId GetStatId() const override;
	virtual void Tick(float DeltaTime) override;

private:
	float Elapsed = 0;
	uint8 bComplete = 0;
};
