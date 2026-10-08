#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "StateTreeTaskBase.h"
#include "CCLAgentAIController.generated.h"

UENUM()
enum class ECCLAgentExecutionPhase : uint8 { Select, Approach, Perform };

USTRUCT()
struct FCCLAgentTaskData
{
	GENERATED_BODY()

	UPROPERTY()
	double StartedTime = 0;
};

USTRUCT(meta = (DisplayName = "CCL Agent execution"))
struct FCCLAgentTask : public FStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FCCLAgentTaskData;
	FCCLAgentTask() = default;
	explicit FCCLAgentTask(ECCLAgentExecutionPhase InPhase) : Phase(InPhase) {}
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;

	UPROPERTY(EditAnywhere, Category = "Execution")
	ECCLAgentExecutionPhase Phase = ECCLAgentExecutionPhase::Select;
};

class UStateTreeAIComponent;

UCLASS()
class CCL_API ACCLAgentAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACCLAgentAIController();
	virtual void OnPossess(APawn* Pawn) override;
	virtual void OnUnPossess() override;

public:
	EStateTreeRunStatus SelectIntent();
	EStateTreeRunStatus Approach(double StartedTime);
	EStateTreeRunStatus Perform(double StartedTime);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStateTreeAIComponent> StateTree;

	FGuid OpportunityId;
	int32 Revision = 0;
	FVector Destination = FVector::ZeroVector;
	double NextDecisionTime = 0;
};
