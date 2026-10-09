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
	static FKey GetBackKey(const UObject* WorldContext);

	static const TCHAR* GetBackKeyLabel(const UObject* WorldContext);

	UInputMappingContext* GetMapping(const UObject* WorldContext) const;

private:
	UPROPERTY()
	TObjectPtr<UInputMappingContext> Mapping;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> PIEMapping;
};
