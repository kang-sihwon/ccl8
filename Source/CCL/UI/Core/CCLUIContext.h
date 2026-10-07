#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CCLUIContext.generated.h"

// Features derive typed display contexts without introducing gameplay dependencies into UI Core.
UCLASS(BlueprintType, Blueprintable)
class CCL_API UCCLUIContext : public UObject
{
	GENERATED_BODY()
};
