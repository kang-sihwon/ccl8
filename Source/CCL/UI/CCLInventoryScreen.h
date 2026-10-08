#pragma once

#include "CoreMinimal.h"
#include "Core/CCLScreen.h"
#include "Core/CCLUIContext.h"
#include "CCLInventoryScreen.generated.h"

class ACCLPlayerController;
class ACCLCharacter;
class SCCLInventoryWidget;
class UNativeWidgetHost;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

UCLASS()
class CCL_API UCCLInventoryContext : public UCCLUIContext
{
	GENERATED_BODY()

public:
	virtual void BeginDestroy() override;

public:
	void Initialize(ACCLPlayerController* InController);
	void UpdatePreview();
	void Release();
	ACCLPlayerController* GetController() const;
	UTextureRenderTarget2D* GetPreview() const { return Preview; }
	bool IsUsable() const;
	bool HasCapture() const { return Camera != nullptr; }

public:
	FGuid SelectedEquipment;
	int32 SelectedItem = 0;

private:
	UPROPERTY(Transient)
	TObjectPtr<USceneCaptureComponent2D> Camera;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> Preview;

	TWeakObjectPtr<ACCLPlayerController> Controller;
	TWeakObjectPtr<ACCLCharacter> Pawn;
};

UCLASS()
class CCL_API UCCLInventoryScreen : public UCCLScreen
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	virtual FReply NativeOnFocusReceived(const FGeometry& Geometry, const FFocusEvent& Event) override;
	virtual void OnContextBound() override;
	virtual void OnContextReleased() override;

public:
	TSharedPtr<SCCLInventoryWidget> GetBody() const { return Body; }

private:
	UPROPERTY(Transient)
	TObjectPtr<UNativeWidgetHost> Host;

	TSharedPtr<SCCLInventoryWidget> Body;
};
