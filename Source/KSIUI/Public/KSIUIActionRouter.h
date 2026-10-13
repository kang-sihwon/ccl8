#pragma once

#include "CoreMinimal.h"
#include "Input/CommonUIActionRouterBase.h"
#include "KSIUIActionRouter.generated.h"

UCLASS()
class KSIUI_API UKSIUIActionRouter : public UCommonUIActionRouterBase
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool CanProcessNormalGameInput() const override;

protected:
	virtual void ApplyUIInputConfig(const FUIInputConfig& NewConfig, bool bForceRefresh) override;

public:
	void RefreshPresentationRouting();

private:
	void HandleFocusChanging(const FFocusEvent& Event, const FWeakWidgetPath& OldPath,
		const TSharedPtr<SWidget>& OldWidget, const FWidgetPath& NewPath, const TSharedPtr<SWidget>& NewWidget);
	bool TickPendingRefresh(float DeltaTime);
	bool IsInputTargetReady() const;

private:
	FTSTicker::FDelegateHandle RefreshTickHandle;
	FDelegateHandle FocusChangingHandle;
	uint8 bRefreshPending = 1;
	uint8 bShuttingDown = 0;
};
