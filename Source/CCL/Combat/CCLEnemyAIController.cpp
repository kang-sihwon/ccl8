#include "CCLEnemyAIController.h"

#include "CCLEnemyCharacter.h"
#include "CCLFighterComponent.h"
#include "CCLCombatDefinition.h"
#include "CCLCharacter.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "Components/StateTreeAIComponent.h"
#include "StateTree.h"
#include "StateTreeExecutionContext.h"
#include "EngineUtils.h"
#include "Navigation/PathFollowingComponent.h"

ACCLEnemyAIController::ACCLEnemyAIController()
{
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTree"));
	StateTreeComponent->SetStartLogicAutomatically(false);
}

void ACCLEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	auto* Tree = LoadObject<UStateTree>(nullptr, TEXT("/Game/Combat/ST_MeleeEnemy.ST_MeleeEnemy"));

	if (Tree)
	{
		StateTreeComponent->SetStateTree(Tree);
		StateTreeComponent->StartLogic();
	}
}

void ACCLEnemyAIController::OnUnPossess()
{
	StateTreeComponent->StopLogic(TEXT("Unpossessed"));
	Target.Reset();
	Super::OnUnPossess();
}

bool ACCLEnemyAIController::AcquireTarget()
{
	APawn* Closest = nullptr;
	const auto* Enemy = Cast<ACCLEnemyCharacter>(GetPawn());

	if (!Enemy || Enemy->IsDead())
	{
		return false;
	}
	float Best = FMath::Square(Enemy->DetectionRadius);

	for (TActorIterator<ACCLCharacter> It(GetWorld()); It; ++It)
	{
		if (It->IsDead() || !It->GetController())
		{
			continue;
		}

		const float Distance = FVector::DistSquared(It->GetActorLocation(), Enemy->GetHome());

		if (Distance < Best)
		{
			Best = Distance;
			Closest = *It;
		}
	}

	Target = Closest;
	return HasTarget();
}

bool ACCLEnemyAIController::HasTarget() const
{
	const auto* ControlledPawn = Cast<ACCLCharacter>(Target.Get());
	const auto* Enemy = Cast<ACCLEnemyCharacter>(GetPawn());
	return ControlledPawn && !ControlledPawn->IsDead() && ControlledPawn->GetController() && Enemy && !Enemy->IsDead() &&
	       FVector::DistSquared(ControlledPawn->GetActorLocation(), Enemy->GetHome()) < FMath::Square(Enemy->LeashRadius);
}

bool ACCLEnemyAIController::Approach()
{
	if (!HasTarget())
	{
		return false;
	}

	SetFocus(Target.Get());

	if (IsActionRunning())
	{
		StopMovement();
		return false;
	}

	const float Distance = FVector::Dist2D(GetPawn()->GetActorLocation(), Target->GetActorLocation());

	if (Distance <= 145.f)
	{
		StopMovement();
		return true;
	}

	if (GetMoveStatus() != EPathFollowingStatus::Moving)
	{
		MoveToActor(Target.Get(), 90.f, true);
	}

	return false;
}

bool ACCLEnemyAIController::ReturnHome()
{
	ClearFocus(EAIFocusPriority::Gameplay);
	const auto* Enemy = Cast<ACCLEnemyCharacter>(GetPawn());

	if (!Enemy)
	{
		return true;
	}

	if (FVector::Dist2D(Enemy->GetActorLocation(), Enemy->GetHome()) < 70.f)
	{
		return true;
	}

	if (!IsActionRunning() && GetMoveStatus() != EPathFollowingStatus::Moving)
	{
		MoveToLocation(Enemy->GetHome(), 40.f);
	}

	return false;
}

bool ACCLEnemyAIController::StartAttack()
{
	auto* Enemy = Cast<ACCLEnemyCharacter>(GetPawn());
	auto* ASC = Enemy ? Cast<UCCLAbilitySystemComponent>(Enemy->GetAbilitySystemComponent()) : nullptr;

	if (!ASC || !HasTarget() || IsActionRunning())
	{
		return false;
	}

	StopMovement();
	ASC->AbilityInputTagPressed(CCLTags::Input_Attack);
	ASC->AbilityInputTagReleased(CCLTags::Input_Attack);
	return ASC->HasMatchingGameplayTag(CCLTags::State_Busy);
}

bool ACCLEnemyAIController::IsActionRunning() const
{
	const auto* Enemy = Cast<ACCLEnemyCharacter>(GetPawn());
	const auto* ASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
	return !ASC || ASC->HasMatchingGameplayTag(CCLTags::State_Busy) || ASC->HasMatchingGameplayTag(CCLTags::State_Stagger);
}

EStateTreeRunStatus FCCLEnemyTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	Context.GetInstanceData(*this).bStarted = 0;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FCCLEnemyTask::Tick(FStateTreeExecutionContext& Context, float DeltaTime) const
{
	auto* Controller = Cast<ACCLEnemyAIController>(Context.GetOwner());

	if (!Controller || !Controller->GetPawn())
	{
		return EStateTreeRunStatus::Failed;
	}

	switch (Mode)
	{
	case ECCLEnemyTask::Acquire:
		return Controller->AcquireTarget() ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Failed;

	case ECCLEnemyTask::Approach:
		if (!Controller->HasTarget())
		{
			return EStateTreeRunStatus::Failed;
		}

		return Controller->Approach() ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;

	case ECCLEnemyTask::Attack:
	{
		auto& Data = Context.GetInstanceData(*this);

		if (!Controller->HasTarget())
		{
			return EStateTreeRunStatus::Failed;
		}

		if (!Data.bStarted)
		{
			Data.bStarted = Controller->StartAttack();
		}

		return Data.bStarted && !Controller->IsActionRunning() ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
	}

	case ECCLEnemyTask::Return:
		Controller->ReturnHome();
		return Controller->AcquireTarget() ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
	}

	return EStateTreeRunStatus::Failed;
}
