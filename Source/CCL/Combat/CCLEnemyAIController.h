#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "StateTreeTaskBase.h"
#include "CCLEnemyAIController.generated.h"

class UStateTreeAIComponent;
class UStateTree;

UCLASS()
class CCL_API ACCLEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ACCLEnemyAIController();
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

public:
	bool AcquireTarget();
	bool HasTarget() const;
	bool Approach();
	bool ReturnHome();
	bool StartAttack();
	bool IsActionRunning() const;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

	TWeakObjectPtr<APawn> Target;
};

UENUM()
enum class ECCLEnemyTask : uint8 { Acquire, Approach, Attack, Return };

USTRUCT()
struct FCCLEnemyTaskData
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 bStarted = 0;
};

USTRUCT(meta = (DisplayName = "CCL Enemy Action"))
struct FCCLEnemyTask : public FStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FCCLEnemyTaskData;

	FCCLEnemyTask() = default;
	explicit FCCLEnemyTask(ECCLEnemyTask InMode) : Mode(InMode) {}
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;

public:
	UPROPERTY(EditAnywhere, Category = "Task")
	ECCLEnemyTask Mode = ECCLEnemyTask::Acquire;
};
