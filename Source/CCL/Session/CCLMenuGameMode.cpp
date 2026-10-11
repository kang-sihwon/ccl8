#include "CCLMenuGameMode.h"

#include "CCLGameInstance.h"
#include "CCLPlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"

// 생성자

ACCLMenuGameMode::ACCLMenuGameMode()
{
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
	PlayerControllerClass = ACCLPlayerController::StaticClass();
}

// 부모 인터페이스 함수

void ACCLMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	// The local player's UI root is ready a moment after the first frame.
	if (auto* Session = GetGameInstance<UCCLGameInstance>())
	{
		FTimerHandle MenuTimer;
		GetWorldTimerManager().SetTimer(MenuTimer, FTimerDelegate::CreateUObject(Session, &UCCLGameInstance::ShowMenu), 0.2f, false);
	}
}
