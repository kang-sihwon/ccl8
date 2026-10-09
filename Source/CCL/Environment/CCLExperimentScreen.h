#pragma once

#include "CoreMinimal.h"
#include "UI/Core/CCLScreen.h"
#include "UI/Core/CCLUIContext.h"
#include "CCLExperimentScreen.generated.h"

class ACCLExperimentPlayerController;
class ACCLExperimentDirector;
class UNativeWidgetHost;
class SWidget;

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
	virtual void OnContextBound() override;
	virtual void OnContextReleased() override;

public:
	void SelectCase(FName CaseId) { SelectedCase = CaseId; }
	TSharedPtr<SWidget> GetStartButton() const { return StartButton; }

private:
	ACCLExperimentPlayerController* Controller() const;
	ACCLExperimentDirector* Director() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UNativeWidgetHost> Host;

	FName SelectedCase = TEXT("Zone_00");
	TSharedPtr<SWidget> FirstButton;
	TSharedPtr<SWidget> StartButton;
};
