#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CCLSurfacePresentation.generated.h"

class UInstancedStaticMeshComponent;
class UDynamicMeshComponent;

UCLASS()
class CCL_API ACCLSurfacePresentation : public AActor
{
	GENERATED_BODY()
public:
	ACCLSurfacePresentation();
	virtual void Tick(float DeltaSeconds) override;

private:
	uint32 BedGeometryHash = 0;
	uint8 bBedsBuilt = 0;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> Beds;
	UPROPERTY()
	TObjectPtr<UDynamicMeshComponent> Water;
	UPROPERTY()
	TObjectPtr<UDynamicMeshComponent> Ice;
	UPROPERTY()
	TObjectPtr<UDynamicMeshComponent> Mud;
	UPROPERTY()
	TObjectPtr<UDynamicMeshComponent> Snow;
};
