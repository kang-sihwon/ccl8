#include "CCLMenuGameMode.h"
#include "CCLPlayerController.h"
ACCLMenuGameMode::ACCLMenuGameMode()
{
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
	PlayerControllerClass = ACCLPlayerController::StaticClass();
}
