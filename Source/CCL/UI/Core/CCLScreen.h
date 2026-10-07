#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "CCLUITypes.h"
#include "CCLScreen.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FCCLScreenCloseRequested, FCCLUIViewHandle);

class UCCLUIContext;

UCLASS(Blueprintable)
class CCL_API UCCLScreen : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UCCLScreen(const FObjectInitializer& Initializer);
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override;

public:
	void BindContext(UCCLUIContext* Context, FCCLUIViewHandle Handle, ECCLUIInputPolicy Policy);
	void ReleaseContext();
	void RequestClose();
	UCCLUIContext* GetContext() const { return ViewContext; }
	FCCLUIViewHandle GetViewHandle() const { return ViewHandle; }
	ECCLUIInputPolicy GetInputPolicy() const { return InputPolicy; }

protected:
	virtual void OnContextBound();
	virtual void OnContextReleased();

private:
	void RefreshBackBinding();

public:
	FCCLScreenCloseRequested OnCloseRequested;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCCLUIContext> ViewContext;

	FCCLUIViewHandle ViewHandle;
	FUIActionBindingHandle BackBinding;
	ECCLUIInputPolicy InputPolicy = ECCLUIInputPolicy::Inherit;
};
