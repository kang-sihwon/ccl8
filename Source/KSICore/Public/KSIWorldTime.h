#pragma once

#include "CoreMinimal.h"
struct KSICORE_API FKSIWorldStep
{
    FGuid Ticket;
    uint64 StepId = 0;
    double GameFromSeconds = 0;
    double GameToSeconds = 0;
    double WorldFromSeconds = 0;
    double WorldToSeconds = 0;
};

struct KSICORE_API FKSIWorldTimeView
{
    FGuid WorldId;
    uint64 CompletedStepId = 0;
    double GameSeconds = 0.;
    double WorldSeconds = 0.;
};

class KSICORE_API IKSIWorldTimeSource
{
public:
    virtual ~IKSIWorldTimeSource() = default;
    // Returns only committed time. False means no running world is available.
    virtual bool ReadWorldTime(FKSIWorldTimeView& OutTime) const = 0;
};
