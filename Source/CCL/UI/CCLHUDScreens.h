#pragma once

#include "CoreMinimal.h"
#include "Core/CCLScreen.h"
#include "Core/CCLUIContext.h"
#include "CCLHUDScreens.generated.h"

DECLARE_MULTICAST_DELEGATE(FCCLHUDContentChanged);

class UCCLCombatViewModel;
class UTextBlock;
class UBorder;
class UProgressBar;

UCLASS()
class CCL_API UCCLHUDContext : public UCCLUIContext
{
	GENERATED_BODY()

public:
	void Update(FString InOverview, FString InPrompts);

public:
	UPROPERTY(Transient)
	TObjectPtr<UCCLCombatViewModel> Vitals;

	FString Overview;
	FString Prompts;
	FCCLHUDContentChanged OnChanged;
};

UCLASS()
class CCL_API UCCLDialogueContext : public UCCLUIContext
{
	GENERATED_BODY()

public:
	void Update(const FString& InSpeaker, const FString& InBody);

public:
	FString Speaker;
	FString Body;
	FCCLHUDContentChanged OnChanged;
};

UCLASS()
class CCL_API UCCLVitalsScreen : public UCCLScreen
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void OnContextBound() override;
	virtual void OnContextReleased() override;
	virtual void OnPresentationChanged() override;

public:
	FString GetDisplayedText() const;
	int32 GetRefreshCount() const { return RefreshCount; }

private:
	void Refresh();

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Values;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> StaminaBar;

	int32 RefreshCount = 0;
};

UCLASS()
class CCL_API UCCLFieldHUDScreen : public UCCLScreen
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void OnContextBound() override;
	virtual void OnContextReleased() override;
	virtual void OnPresentationChanged() override;

private:
	void Refresh();

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Overview;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Prompts;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> PromptPanel;
};

UCLASS()
class CCL_API UCCLDialogueScreen : public UCCLScreen
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void OnContextBound() override;
	virtual void OnContextReleased() override;
	virtual void OnPresentationChanged() override;

public:
	FString GetDisplayedBody() const;

private:
	void Refresh();

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Speaker;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Body;
};
