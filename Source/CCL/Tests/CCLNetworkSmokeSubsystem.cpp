#include "CCLNetworkSmokeSubsystem.h"

#include "../CCLCharacter.h"
#include "../CCLPlayerController.h"
#include "EnhancedPlayerInput.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/BoxComponent.h"
#include "GameFramework/PlayerStart.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "InputActionValue.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// 부모 인터페이스 함수

bool UCCLNetworkSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	FString Role;
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && !IsRunningDedicatedServer() && FParse::Value(FCommandLine::Get(), TEXT("CCLSmoke="), Role) && (Role == TEXT("driver") || Role == TEXT("witness"));
#else
	return false;
#endif
}

void UCCLNetworkSmokeSubsystem::Tick(float DeltaTime)
{
	if (bFinished)
	{
		return;
	}

	const double RealNow = FPlatformTime::Seconds();
	// Startup stalls count toward the watchdog, but not simulated movement windows.
	const double Now = GetWorld()->GetTimeSeconds();

	if (StartedAt == 0.0)
	{
		StartedAt = RealNow;
	}

	if (RealNow - StartedAt > 60.0)
	{
		Finish(false, TEXT("Timed out waiting for replicated state"));
		return;
	}

	auto* PC = Cast<ACCLPlayerController>(GetWorld()->GetFirstPlayerController());

	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	auto* Character = Cast<ACCLCharacter>(PC->GetPawn());
	FString Role;
	FParse::Value(FCommandLine::Get(), TEXT("CCLSmoke="), Role);
	int32 ExpectedPlayers = 1;
	FParse::Value(FCommandLine::Get(), TEXT("CCLSmokePlayers="), ExpectedPlayers);
	TArray<ACCLCharacter*> Characters;

	for (TActorIterator<ACCLCharacter> It(GetWorld()); It; ++It)
	{
		Characters.Add(*It);
	}

	if (Phase == 0)
	{
		if (!Character || Characters.Num() != ExpectedPlayers || !PC->GetMoveAction())
		{
			return;
		}

		Original = Character;

		for (auto* Other : Characters)
		{
			if (Other != Character)
			{
				Peer = Other;
				PeerStartLocation = Other->GetActorLocation();
			}
		}

		StartLocation = Character->GetActorLocation();
		Phase = 1;
		PhaseStartedAt = Now;
		PC->CCLRetry(); // A living player must retain the same pawn.
		return;
	}

	if (Role == TEXT("witness"))
	{
		if (Character != Original.Get() || !Character || Character->IsDead())
		{
			Finish(false, TEXT("Other player's retry changed the witness pawn"));
			return;
		}

		if (Peer.IsValid() && Peer->IsDead())
		{
			bSawPeerDeath = 1;
		}

		if (Peer.IsValid() && FVector::Dist2D(PeerStartLocation, Peer->GetActorLocation()) > 100.f)
		{
			bSawPeerMovement = 1;
		}

		if (Phase == 1 && bSawPeerDeath && bSawPeerMovement && !Peer.IsValid() && Characters.Num() == ExpectedPlayers)
		{
			Phase = 2;
			PhaseStartedAt = Now;
		}

		if (Phase == 2 && Now - PhaseStartedAt > 6.0)
		{
			Finish(true, TEXT("Remote death and replacement observed; witness pawn preserved"));
		}

		return;
	}

	auto InjectMove = [PC]()
	{
		if (auto* Input = Cast<UEnhancedPlayerInput>(PC->PlayerInput))
		{
			Input->InjectInputForAction(PC->GetMoveAction(), FInputActionValue(FVector2D(0.f, 1.f)));
		}
	};
	const double Age = Now - PhaseStartedAt;

	if (Phase == 1)
	{
		InjectMove();

		if (Age < 1.0)
		{
			return;
		}

		if (Character != Original.Get() || !Character || Character->IsDead() || FVector::Dist2D(StartLocation, Character->GetActorLocation()) < 100.f)
		{
			Finish(false, TEXT("Living retry guard or input movement failed"));
			return;
		}

		PC->CCLDie();
		PC->CCLDie();
		Phase = 2;
		PhaseStartedAt = Now;
	}
	else if (Phase == 2 && Character && Character->IsDead())
	{
		StartLocation = Character->GetActorLocation();
		Phase = 3;
		PhaseStartedAt = Now;
	}
	else if (Phase == 3)
	{
		InjectMove();

		if (Age < 1.0)
		{
			return;
		}

		if (!Character || FVector::Dist(StartLocation, Character->GetActorLocation()) > 5.f)
		{
			Finish(false, TEXT("Dead character moved"));
			return;
		}

		PC->CCLRetry();
		PC->CCLRetry();
		PC->CCLRetry();
		Phase = 4;
		PhaseStartedAt = Now;
	}
	else if (Phase == 4 && Character && !Character->IsDead() && Character != Original.Get() && Age > 0.5)
	{
		if (Characters.Num() != ExpectedPlayers || PC->GetViewTarget() != Character)
		{
			Finish(false, TEXT("Duplicate pawn or camera target did not recover"));
			return;
		}

		StartLocation = Character->GetActorLocation();
		Phase = 5;
		PhaseStartedAt = Now;
	}
	else if (Phase == 5)
	{
		InjectMove();

		if (Age < 1.0)
		{
			return;
		}

		if (!Character || FVector::Dist2D(StartLocation, Character->GetActorLocation()) < 100.f)
		{
			Finish(false, TEXT("Input did not recover after respawn"));
			return;
		}

		Phase = 6;
		PhaseStartedAt = Now;
	}
	else if (Phase == 6 && Age > 7.0)
	{
		if (GetWorld()->GetNetMode() == NM_Standalone && Character)
		{
			Character->SetActorLocation(FVector(-800.f, -600.f, -900.f));
			Phase = 7;
			PhaseStartedAt = Now;
		}
		else
		{
			Finish(true, TEXT("Move, living retry guard, death, repeated retry, camera target and respawn input"));
		}
	}
	else if (Phase == 7 && Character && Character->IsDead())
	{
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			AActor* Blocker = GetWorld()->SpawnActor<AActor>();
			UBoxComponent* Box = NewObject<UBoxComponent>(Blocker);
			Blocker->SetRootComponent(Box);
			Blocker->AddInstanceComponent(Box);
			Box->SetBoxExtent(FVector(150.f));
			Box->SetCollisionProfileName(TEXT("BlockAll"));
			Box->RegisterComponent();
			Blocker->SetActorLocation(It->GetActorLocation());
			SpawnBlockers.Add(Blocker);
		}

		if (SpawnBlockers.IsEmpty())
		{
			Finish(false, TEXT("No starts found for blocked-start test"));
			return;
		}

		Phase = 10;
		PhaseStartedAt = Now;
	}
	else if (Phase == 10 && Age > 0.5)
	{
		PC->CCLRetry();
		Phase = 8;
		PhaseStartedAt = Now;
	}
	else if (Phase == 8 && Age > 1.0)
	{
		if (PC->GetPawn())
		{
			Finish(false, TEXT("Spawn ignored blocked starts"));
			return;
		}

		for (auto Blocker : SpawnBlockers)
		{
			if (Blocker.IsValid())
			{
				Blocker->Destroy();
			}
		}

		SpawnBlockers.Reset();
		PC->CCLRetry();
		Phase = 9;
		PhaseStartedAt = Now;
	}
	else if (Phase == 9 && Character && !Character->IsDead())
	{
		Finish(true, TEXT("Input, retry guards, death, camera target, respawn, KillZ and blocked-start recovery"));
	}
}

TStatId UCCLNetworkSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLNetworkSmokeSubsystem, STATGROUP_Tickables);
}

// 내 클래스 함수

void UCCLNetworkSmokeSubsystem::Finish(bool bSuccess, const TCHAR* Reason)
{
	bFinished = 1;
	UE_LOG(LogTemp, Display, TEXT("CCL_SMOKE %s: %s NetMode=%d"),
	    bSuccess ? TEXT("PASS") : TEXT("FAIL"), Reason, static_cast<int32>(GetWorld()->GetNetMode()));
	FString Role;
	FParse::Value(FCommandLine::Get(), TEXT("CCLSmoke="), Role);

	if (bSuccess && Role == TEXT("witness"))
	{
		return;
	}

	FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1);
}
