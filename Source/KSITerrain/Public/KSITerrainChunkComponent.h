#pragma once

#include "CoreMinimal.h"
#include "Components/DynamicMeshComponent.h"
#include "KSITerrainMesher.h"
#include "KSITerrainChunkComponent.generated.h"

enum class EKSITerrainChunkState : uint8
{
	Empty,
	Cooking,
	Ready,
	Active,
	Failed,
	Cancelled
};

UCLASS(Transient)
class KSITERRAIN_API UKSITerrainChunkComponent : public UDynamicMeshComponent
{
	GENERATED_BODY()

public:
	UKSITerrainChunkComponent();
	virtual bool DoCustomNavigableGeometryExport(FNavigableGeometryExport& Export) const override;

protected:
	virtual void FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup) override;

public:
	bool Prepare(FKSITerrainMesh&& Mesh, FString& Error);
	bool ActivatePrepared(FString& Error);
	void Discard();
	bool OverlapsCapsule(const FVector& WorldCenter, const FVector& WorldAxis, double RadiusCm, double HalfHeightCm) const;

	EKSITerrainChunkState GetPreparationState() const { return State; }
	const FKSITerrainMesh& GetSourceMesh() const { return SourceMesh; }
	const FString& GetFailure() const { return Failure; }

#if WITH_DEV_AUTOMATION_TESTS
	void RejectCookForTesting() { bRejectCook = 1; }
#endif

private:
	FKSITerrainMesh SourceMesh;
	FString Failure;
	EKSITerrainChunkState State = EKSITerrainChunkState::Empty;
	uint8 bDiscardResults = 0;
	uint8 bRejectCook = 0;
};
