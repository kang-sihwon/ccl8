#include "CCLCampaignGameMode.h"

#include "CCLCampaignState.h"

ACCLCampaignGameMode::ACCLCampaignGameMode()
{
	GameStateClass = ACCLCampaignState::StaticClass();
}
