#pragma once

#include "CoreMinimal.h"
#include "Input/CommonUIActionRouterBase.h"
#include "CCLUIActionRouter.generated.h"

UCLASS()
class CCL_API UCCLUIActionRouter : public UCommonUIActionRouterBase
{
	GENERATED_BODY()

public:
	virtual bool CanProcessNormalGameInput() const override;

public:
	void RefreshPresentationRouting();
};
