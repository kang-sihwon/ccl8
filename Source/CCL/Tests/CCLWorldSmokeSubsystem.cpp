#include "CCLWorldSmokeSubsystem.h"

#include "Agents/CCLAgentWorldSubsystem.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Environment/CCLWorldEnvironmentState.h"
#include "Engine/World.h"
#include "EngineUtils.h"

bool UCCLWorldSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLWorldSmoke"));
#endif
}

void UCCLWorldSmokeSubsystem::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	if (Started == 0)
	{
		Started = Now;
	}

	if (Now - Started > 120)
	{
		Check(false, TEXT("watchdog"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !World->HasBegunPlay() || Now < Next)
	{
		return;
	}

	auto* Runtime = World->GetSubsystem<UCCLWorldSimulationSubsystem>();
	auto* Agents = World->GetSubsystem<UCCLAgentWorldSubsystem>();
	FString Error;
	if (World->GetNetMode() == NM_Client)
	{
		FString ExpectedId;
		double ExpectedTime = -1;
		FParse::Value(FCommandLine::Get(), TEXT("CCLWorldId="), ExpectedId);
		FParse::Value(FCommandLine::Get(), TEXT("CCLWorldTime="), ExpectedTime);
		for (TActorIterator<ACCLWorldEnvironmentState> It(World); It; ++It)
		{
			const auto& Time = It->GetTime();
			if (Time.WorldId.ToString() != ExpectedId || Time.TimeScale != 0 ||
				!FMath::IsNearlyEqual(Time.WorldSeconds, ExpectedTime, 1.e-6))
			{
				continue;
			}

			if (!Check(Runtime && Agents && !Runtime->IsRunning() && !Agents->IsRunning(), TEXT("client has no simulation owner")) ||
				!Check(!Runtime->QueueGameTime(1, Error) && !Runtime->ChangeTimeScale(500, Error), TEXT("client time mutations rejected")) ||
				!Check(Time.Epoch.IsValid() && Time.CompletedStepId > 0 && Time.GameSeconds > 0 && !Time.bAdvanceFailed,
					TEXT("replicated server time, epoch and completion received")))
			{
				return;
			}

			UE_LOG(LogTemp, Display, TEXT("CCL_WORLD_SMOKE PASS Client World=%s Time=%.9f"), *Time.WorldId.ToString(), Time.WorldSeconds);
			bComplete = 1;
			FPlatformMisc::RequestExitWithStatus(false, 0);
			return;
		}

		return;
	}

	if (!Runtime || !Agents || !Runtime->IsRunning() || !Agents->IsRunning() || Now - Started < 2)
	{
		return;
	}

	if (Step == 0)
	{
		if (!Check(Runtime->GetClock().GetWorldSeconds() == Agents->GetSimulation().GetTime() &&
			Runtime->GetClock().GetGameSeconds() > 0, TEXT("single clock drives live Agent time")) ||
			!Check(Runtime->ChangeTimeScale(0, Error), TEXT("freeze world time without freezing game")))
		{
			return;
		}

		Step = 1;
		Next = Now + 1;
		return;
	}

	if (Step == 1)
	{
		const double Frozen = Runtime->GetClock().GetWorldSeconds();
		const double GameBefore = Runtime->GetClock().GetGameSeconds();
		if (!Check(Runtime->QueueGameTime(2, Error) && Runtime->AdvancePending(3, 180, Error), TEXT("game progresses at zero world scale")) ||
			!Check(Runtime->GetClock().GetGameSeconds() >= GameBefore + 2 && Runtime->GetClock().GetWorldSeconds() == Frozen,
				TEXT("physics time and life time are separate")))
		{
			return;
		}

		TArray<uint8> Saved;
		if (!Check(Agents->Save(Saved), TEXT("save common clock and Actor life")))
		{
			return;
		}

		const FGuid SavedId = Runtime->GetIdentity().WorldId;
		const uint64 Generation = Runtime->GetIdentity().Generation;
		if (!Check(Runtime->ChangeTimeScale(7, Error) && Runtime->QueueGameTime(3, Error) &&
			Runtime->AdvancePending(10, 1000, Error), TEXT("advance after checkpoint")) ||
			!Check(Agents->Restore(Saved, Error), TEXT("restore rebuilds life and clock together")) ||
			!Check(Runtime->GetClock().GetWorldSeconds() == Frozen && Agents->GetSimulation().GetTime() == Frozen &&
				Runtime->GetIdentity().WorldId == SavedId && Runtime->GetIdentity().Generation == Generation,
				TEXT("restore preserves world identity generation and completed time")))
		{
			return;
		}

		UE_LOG(LogTemp, Display, TEXT("CCL_WORLD_SMOKE READY World=%s Time=%.9f"), *SavedId.ToString(), Frozen);
		UE_LOG(LogTemp, Display, TEXT("CCL_WORLD_SMOKE PASS Authority Mode=%d"), int32(World->GetNetMode()));
		bComplete = 1;
		if (World->GetNetMode() == NM_Standalone)
		{
			FPlatformMisc::RequestExitWithStatus(false, 0);
		}
	}
}

TStatId UCCLWorldSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLWorldSmokeSubsystem, STATGROUP_Tickables);
}

bool UCCLWorldSmokeSubsystem::Check(bool bCondition, const TCHAR* Message)
{
	UE_LOG(LogTemp, Display, TEXT("CCL_WORLD_SMOKE %s %s"), bCondition ? TEXT("CHECK") : TEXT("FAIL"), Message);
	if (!bCondition)
	{
		bComplete = 1;
		FPlatformMisc::RequestExitWithStatus(false, 1);
	}

	return bCondition;
}
