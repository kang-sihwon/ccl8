#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CCLSkillDefinition.generated.h"

class UGameplayEffect;

UCLASS()
class CCL_API UCCLSkillDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Skill")
	FText Label;

	UPROPERTY(EditAnywhere, Category = "Skill", meta = (ClampMin = "1"))
	int32 PointCost = 1;

	UPROPERTY(EditAnywhere, Category = "Skill")
	TSubclassOf<UGameplayEffect> Effect;

	UPROPERTY(EditAnywhere, Category = "Skill")
	float Magnitude = 5.f;
};
