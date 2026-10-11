#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CCLMenuGameMode.generated.h"

// Front-end mode without a pawn. It asks the game instance to show the session menu once play begins.
UCLASS()
class CCL_API ACCLMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

	// 부모 인터페이스 함수
public:
	ACCLMenuGameMode();

	virtual void BeginPlay() override;
};
