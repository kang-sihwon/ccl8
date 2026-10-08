#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CCLAgentComponent.generated.h"

UCLASS(ClassGroup = Agent, meta = (BlueprintSpawnableComponent))
class CCL_API UCCLAgentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCCLAgentComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type Reason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;

public:
	UPROPERTY(EditAnywhere, Replicated)
	FGuid AgentId;
};
