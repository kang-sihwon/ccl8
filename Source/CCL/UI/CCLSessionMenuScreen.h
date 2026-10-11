#pragma once

#include "CoreMinimal.h"
#include "KSIScreen.h"
#include "KSIUIContext.h"
#include "CCLSessionMenuScreen.generated.h"

class UCCLGameInstance;
class UNativeWidgetHost;
class SEditableTextBox;

UCLASS()
class CCL_API UCCLSessionMenuContext : public UKSIUIContext
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<UCCLGameInstance> Session;
	TWeakObjectPtr<APlayerController> Controller;
	uint8 bInCampaign = 0;
};

UCLASS()
class CCL_API UCCLSessionMenuScreen : public UKSIScreen
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnFocusReceived(const FGeometry& Geometry, const FFocusEvent& Event) override;
	virtual bool NativeOnHandleBackAction() override;
	virtual void OnContextBound() override;
	virtual void OnContextReleased() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UNativeWidgetHost> Host;

	TSharedPtr<SEditableTextBox> AddressBox;
	TSharedPtr<SWidget> FirstButton;
};
