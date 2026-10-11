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
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options,
		const FString& Portal = TEXT("")) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;

	// 내 클래스 함수
public:
	void RequestRetry(APlayerController* Player);

private:
	// Registers remote players with their server-issued profile once the persistent world is running.
	void BindPendingProfiles();
	bool BindProfile(APlayerController& Player, const FString& Secret);

	// 프로퍼티
private:
	TSet<TWeakObjectPtr<AController>> PendingRespawns;
	TMap<TWeakObjectPtr<AController>, double> NextRetryTimes;
	TMap<TWeakObjectPtr<APlayerController>, FString> PendingProfiles;
	TMap<TWeakObjectPtr<APlayerController>, int32> ProfileAttempts;
	FTimerHandle ProfileTimer;
};
