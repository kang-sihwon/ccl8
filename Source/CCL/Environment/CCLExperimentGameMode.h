#pragma once

#include "CoreMinimal.h"
#include "CCLGameModeBase.h"
#include "CCLExperimentGameMode.generated.h"

UCLASS()
class CCL_API ACCLExperimentGameMode : public ACCLGameModeBase
{
	GENERATED_BODY()

public:
	ACCLExperimentGameMode();
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
};
