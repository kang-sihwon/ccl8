#pragma once

#include "CoreMinimal.h"
#include "Input/CommonUIActionRouterBase.h"
#include "KSIUIActionRouter.generated.h"

UCLASS()
class KSIUI_API UKSIUIActionRouter : public UCommonUIActionRouterBase
{
	GENERATED_BODY()

public:
	virtual bool CanProcessNormalGameInput() const override;

public:
	void RefreshPresentationRouting();
};
