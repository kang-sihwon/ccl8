#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CCLVegetationPresentation.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UTextRenderComponent;
struct FKSISurfaceGrid;

UCLASS()
class CCL_API ACCLVegetationPresentation : public AActor
{
	GENERATED_BODY()

public:
	ACCLVegetationPresentation();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

public:
	UFUNCTION(BlueprintCallable, CallInEditor)
	void RefreshPreview();

	int32 GetRenderedPatchCount() const { return RenderedPatchCount; }

private:
	void Append(const FKSISurfaceGrid& Grid);
	void Clear();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<UMaterialInterface>> StateMaterials;

private:
	UPROPERTY()
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Batches;

	UPROPERTY()
	TArray<TObjectPtr<UTextRenderComponent>> Labels;

	uint32 LastHash = 0;
	int32 RenderedPatchCount = 0;
	uint8 bHasState = 0;
};
