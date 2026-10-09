#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CCLSnowAnimInstance.generated.h"

// Native post process keeps the input locomotion pose and solves only the two feet.
UCLASS(Transient)
class CCL_API UCCLSnowAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
};
