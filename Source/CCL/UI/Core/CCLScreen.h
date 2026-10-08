#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "CCLUITypes.h"
#include "CCLUIPresentation.h"
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
	void ApplyPresentation(const FCCLUIViewPresentation& State, float FadeSeconds);
	void TickManagedPresentation(float DeltaTime);
	const FCCLUIViewPresentation& GetPresentation() const { return Presentation; }
	bool IsPresentationUpdating() const { return Presentation.bUpdatesEnabled != 0; }
	bool IsPresentationInteractive() const { return bPresentationInteractive != 0; }
	UCCLUIContext* GetContext() const { return ViewContext; }
	FCCLUIViewHandle GetViewHandle() const { return ViewHandle; }
	ECCLUIInputPolicy GetInputPolicy() const { return InputPolicy; }

protected:
	virtual void OnContextBound();
	virtual void OnContextReleased();
	virtual void OnPresentationChanged();
	virtual void OnManagedTick(float DeltaTime);

private:
	void RefreshBackBinding();
	void UpdatePresentationVisuals();

public:
	FCCLScreenCloseRequested OnCloseRequested;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCCLUIContext> ViewContext;

	FCCLUIViewHandle ViewHandle;
	FUIActionBindingHandle BackBinding;
	ECCLUIInputPolicy InputPolicy = ECCLUIInputPolicy::Inherit;
	FCCLUIViewPresentation Presentation;
	ESlateVisibility OriginalVisibility = ESlateVisibility::SelfHitTestInvisible;
	float OriginalOpacity = 1.f;
	float CurrentOpacity = 1.f;
	float StartOpacity = 1.f;
	float TargetOpacity = 1.f;
	float FadeDuration = 0.f;
	float FadeElapsed = 0.f;
	uint8 bOriginalEnabled = 1;
	uint8 bPresentationInteractive = 1;
};
