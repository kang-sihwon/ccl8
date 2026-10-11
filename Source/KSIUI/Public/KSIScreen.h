#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "KSIUITypes.h"
#include "KSIUIPresentation.h"
#include "KSIScreen.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FKSIScreenCloseRequested, FKSIUIViewHandle);

class UKSIUIContext;

UCLASS(Blueprintable)
class KSIUI_API UKSIScreen : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UKSIScreen(const FObjectInitializer& Initializer);
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual bool NativeOnHandleBackAction() override;

public:
	void BindContext(UKSIUIContext* Context, FKSIUIViewHandle Handle, EKSIUIInputPolicy Policy);
	void ReleaseContext();
	void RequestClose();
	void ApplyPresentation(const FKSIUIViewPresentation& State, float FadeSeconds);
	void TickManagedPresentation(float DeltaTime);
	const FKSIUIViewPresentation& GetPresentation() const { return Presentation; }
	bool IsPresentationUpdating() const { return Presentation.bUpdatesEnabled != 0; }
	bool IsPresentationInteractive() const { return bPresentationInteractive != 0; }
	UKSIUIContext* GetContext() const { return ViewContext; }
	FKSIUIViewHandle GetViewHandle() const { return ViewHandle; }
	EKSIUIInputPolicy GetInputPolicy() const { return InputPolicy; }

protected:
	virtual void OnContextBound();
	virtual void OnContextReleased();
	virtual void OnPresentationChanged();
	virtual void OnManagedTick(float DeltaTime);

private:
	void RefreshBackBinding();
	void UpdatePresentationVisuals();

public:
	FKSIScreenCloseRequested OnCloseRequested;

private:
	UPROPERTY(Transient)
	TObjectPtr<UKSIUIContext> ViewContext;

	FKSIUIViewHandle ViewHandle;
	FUIActionBindingHandle BackBinding;
	EKSIUIInputPolicy InputPolicy = EKSIUIInputPolicy::Inherit;
	FKSIUIViewPresentation Presentation;
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
