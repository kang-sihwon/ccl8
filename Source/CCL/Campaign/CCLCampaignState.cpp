#include "CCLCampaignState.h"

#include "Net/UnrealNetwork.h"

void ACCLCampaignState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACCLCampaignState, Progress);
}

void ACCLCampaignState::SetProgress(ECCLCampaignPhase Phase, int32 RemainingGuards)
{
	if (!HasAuthority())
	{
		return;
	}

	Progress.Phase = Phase;
	Progress.RemainingGuards = FMath::Max(0, RemainingGuards);
	ForceNetUpdate();
}

FString ACCLCampaignState::GetObjective() const
{
	switch (Progress.Phase)
	{
	case ECCLCampaignPhase::Village:
		return TEXT("VILLAGE | Follow the road east to the ruined gate.");
	case ECCLCampaignPhase::Road:
		return FString::Printf(TEXT("ROAD | Defeat the gate guards: %d remaining"), Progress.RemainingGuards);
	case ECCLCampaignPhase::Boss:
		return TEXT("BOSS | Defeat the Gate Warden in the eastern courtyard.");
	case ECCLCampaignPhase::Victory:
		return TEXT("VICTORY | The road is open. Expedition complete. Esc: Menu");
	default:
		return TEXT("Encounter could not start. Restart the session.");
	}
}
