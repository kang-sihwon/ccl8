#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CCLWeatherPresentation.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class UVolumetricCloudComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS()
class CCL_API ACCLWeatherPresentation : public AActor
{
	GENERATED_BODY()

public:
	ACCLWeatherPresentation();
	virtual void Tick(float DeltaSeconds) override;
	static float GetCloudFraction(const UWorld* World);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UNiagaraSystem> RainSystem;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UNiagaraSystem> SnowSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UMaterialInterface> CloudMaterial;

private:
	UPROPERTY()
	TObjectPtr<UVolumetricCloudComponent> Clouds;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CloudInstance;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> Rain;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> Snow;
};
