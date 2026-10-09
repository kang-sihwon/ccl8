#include "CCLWorldEnvironmentState.h"

#include "Net/UnrealNetwork.h"

ACCLWorldEnvironmentState::ACCLWorldEnvironmentState()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(4);
}

void ACCLWorldEnvironmentState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACCLWorldEnvironmentState, Time);
}

void ACCLWorldEnvironmentState::Publish(const FCCLReplicatedWorldTime& Value)
{
	if (HasAuthority())
	{
		Time = Value;
		ForceNetUpdate();
	}
}
