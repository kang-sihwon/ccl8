#pragma once
#include "CoreMinimal.h"
#include "CCLFoodState.h"
#include "CCLItemState.generated.h"

USTRUCT()
struct CCL_API FCCLItemState
{
	GENERATED_BODY()

	UPROPERTY()
	int32 LoadedAmmo = 0;
	UPROPERTY()
	FCCLFoodState Food;
};
