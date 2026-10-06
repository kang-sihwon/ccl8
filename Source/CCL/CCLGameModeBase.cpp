#include "CCLGameModeBase.h"

#include "CCLCharacter.h"
#include "CCLPlayerController.h"
#include "CCLHUD.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

// 생성자

ACCLGameModeBase::ACCLGameModeBase()
{
	DefaultPawnClass = ACCLCharacter::StaticClass();
	PlayerControllerClass = ACCLPlayerController::StaticClass();
	HUDClass = ACCLHUD::StaticClass();
}

// 부모 인터페이스 함수

void ACCLGameModeBase::RestartPlayer(AController* NewPlayer)
{
	if (!IsValid(NewPlayer))
	{
		return;
	}
	// The base RestartPlayer falls back to the cached StartSpot when no free start exists.
	// Require a freshly checked start so a blocked checkpoint remains retryable.
	if (AActor* StartSpot = FindPlayerStart(NewPlayer))
	{
		RestartPlayerAtPlayerStart(NewPlayer, StartSpot);
	}
	if (IsValid(NewPlayer->GetPawn()))
	{
		PendingRespawns.Remove(NewPlayer);
		UE_LOG(LogTemp, Display, TEXT("CCL Spawn Controller=%s Pawn=%s NetMode=%d"),
			*GetNameSafe(NewPlayer), *GetNameSafe(NewPlayer->GetPawn()), static_cast<int32>(GetNetMode()));
	}
	else
	{
		PendingRespawns.Add(NewPlayer);
		UE_LOG(LogTemp, Warning, TEXT("CCL Spawn blocked Controller=%s; retry is available"), *GetNameSafe(NewPlayer));
	}
}

void ACCLGameModeBase::Logout(AController* Exiting)
{
	PendingRespawns.Remove(Exiting);
	NextRetryTimes.Remove(Exiting);
	Super::Logout(Exiting);
}

AActor* ACCLGameModeBase::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	const ACCLCharacter* DefaultCharacter = GetDefault<ACCLCharacter>();
	const UCapsuleComponent* Capsule = DefaultCharacter->GetCapsuleComponent();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CCLSpawn), false);
	if (Player && Player->GetPawn())
	{
		Params.AddIgnoredActor(Player->GetPawn());
	}
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		if (!GetWorld()->OverlapBlockingTestByChannel(It->GetActorLocation(), FQuat::Identity, ECC_Pawn,
			FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight()), Params))
		{
			return *It;
		}
	}
	return nullptr;
}

// 내 클래스 함수

void ACCLGameModeBase::RequestRetry(APlayerController* Player)
{
	if (!HasAuthority() || !IsValid(Player))
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (const double* NextRetry = NextRetryTimes.Find(Player); NextRetry && Now < *NextRetry)
	{
		return;
	}
	ACCLCharacter* Character = Cast<ACCLCharacter>(Player->GetPawn());
	if (Character && Character->IsDead())
	{
		PendingRespawns.Add(Player);
		Player->UnPossess();
		Character->Destroy();
	}
	else if (Player->GetPawn() || !PendingRespawns.Contains(Player))
	{
		return;
	}
	NextRetryTimes.Add(Player, Now + 0.5);
	RestartPlayer(Player);
}
