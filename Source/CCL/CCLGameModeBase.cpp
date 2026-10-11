#include "CCLGameModeBase.h"

#include "CCLCharacter.h"
#include "CCLPlayerState.h"
#include "CCLPlayerController.h"
#include "CCLHUD.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Session/CCLPlayerPersistenceSubsystem.h"
#include "Session/CCLProfileIdentity.h"
#include "TimerManager.h"

namespace
{
	// A player whose profile cannot bind within this many retries keeps playing unregistered.
	constexpr int32 MaxProfileAttempts = 40;
}

// 생성자

ACCLGameModeBase::ACCLGameModeBase()
{
	DefaultPawnClass = ACCLCharacter::StaticClass();
	PlayerControllerClass = ACCLPlayerController::StaticClass();
	HUDClass = ACCLHUD::StaticClass();
	PlayerStateClass = ACCLPlayerState::StaticClass();
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

FString ACCLGameModeBase::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId,
	const FString& Options, const FString& Portal)
{
	const FString Result = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

	// The host keeps its personal session record; only remote players get world profiles.
	if (Result.IsEmpty() && NewPlayerController && !NewPlayerController->IsLocalController())
	{
		PendingProfiles.Add(NewPlayerController, UGameplayStatics::ParseOption(Options, CCLProfileIdentity::LoginOption()));
	}

	return Result;
}

void ACCLGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	BindPendingProfiles();
	if (!PendingProfiles.IsEmpty() && !GetWorldTimerManager().IsTimerActive(ProfileTimer))
	{
		GetWorldTimerManager().SetTimer(ProfileTimer, this, &ThisClass::BindPendingProfiles, 0.25f, true);
	}
}

void ACCLGameModeBase::Logout(AController* Exiting)
{
	PendingRespawns.Remove(Exiting);
	NextRetryTimes.Remove(Exiting);
	if (auto* Player = Cast<APlayerController>(Exiting))
	{
		PendingProfiles.Remove(Player);
		ProfileAttempts.Remove(Player);
	}

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

void ACCLGameModeBase::BindPendingProfiles()
{
	for (auto It = PendingProfiles.CreateIterator(); It; ++It)
	{
		auto* Player = It.Key().Get();
		int32& Attempts = ProfileAttempts.FindOrAdd(It.Key());
		if (!Player || BindProfile(*Player, It.Value()) || ++Attempts >= MaxProfileAttempts)
		{
			if (Player && Attempts >= MaxProfileAttempts)
			{
				UE_LOG(LogTemp, Warning, TEXT("CCL_PROFILE %s plays without a world profile."), *GetNameSafe(Player));
			}

			ProfileAttempts.Remove(It.Key());
			It.RemoveCurrent();
		}
	}

	if (PendingProfiles.IsEmpty())
	{
		GetWorldTimerManager().ClearTimer(ProfileTimer);
	}
}

bool ACCLGameModeBase::BindProfile(APlayerController& Player, const FString& Secret)
{
	auto* State = Player.GetPlayerState<ACCLPlayerState>();
	auto* Profiles = GetWorld()->GetSubsystem<UCCLPlayerPersistenceSubsystem>();
	const auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	const auto* Agents = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	if (!State || !Profiles || !Runtime || !Runtime->IsRunning() || !Agents || !Agents->IsRunning())
	{
		return false;
	}

	if (Profiles->IsBound(*State))
	{
		return true;
	}

	// A known secret returns to its profile. A missing, malformed or already-connected secret gets a new identity
	// instead of taking over another connection.
	FString Error;
	if (CCLProfileIdentity::IsValidSecret(Secret))
	{
		if (Profiles->Bind(*State, CCLProfileIdentity::IdentityKeyFor(Secret), Error))
		{
			UE_LOG(LogTemp, Display, TEXT("CCL_PROFILE %s rejoined its world profile."), *GetNameSafe(&Player));
			return true;
		}

		UE_LOG(LogTemp, Warning, TEXT("CCL_PROFILE %s presented a profile that cannot bind (%s); issuing a new one."),
			*GetNameSafe(&Player), *Error);
	}

	const FString Issued = CCLProfileIdentity::IssueSecret();
	if (!Profiles->Bind(*State, CCLProfileIdentity::IdentityKeyFor(Issued), Error))
	{
		UE_LOG(LogTemp, Verbose, TEXT("CCL_PROFILE %s waits for binding: %s"), *GetNameSafe(&Player), *Error);
		return false;
	}

	if (auto* Controller = Cast<ACCLPlayerController>(&Player))
	{
		Controller->ClientReceiveProfileKey(Issued);
	}

	UE_LOG(LogTemp, Display, TEXT("CCL_PROFILE %s registered a new world profile."), *GetNameSafe(&Player));
	return true;
}
