#pragma once

#include "CoreMinimal.h"
#include "KSIItemDefinition.h"
#include "CCLItemFragments.h"
#include "CCLItemDefinition.generated.h"

UCLASS(BlueprintType)
class CCL_API UCCLItemDefinition : public UKSIItemDefinition
{
	GENERATED_BODY()

	// 내 클래스 함수
public:
	// 에셋 생성 스크립트가 음식 정책을 정한다. 기존 Perishable Fragment는 바꾼다.
	UFUNCTION(BlueprintCallable, Category = "Item")
	bool ConfigurePerishable(double ShelfLifeSeconds, double MoldDelaySeconds, double MoldGrowthSeconds);
};
