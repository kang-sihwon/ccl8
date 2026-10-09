#include "CCLAgentAIController.h"

#include "Environment/CCLTerrainNavigation.h"
#include "CCLAgentComponent.h"
#include "CCLAgentWorldSubsystem.h"
#include "CCLAgentTags.h"
#include "Campaign/CCLLifeVillager.h"
#include "Components/StateTreeAIComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "StateTree.h"
#include "StateTreeExecutionContext.h"
#include "Engine/World.h"

ACCLAgentAIController::ACCLAgentAIController()
{
	StateTree = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTree"));
	StateTree->SetStartLogicAutomatically(false);
}

void ACCLAgentAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (auto* Tree = LoadObject<UStateTree>(nullptr, TEXT("/Game/Progression/ST_LifeAgent.ST_LifeAgent")))
	{
		StateTree->SetStateTree(Tree);
		StateTree->StartLogic();
	}
}

void ACCLAgentAIController::OnUnPossess()
{
	AbortIntent();
	StateTree->StopLogic(TEXT("Agent unloaded"));
	StopMovement();
	OpportunityId.Invalidate();
	Super::OnUnPossess();
}

EStateTreeRunStatus ACCLAgentAIController::SelectIntent()
{
	if (GetWorld()->GetTimeSeconds() < NextDecisionTime)
	{
		return EStateTreeRunStatus::Running;
	}

	NextDecisionTime = GetWorld()->GetTimeSeconds() + 1;
	const auto* Agent = GetPawn() ? GetPawn()->FindComponentByClass<UCCLAgentComponent>() : nullptr;
	auto* World = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	if (!Agent || !World || !World->IsRunning())
	{
		return EStateTreeRunStatus::Running;
	}

	auto& Simulation = World->GetSimulation();
	const auto Decision = Simulation.Decide(Agent->AgentId);
	const auto* Opportunity = Decision.bSelected ? Simulation.FindOpportunity(Decision.Intent.OpportunityId) : nullptr;
	if (!Opportunity || !Simulation.SelectIntent(Agent->AgentId, Decision.Intent, &Decision.Traces))
	{
		return EStateTreeRunStatus::Running;
	}

	Simulation.ReleaseOpportunity(Reservation);
	Reservation = Simulation.ReserveOpportunity(Agent->AgentId, Opportunity->OpportunityId);
	if (!Reservation.IsValid())
	{
		return EStateTreeRunStatus::Running;
	}
	OpportunityId = Opportunity->OpportunityId;
	Revision = Opportunity->Revision;
	Destination = Opportunity->Location.Position;
	if (auto* Villager = Cast<ACCLLifeVillager>(GetPawn()))
	{
		Villager->SetActivity(Decision.Intent.Activity);
	}

	return EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus ACCLAgentAIController::Approach(double StartedTime)
{
	if (!GetPawn() || GetWorld()->GetTimeSeconds() - StartedTime > 30)
	{
		AbortIntent();
		return EStateTreeRunStatus::Failed;
	}

	if (!FCCLTerrainNavigation::IsRouteReady(GetWorld(), GetPawn()->GetActorLocation(), Destination))
	{
		StopMovement();
		return EStateTreeRunStatus::Running;
	}

	if (FVector::DistSquared2D(GetPawn()->GetActorLocation(), Destination) < FMath::Square(140.f))
	{
		StopMovement();
		return EStateTreeRunStatus::Succeeded;
	}

	if (GetMoveStatus() != EPathFollowingStatus::Moving)
	{
		const auto Result = MoveToLocation(Destination, 80, true, true, true, false, nullptr, false);
		if (Result == EPathFollowingRequestResult::Failed)
		{
			AbortIntent();
			return EStateTreeRunStatus::Failed;
		}
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus ACCLAgentAIController::Perform(double StartedTime)
{
	auto* World = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	const auto* Agent = GetPawn() ? GetPawn()->FindComponentByClass<UCCLAgentComponent>() : nullptr;
	if (!Agent || !World || !World->GetSimulation().HasReservation(Agent->AgentId, OpportunityId, Reservation) || FVector::DistSquared2D(GetPawn()->GetActorLocation(), Destination) > FMath::Square(180.f))
	{
		AbortIntent();
		return EStateTreeRunStatus::Failed;
	}

	auto& Simulation = World->GetSimulation();
	const bool bCheckDecision = GetWorld()->GetTimeSeconds() >= NextDecisionTime;
	const auto Decision = bCheckDecision ? Simulation.Decide(Agent->AgentId) : FCCLDecisionResult();
	if (bCheckDecision)
	{
		NextDecisionTime = GetWorld()->GetTimeSeconds() + 1;
	}
	if (Decision.bSelected && Decision.Intent.OpportunityId != OpportunityId)
	{
		const auto* Record = Simulation.Find(Agent->AgentId);
		const auto* Needs = Record ? Record->Features.Find(CCLAgentTags::Feature_Needs) : nullptr;
		if (Needs && Needs->Data.Get<FCCLAgentNeeds>().Urgency.FindRef(CCLAgentTags::Hunger) >= 0.9f)
		{
			AbortIntent();
			return EStateTreeRunStatus::Failed;
		}
	}

	if (GetWorld()->GetTimeSeconds() - StartedTime < 60)
	{
		return EStateTreeRunStatus::Running;
	}

	const auto Result = Simulation.Execute(Agent->AgentId, OpportunityId, Revision, FGuid::NewGuid());
	AbortIntent();
	NextDecisionTime = GetWorld()->GetTimeSeconds() + 1;
	return Result.bSucceeded ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FCCLAgentTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	Context.GetInstanceData(*this).StartedTime = Context.GetWorld()->GetTimeSeconds();
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FCCLAgentTask::Tick(FStateTreeExecutionContext& Context, float DeltaTime) const
{
	auto* Controller = Cast<ACCLAgentAIController>(Context.GetOwner());
	if (!Controller)
	{
		return EStateTreeRunStatus::Failed;
	}

	const double StartedTime = Context.GetInstanceData(*this).StartedTime;
	if (Phase == ECCLAgentExecutionPhase::Select)
	{
		return Controller->SelectIntent();
	}

	return Phase == ECCLAgentExecutionPhase::Approach ? Controller->Approach(StartedTime) : Controller->Perform(StartedTime);
}

void ACCLAgentAIController::AbortIntent()
{
	StopMovement();
	if (auto* World = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>())
	{
		World->GetSimulation().ReleaseOpportunity(Reservation);
	}
	Reservation.Invalidate();
}
