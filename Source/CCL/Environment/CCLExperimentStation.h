#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CCLExperimentStation.generated.h"

class UCCLExperimentDefinition;
class UTextRenderComponent;

UCLASS()
class CCL_API ACCLExperimentStation : public AActor
{
	GENERATED_BODY()

public:
	ACCLExperimentStation();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

public:
	UFUNCTION(BlueprintCallable, CallInEditor)
	void RefreshLabel();

public:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	TObjectPtr<UCCLExperimentDefinition> Definition;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> State;
};
