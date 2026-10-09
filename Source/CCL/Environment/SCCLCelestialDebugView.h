#pragma once

#include "CoreMinimal.h"
#include "CCLCelestialSystem.h"
#include "CCLWorldEnvironmentState.h"
#include "Widgets/SLeafWidget.h"

// An inspection camera over published simulation state. It never advances world time.
class SCCLCelestialDebugView : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SCCLCelestialDebugView) {}
		SLATE_ARGUMENT(UWorld*, World)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args);
	virtual void Tick(const FGeometry& Geometry, double CurrentTime, float DeltaTime) override;
	virtual FVector2D ComputeDesiredSize(float Scale) const override { return FVector2D(620, 600); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
		FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bEnabled) const override;
	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override;

private:
	TWeakObjectPtr<UWorld> World;
	FCCLReplicatedWorldTime Published;
	FCCLCelestialSystem System;
	TArray<FCCLCelestialBodyState> States;
	TArray<TArray<FVector3d>> Orbits;
	FGuid CachedEpoch;
	uint64 CachedRevision = MAX_uint64;
	uint8 bReady = 0;
	double Yaw = 35.;
	double Pitch = 40.;
	double Zoom = 1.;
};
