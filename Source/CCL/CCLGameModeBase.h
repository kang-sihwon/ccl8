#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CCLGameModeBase.generated.h"

UCLASS()
class CCL_API ACCLGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

	// 부모 인터페이스 함수
public:
	ACCLGameModeBase();

	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;

	// 내 클래스 함수
public:
	void RequestRetry(APlayerController* Player);

	// 프로퍼티
private:
	TSet<TWeakObjectPtr<AController>> PendingRespawns;
	TMap<TWeakObjectPtr<AController>, double> NextRetryTimes;
};
