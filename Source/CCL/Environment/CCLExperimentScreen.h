#pragma once

#include "CoreMinimal.h"
#include "UI/Core/CCLScreen.h"
#include "UI/Core/CCLUIContext.h"
#include "CCLExperimentScreen.generated.h"

class ACCLExperimentPlayerController;
class ACCLExperimentDirector;
class UNativeWidgetHost;
class SWidget;
class SComboButton;
class SScrollBox;

UCLASS()
class CCL_API UCCLExperimentContext : public UCCLUIContext
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<ACCLExperimentPlayerController> Controller;
};

UCLASS()
class CCL_API UCCLExperimentScreen : public UCCLScreen
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnFocusReceived(const FGeometry& Geometry, const FFocusEvent& Event) override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	virtual FReply NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual void OnContextBound() override;
	virtual void OnContextReleased() override;

public:
	void SelectCase(FName CaseId);
	TSharedPtr<SWidget> GetCloseButton() const { return CloseButton; }
	TSharedPtr<SWidget> GetStartButton() const { return StartButton; }
	TSharedPtr<SWidget> GetRotationButton() const { return RotationButton; }
	TSharedPtr<SWidget> GetDoorButton() const { return DoorButton; }
	TSharedPtr<SWidget> GetMenuPanel() const { return MenuPanel; }
	TSharedPtr<SWidget> GetCelestialViewButton() const { return CelestialViewButton; }
	TSharedPtr<SWidget> GetCelestialView() const { return CelestialView; }
	void RevealControl(const TSharedPtr<SWidget>& Widget);

private:
	ACCLExperimentPlayerController* Controller() const;
	ACCLExperimentDirector* Director() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UNativeWidgetHost> Host;

	FName SelectedCase = TEXT("Zone_00");
	uint8 bShowCelestialView = 0;
	TSharedPtr<SScrollBox> DetailScroll;
	TSharedPtr<SWidget> CelestialViewButton;
	TSharedPtr<SWidget> CelestialView;
	TSharedPtr<SComboButton> ZoneSelector;
	TSharedPtr<SWidget> MenuPanel;
	TSharedPtr<SWidget> FirstButton;
	TSharedPtr<SWidget> CloseButton;
	TSharedPtr<SWidget> StartButton;
	TSharedPtr<SWidget> RotationButton;
	TSharedPtr<SWidget> DoorButton;
};
