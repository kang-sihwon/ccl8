#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KSIUIContext.generated.h"

// Features derive typed display contexts without introducing gameplay dependencies into UI Core.
UCLASS(BlueprintType, Blueprintable)
class KSIUI_API UKSIUIContext : public UObject
{
	GENERATED_BODY()
};
