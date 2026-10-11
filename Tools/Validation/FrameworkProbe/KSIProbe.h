#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "KSIProbe.generated.h"

UCLASS()
class UKSIProbe : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
private:
    FGuid ViewId;
    FGuid PresentationId;
    int32 Step = 0;
    float StepSeconds = 0.f;
    uint8 bPassed = 1;
    uint8 bFinished = 0;
};
