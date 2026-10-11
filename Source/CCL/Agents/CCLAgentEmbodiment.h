#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "CCLAgentEmbodiment.generated.h"

class UCCLAgentComponent;
class FCCLLifeSimulation;
struct FCCLSimulationSnapshot;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UCCLAgentEmbodiment : public UInterface
{
	GENERATED_BODY()
};

// Implemented by an Actor that shows one Agent record in the world.
// Agent code drives presentation through this contract without knowing content classes.
class CCL_API ICCLAgentEmbodiment
{
	GENERATED_BODY()

	// 내 클래스 함수
public:
	virtual UCCLAgentComponent* GetAgentComponent() const = 0;
	virtual void SetAgentActivity(FGameplayTag Activity) = 0;
	virtual void SetAgentPublicName(const FString& Name) = 0;
};

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UCCLAgentWorldHost : public UInterface
{
	GENERATED_BODY()
};

// Implemented by the GameState or a map-placed Actor that lets a world run the Agent simulation.
// A world without a host does not start the simulation.
class CCL_API ICCLAgentWorldHost
{
	GENERATED_BODY()

	// 내 클래스 함수
public:
	// Supplies the population of a new world. A world restored from a snapshot ignores it.
	virtual bool BuildInitialPopulation(FCCLSimulationSnapshot& OutInitial, FString& Error) const = 0;
};
