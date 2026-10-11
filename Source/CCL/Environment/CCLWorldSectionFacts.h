#pragma once
#include "KSIWorldParticipant.h"
class FKSISurfaceSimulation;
class FCCLLifeSimulation;
struct FCCLWorldSectionFacts final : IKSIWorldSectionFacts
{
    static FName TypeName() { return TEXT("CCL.WorldSectionFacts"); }
    virtual FName GetTypeName() const override { return TypeName(); }
    const FKSISurfaceSimulation* Surface = nullptr;
    const FCCLLifeSimulation* Life = nullptr;
};
