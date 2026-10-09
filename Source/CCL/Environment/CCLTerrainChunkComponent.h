#pragma once

#include "CoreMinimal.h"
#include "Components/DynamicMeshComponent.h"
#include "CCLTerrainMesher.h"
#include "CCLTerrainChunkComponent.generated.h"

enum class ECCLTerrainChunkState : uint8
{
	Empty,
	Cooking,
	Ready,
	Active,
	Failed,
	Cancelled
};

UCLASS(Transient)
class CCL_API UCCLTerrainChunkComponent : public UDynamicMeshComponent
{
	GENERATED_BODY()

public:
	UCCLTerrainChunkComponent();

protected:
	virtual void FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup) override;

public:
	bool Prepare(FCCLTerrainMesh&& Mesh, FString& Error);
	bool ActivatePrepared(FString& Error);
	void Discard();
	bool OverlapsCapsule(const FVector& WorldCenter, const FVector& WorldAxis, double RadiusCm, double HalfHeightCm) const;

	ECCLTerrainChunkState GetPreparationState() const { return State; }
	const FCCLTerrainMesh& GetSourceMesh() const { return SourceMesh; }
	const FString& GetFailure() const { return Failure; }

#if WITH_DEV_AUTOMATION_TESTS
	void RejectCookForTesting() { bRejectCook = 1; }
#endif

private:
	FCCLTerrainMesh SourceMesh;
	FString Failure;
	ECCLTerrainChunkState State = ECCLTerrainChunkState::Empty;
	uint8 bDiscardResults = 0;
	uint8 bRejectCook = 0;
};
