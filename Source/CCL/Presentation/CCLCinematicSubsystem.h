#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "KSIUIPresentation.h"
#include "KSIScreen.h"
#include "KSIUIContext.h"
#include "CCLCinematicSubsystem.generated.h"

class ACameraActor;
class UAudioComponent;
class USoundWaveProcedural;
class APointLight;

UCLASS()
class CCL_API UCCLCinematicContext : public UKSIUIContext
{
	GENERATED_BODY()

public:
	FString Title;
};

UCLASS()
class CCL_API UCCLCinematicScreen : public UKSIScreen
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
		FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;
};

// One local presentation owner. A stale handle cannot cancel a later presentation.
UCLASS()
class CCL_API UCCLCinematicSubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return !IsTemplate(); }
	FGuid Play(APlayerController* Controller, FVector Focus, const FString& Title, float Duration = 4);
	bool Cancel(FGuid Handle);
	bool IsActive() const { return Active.IsValid(); }
	FGuid GetActiveHandle() const { return Active; }
private:
	TWeakObjectPtr<APlayerController> Observer;
	TWeakObjectPtr<AActor> PreviousTarget;
	TWeakObjectPtr<APawn> InitialPawn;
	UPROPERTY()
	TObjectPtr<ACameraActor> Camera;
	UPROPERTY()
	TObjectPtr<UAudioComponent> Audio;
	UPROPERTY()
	TObjectPtr<USoundWaveProcedural> Sound;
	UPROPERTY()
	TObjectPtr<APointLight> Light;
	FGuid Active;
	FKSIUIPresentationHandle UIHandle;
	FKSIUIViewHandle Overlay;
	FVector FocusPoint = FVector::ZeroVector;
	float Elapsed = 0;
	float Seconds = 4;
};
