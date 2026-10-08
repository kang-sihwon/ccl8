#pragma once
#include "CoreMinimal.h"
#include "Core/CCLScreen.h"
#include "Core/CCLUIContext.h"
#include "CCLMapScreen.generated.h"

UCLASS()
class CCL_API UCCLMapContext : public UCCLUIContext
{
	GENERATED_BODY()

public:
	uint8 bFullMap = 0;
};

UCLASS()
class CCL_API UCCLMapScreen : public UCCLScreen
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void OnContextBound() override;
	virtual void OnContextReleased() override;
	virtual void OnManagedTick(float DeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
		FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;
private:
	float RefreshTime = 0;
	FCCLUIPresentationHandle HUDHidden;
};
