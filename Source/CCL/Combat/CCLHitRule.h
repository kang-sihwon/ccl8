#pragma once

#include "CoreMinimal.h"
#include "KSIHitRule.h"
#include "CCLHitRule.generated.h"

USTRUCT()
struct FCCLHitSourceFacts
{
    GENERATED_BODY()

    UPROPERTY()
    int32 Team = INDEX_NONE;
};

UCLASS()
class CCL_API UCCLDuelHitRule : public UKSIHitRule
{
    GENERATED_BODY()

public:
    virtual FKSIHitResolution Resolve(const FKSIHitContext& Context) const override;
    virtual void Commit(const FKSIHitContext& Context, const FKSIHitResolution& Resolution) const override;
};
