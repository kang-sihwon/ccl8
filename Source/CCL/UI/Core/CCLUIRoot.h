#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Blueprint/UserWidgetPool.h"
#include "CCLUIRoot.generated.h"

struct FCCLUIViewDefinition;
class UCCLScreen;
class UCCLUIRegistry;
class UCanvasPanel;
class UOverlay;
class UCommonActivatableWidgetContainerBase;

UCLASS()
class CCL_API UCCLUIRoot : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UCCLUIRoot(const FObjectInitializer& Initializer);
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

public:
	bool Configure(UCCLUIRegistry* Registry);
	UCCLScreen* AddScreen(const FCCLUIViewDefinition& Definition, TFunctionRef<void(UCCLScreen&)> InitializeScreen);
	void RemoveScreen(UCCLScreen* Screen);
	void ClearScreens();
	bool ContainsScreen(const UCCLScreen* Screen) const;
	const UCommonActivatableWidgetContainerBase* GetContainer(FGameplayTag Layer) const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCCLUIRegistry> Layout;

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UCanvasPanel>> Overlays;

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UOverlay>> ExtensionHosts;

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UCommonActivatableWidgetContainerBase>> Containers;

	UPROPERTY(Transient)
	FUserWidgetPool OverlayPool;
};
