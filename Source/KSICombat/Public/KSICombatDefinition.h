#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "KSICombatDefinition.generated.h"

class UKSIHitRule;

UCLASS()
class KSICOMBAT_API UKSICombatDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Trace", meta = (ClampMin = "0"))
    float Reach = 150.f;

    UPROPERTY(EditAnywhere, Category = "Trace", meta = (ClampMin = "0"))
    float Radius = 35.f;

    UPROPERTY(EditAnywhere, Category = "Policy")
    TSubclassOf<UKSIHitRule> HitRule;
};
