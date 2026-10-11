#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CCLFirePresentation.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UPointLightComponent;

UCLASS()
class CCL_API ACCLFirePresentation : public AActor
{
	GENERATED_BODY()

public:
	ACCLFirePresentation();
	virtual void Tick(float DeltaSeconds) override;

private:
	void UpdateEmitter(int32 Index, UNiagaraSystem* Asset, const FVector& Position, float Rate, const FVector& Velocity);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UNiagaraSystem> FlameSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UNiagaraSystem> SmokeSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UMaterialInterface> FuelMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UMaterialInterface> CharMaterial;

private:
	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> Fuel;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> Char;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> Emitters;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> Lights;
};
