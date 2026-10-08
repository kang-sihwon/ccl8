#include "CCLAgentComponent.h"

#include "CCLAgentWorldSubsystem.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

UCCLAgentComponent::UCCLAgentComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 1.f;
}

void UCCLAgentComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()->HasAuthority())
	{
		if (auto* World = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>(); World && World->IsRunning())
		{
			World->GetSimulation().SetActorActive(AgentId, true);
		}
	}
}

void UCCLAgentComponent::EndPlay(EEndPlayReason::Type Reason)
{
	if (GetOwner()->HasAuthority())
	{
		if (auto* World = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>(); World && World->IsRunning())
		{
			World->GetSimulation().UpdateLocation(AgentId, GetOwner()->GetActorLocation());
			World->GetSimulation().SetActorActive(AgentId, false);
		}
	}

	Super::EndPlay(Reason);
}

void UCCLAgentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function)
{
	Super::TickComponent(DeltaTime, TickType, Function);
	if (GetOwner()->HasAuthority())
	{
		if (auto* World = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>(); World && World->IsRunning())
		{
			World->GetSimulation().UpdateLocation(AgentId, GetOwner()->GetActorLocation());
		}
	}
}

void UCCLAgentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCCLAgentComponent, AgentId);
}
