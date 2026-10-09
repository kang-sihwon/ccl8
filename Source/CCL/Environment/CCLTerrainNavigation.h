#pragma once

#include "CoreMinimal.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "CCLTerrainNavigation.generated.h"

class UBoxComponent;

UCLASS()
class CCL_API ACCLTerrainNavigationBounds : public ANavMeshBoundsVolume
{
	GENERATED_BODY()

public:
	ACCLTerrainNavigationBounds();
	void Configure(const FBox& Bounds);

private:
	UPROPERTY()
	TObjectPtr<UBoxComponent> BoundsComponent;
};

class CCL_API FCCLTerrainNavigation
{
public:
	static bool IsRouteReady(const UWorld* World, const FVector& Start, const FVector& End);
};
