#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Blueprint/UserWidgetPool.h"
#include "GameplayTagContainer.h"
#include "KSIUIRoot.generated.h"

struct FKSIUIViewDefinition;
class UKSIScreen;
class UKSIUIRegistry;
class UCanvasPanel;
class UOverlay;
class UCommonActivatableWidgetContainerBase;

UCLASS()
class KSIUI_API UKSIUIRoot : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UKSIUIRoot(const FObjectInitializer& Initializer);
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

public:
	bool Configure(UKSIUIRegistry* Registry);
	UKSIScreen* AddScreen(const FKSIUIViewDefinition& Definition, TFunctionRef<void(UKSIScreen&)> InitializeScreen);
	void RemoveScreen(UKSIScreen* Screen);
	void ClearScreens();
	bool ContainsScreen(const UKSIScreen* Screen) const;
	const UCommonActivatableWidgetContainerBase* GetContainer(FGameplayTag Layer) const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UKSIUIRegistry> Layout;

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UCanvasPanel>> Overlays;

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UOverlay>> ExtensionHosts;

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UCommonActivatableWidgetContainerBase>> Containers;

	UPROPERTY(Transient)
	FUserWidgetPool OverlayPool;
};
