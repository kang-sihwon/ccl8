#pragma once

#include "CoreMinimal.h"
#include "KSIScreen.h"
#include "KSIUIContext.h"
#include "CCLExperimentScreen.generated.h"

class ACCLExperimentPlayerController;
class ACCLExperimentDirector;
class UNativeWidgetHost;
class SWidget;
class SComboButton;
class SScrollBox;
class SEditableTextBox;

UCLASS()
class CCL_API UCCLExperimentContext : public UKSIUIContext
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<ACCLExperimentPlayerController> Controller;
};

UCLASS()
class CCL_API UCCLExperimentScreen : public UKSIScreen
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnFocusReceived(const FGeometry& Geometry, const FFocusEvent& Event) override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	virtual FReply NativeOnKeyUp(const FGeometry& Geometry, const FKeyEvent& Event) override;
	virtual void OnManagedTick(float DeltaTime) override;
	virtual FReply NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual void OnContextBound() override;
	virtual void OnContextReleased() override;

public:
	void SelectCase(FName CaseId);
	TSharedPtr<SWidget> GetCloseButton() const { return CloseButton; }
	TSharedPtr<SWidget> GetStartButton() const { return StartButton; }
	TSharedPtr<SWidget> GetScaleStopButton() const { return ScaleStopButton; }
	TSharedPtr<SWidget> GetScaleDecreaseButton() const { return ScaleDecreaseButton; }
	TSharedPtr<SWidget> GetScaleIncreaseButton() const { return ScaleIncreaseButton; }
	TSharedPtr<SEditableTextBox> GetScaleInput() const { return ScaleInput; }
	TSharedPtr<SWidget> GetScaleApplyButton() const { return ScaleApplyButton; }
	TSharedPtr<SWidget> GetRotationButton() const { return RotationButton; }
	TSharedPtr<SWidget> GetDoorButton() const { return DoorButton; }
	TSharedPtr<SWidget> GetFoodAddButton() const { return FoodAddButton; }
	TSharedPtr<SWidget> GetFoodDropButton() const { return FoodDropButton; }
	TSharedPtr<SWidget> GetFoodCollectButton() const { return FoodCollectButton; }
	TSharedPtr<SWidget> GetFoodWeatherButton() const { return FoodWeatherButton; }
	TSharedPtr<SWidget> GetMenuPanel() const { return MenuPanel; }
	TSharedPtr<SWidget> GetCelestialViewButton() const { return CelestialViewButton; }
	TSharedPtr<SWidget> GetCelestialView() const { return CelestialView; }
	TSharedPtr<SEditableTextBox> GetObservationScaleInput() const { return ObservationScaleInput; }
	TSharedPtr<SWidget> GetObservationApplyButton() const { return ObservationApplyButton; }
	TSharedPtr<SWidget> GetObservationFollowButton() const { return ObservationFollowButton; }
	void RevealControl(const TSharedPtr<SWidget>& Widget);
	void RevealFoodControls();
	void RevealBiologyControls();
	TSharedPtr<SWidget> GetBiologyAgeButton() const { return BiologyAgeButton; }
	TSharedPtr<SWidget> GetBiologyDamageButton() const { return BiologyDamageButton; }
	void RevealEcologyControls();
	TSharedPtr<SWidget> GetEcologyFeedButton() const { return EcologyFeedButton; }

private:
	void ApplyTimeScale();
	void ApplyObservationScale();
	void ApplyScale(double Scale);
	void AdjustScale(double Delta, bool bStop = false);
	void RefreshScaleInputs(bool bForce = false);
	bool IsObserving() const;
	double CurrentScale() const;
	ACCLExperimentPlayerController* Controller() const;
	ACCLExperimentDirector* Director() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UNativeWidgetHost> Host;

	FName SelectedCase = TEXT("Zone_00");
	uint8 bShowCelestialView = 0;
	double DisplayedScale = -1.;
	bool bDisplayedObservation = false;
	TSharedPtr<SScrollBox> DetailScroll;
	TSharedPtr<SWidget> CelestialViewButton;
	TSharedPtr<SWidget> CelestialView;
	TSharedPtr<SEditableTextBox> ObservationScaleInput;
	TSharedPtr<SWidget> ObservationApplyButton;
	TSharedPtr<SWidget> ObservationFollowButton;
	TSharedPtr<SComboButton> ZoneSelector;
	TSharedPtr<SWidget> MenuPanel;
	TSharedPtr<SWidget> FirstButton;
	TSharedPtr<SWidget> CloseButton;
	TSharedPtr<SWidget> StartButton;
	TSharedPtr<SWidget> ScaleStopButton;
	TSharedPtr<SWidget> ScaleDecreaseButton;
	TSharedPtr<SWidget> ScaleIncreaseButton;
	TSharedPtr<SEditableTextBox> ScaleInput;
	TSharedPtr<SWidget> ScaleApplyButton;
	TSharedPtr<SWidget> RotationButton;
	TSharedPtr<SWidget> DoorButton;
	TSharedPtr<SWidget> FoodAddButton;
	TSharedPtr<SWidget> FoodWeatherButton;
	TSharedPtr<SWidget> FoodDropButton;
	TSharedPtr<SWidget> FoodCollectButton;
	TSharedPtr<SWidget> FoodPanel;
	TSharedPtr<SWidget> BiologyPanel;
	TSharedPtr<SWidget> BiologyAgeButton;
	TSharedPtr<SWidget> BiologyDamageButton;
	TSharedPtr<SWidget> EcologyPanel;
	TSharedPtr<SWidget> EcologyFeedButton;
};
