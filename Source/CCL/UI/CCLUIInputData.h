#pragma once

#include "CoreMinimal.h"
#include "CommonInputBaseTypes.h"
#include "CCLUIInputData.generated.h"

class UInputMappingContext;

UCLASS()
class CCL_API UCCLUIInputData : public UCommonUIInputData
{
	GENERATED_BODY()

public:
	UCCLUIInputData();

public:
	UInputMappingContext* GetMapping() const { return Mapping; }

private:
	UPROPERTY()
	TObjectPtr<UInputMappingContext> Mapping;
};
