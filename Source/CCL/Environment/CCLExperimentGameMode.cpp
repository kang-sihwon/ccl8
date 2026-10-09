#include "CCLExperimentGameMode.h"

#include "CCLExperimentDirector.h"
#include "CCLExperimentPlayerController.h"

ACCLExperimentGameMode::ACCLExperimentGameMode()
{
	PlayerControllerClass = ACCLExperimentPlayerController::StaticClass();
}

void ACCLExperimentGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (auto* Director = ACCLExperimentDirector::Find(GetWorld()))
	{
		Director->AssignOperator(NewPlayer);
	}
}

void ACCLExperimentGameMode::Logout(AController* Exiting)
{
	if (auto* Director = ACCLExperimentDirector::Find(GetWorld()))
	{
		Director->ReleaseOperator(Cast<APlayerController>(Exiting));
	}

	Super::Logout(Exiting);
}
