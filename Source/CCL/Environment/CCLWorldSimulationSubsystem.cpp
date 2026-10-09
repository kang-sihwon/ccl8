#include "CCLWorldSimulationSubsystem.h"

#include "Agents/CCLAgentWorldSubsystem.h"
#include "CCLWorldAdvance.h"
#include "CCLWorldEnvironmentState.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

bool UCCLWorldSimulationSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const auto* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UCCLWorldSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UCCLAgentWorldSubsystem>();
}

void UCCLWorldSimulationSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bRunning || GetWorld()->GetNetMode() == NM_Client || GetWorld()->IsPaused())
	{
		return;
	}

	UnqueuedGameSeconds += DeltaTime;
	FString Error;
	if (Clock.QueueGameTime(UnqueuedGameSeconds, false, Error))
	{
		UnqueuedGameSeconds = 0;
	}
	else
	{
		ReportFailure(Error);
	}

	// Limit per-frame work without discarding elapsed input or old-scale intervals.
	for (int32 Step = 0; Step < 8 && Clock.GetPendingGameSeconds() >= 0.25; ++Step)
	{
		if (!AdvancePending(0.25, 15, Error))
		{
			break;
		}
	}
}

TStatId UCCLWorldSimulationSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLWorldSimulationSubsystem, STATGROUP_Tickables);
}

bool UCCLWorldSimulationSubsystem::Start(FCCLLifeSimulation& Life, ECCLWorldDomain Domain,
	const TArray<uint8>& Saved, FString& Error)
{
	if (GetWorld()->GetNetMode() == NM_Client || bRunning)
	{
		Error = TEXT("Only an unstarted server world may initialize its clock.");
		return false;
	}

	FCCLWorldIdentity CandidateIdentity;
	CandidateIdentity.Domain = Domain;
	CandidateIdentity.WorldId = FGuid::NewGuid();
	FCCLWorldClock CandidateClock;
	if (Saved.IsEmpty())
	{
		if (!CandidateClock.Reset(Life.GetTime(), 60, Error))
		{
			return false;
		}
	}
	else
	{
		FCCLWorldSnapshot Snapshot;
		if (!FCCLWorldSnapshotCodec::Decode(Saved, Snapshot, Error) ||
			!FCCLWorldSnapshotCodec::Restore(Snapshot, Domain, CandidateClock, Life, Error))
		{
			return false;
		}

		CandidateIdentity = Snapshot.Identity;
	}

	Clock = MoveTemp(CandidateClock);
	Identity = CandidateIdentity;
	Epoch = FGuid::NewGuid();
	bRunning = 1;
	return true;
}

bool UCCLWorldSimulationSubsystem::QueueGameTime(double Seconds, FString& Error)
{
	return CheckAuthority(Error) && Clock.QueueGameTime(Seconds, GetWorld()->IsPaused(), Error);
}

bool UCCLWorldSimulationSubsystem::AdvancePending(double MaxGameSeconds, double MaxWorldSeconds,
	FString& Error, int32 MaxLifeSlices)
{
	if (!CheckAuthority(Error))
	{
		return false;
	}

	auto* Agents = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	if (!Agents || !Agents->IsRunning())
	{
		Error = TEXT("The life owner is not ready.");
		return false;
	}

	if (!CCLWorldAdvance::Advance(Clock, Agents->GetSimulation(), MaxGameSeconds, MaxWorldSeconds, Error, MaxLifeSlices))
	{
		ReportFailure(Error);
		Publish();
		return false;
	}

	if (UnqueuedGameSeconds == 0)
	{
		LastError.Reset();
	}

	Publish();
	return true;
}

bool UCCLWorldSimulationSubsystem::ChangeTimeScale(double Scale, FString& Error)
{
	if (!CheckAuthority(Error))
	{
		return false;
	}

	if (UnqueuedGameSeconds != 0)
	{
		Error = TEXT("Time input must be accepted before changing scale.");
		return false;
	}

	if (!Clock.ChangeTimeScale(Scale, Error))
	{
		return false;
	}

	Publish();
	return true;
}

bool UCCLWorldSimulationSubsystem::Save(const FCCLLifeSimulation& Life, TArray<uint8>& Bytes, FString& Error)
{
	if (!CheckAuthority(Error) || UnqueuedGameSeconds != 0)
	{
		Error = TEXT("Cannot save a client, unstarted world or unaccepted input.");
		return false;
	}

	if (Identity.Generation == MAX_uint64)
	{
		Error = TEXT("World save generation exhausted.");
		return false;
	}

	FCCLWorldIdentity NextIdentity = Identity;
	++NextIdentity.Generation;
	FCCLWorldSnapshot Snapshot;
	if (!FCCLWorldSnapshotCodec::Capture(NextIdentity, Clock, Life, Snapshot, Error) ||
		!FCCLWorldSnapshotCodec::Encode(Snapshot, Bytes, Error))
	{
		return false;
	}

	Identity = NextIdentity;
	return true;
}

bool UCCLWorldSimulationSubsystem::Restore(FCCLLifeSimulation& Life, const TArray<uint8>& Bytes, FString& Error)
{
	if (!CheckAuthority(Error))
	{
		return false;
	}

	FCCLWorldSnapshot Snapshot;
	if (!FCCLWorldSnapshotCodec::Decode(Bytes, Snapshot, Error) ||
		!FCCLWorldSnapshotCodec::Restore(Snapshot, Identity.Domain, Clock, Life, Error))
	{
		return false;
	}

	Identity = Snapshot.Identity;
	Epoch = FGuid::NewGuid();
	UnqueuedGameSeconds = 0;
	LastError.Reset();
	Publish();
	return true;
}

ECCLWorldDomain UCCLWorldSimulationSubsystem::DomainForWorld(const UWorld* World)
{
	const FString Name = UGameplayStatics::GetCurrentLevelName(World, true);
	if (Name == TEXT("EnvironmentPlayground"))
	{
		return ECCLWorldDomain::Playground;
	}

	return Name == TEXT("EnvironmentScenario") ? ECCLWorldDomain::Scenario : ECCLWorldDomain::Campaign;
}

void UCCLWorldSimulationSubsystem::Publish()
{
	if (!IsValid(ReplicatedState))
	{
		ReplicatedState = GetWorld()->SpawnActor<ACCLWorldEnvironmentState>();
	}

	if (ReplicatedState)
	{
		FCCLReplicatedWorldTime Time;
		Time.WorldId = Identity.WorldId;
		Time.Epoch = Epoch;
		Time.GameSeconds = Clock.GetGameSeconds();
		Time.WorldSeconds = Clock.GetWorldSeconds();
		Time.TimeScale = Clock.GetTimeScale();
		Time.PendingGameSeconds = Clock.GetPendingGameSeconds() + UnqueuedGameSeconds;
		Time.CompletedStepId = Clock.GetCompletedStepId();
		Time.bAdvanceFailed = !LastError.IsEmpty();
		ReplicatedState->Publish(Time);
	}
}

bool UCCLWorldSimulationSubsystem::CheckAuthority(FString& Error) const
{
	Error.Reset();
	if (!bRunning || GetWorld()->GetNetMode() == NM_Client)
	{
		Error = TEXT("World time is owned by the running server.");
		return false;
	}

	return true;
}

void UCCLWorldSimulationSubsystem::ReportFailure(const FString& Error)
{
	if (LastError != Error)
	{
		UE_LOG(LogTemp, Warning, TEXT("CCL_WORLD advancement pending: %s"), *Error);
	}

	LastError = Error;
}
