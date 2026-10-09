#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CCLEnvironmentView.h"
#include "CCLWorldEnvironmentPresentation.generated.h"

class ADirectionalLight;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;

UCLASS()
class CCL_API ACCLWorldEnvironmentPresentation : public AActor
{
	GENERATED_BODY()

public:
	ACCLWorldEnvironmentPresentation();
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void EndPlay(EEndPlayReason::Type Reason) override;

public:
	UFUNCTION(BlueprintCallable, CallInEditor)
	void RefreshPreview();

	int32 GetSurfaceComponentCount() const { return Pieces.Num(); }
	uint64 GetDisplayedRevision() const { return DisplayedRevision; }

private:
	void RebuildSurfaces(const FCCLEnvironmentView& View);
	void ClearSurfaces();
	void AddPiece(const FCCLSurfacePatch& Patch, const FVector2D& Minimum, const FVector2D& Maximum);

#if WITH_EDITOR
public:
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
#endif

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<ADirectionalLight> Sun;

	// Presentation scale only. Physical irradiance remains in W/m2 in the replicated view.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ReferenceSunIntensity = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UStaticMesh> SolidMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UStaticMesh> GlassMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UMaterialInterface> SolidMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UMaterialInterface> GlassMaterial;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Pieces;

	FGuid DisplayedEpoch;
	uint64 DisplayedRevision = 0;
};
