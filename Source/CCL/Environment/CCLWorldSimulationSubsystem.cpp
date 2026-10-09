#include "CCLWorldSimulationSubsystem.h"

#include "Agents/CCLAgentWorldSubsystem.h"
#include "CCLWorldAdvance.h"
#include "CCLWorldEnvironmentState.h"
#include "CCLWorldEnvironmentConfig.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	bool CheckMapInterest(const UWorld* World, const FCCLEnvironmentInputs& Inputs, FString& Error)
	{
		const auto* Config = ACCLWorldEnvironmentConfig::Find(World);
		return !Config || Config->ValidateViewInputs(Inputs, Error);
	}
}

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
	FCCLSurfaceSimulation CandidateSurfaceSimulation;
	FCCLEnvironmentInputs CandidateInputs = FCCLEnvironmentInputsCodec::MakeDefault(42);
	int32 ConfigCount = 0;
	for (TActorIterator<ACCLWorldEnvironmentConfig> It(GetWorld()); It; ++It)
	{
		if (++ConfigCount > 1 || !It->BuildInputs(CandidateInputs, Error))
		{
			Error = TEXT("A world requires one valid environment config at most.");
			return false;
		}
	}

	FCCLCelestialSystem CandidateCelestial;
	FCCLSurfaceScene CandidateSurfaces;
	FCCLCelestialObservation Observation;
	if (Saved.IsEmpty())
	{
		if (!CandidateClock.Reset(Life.GetTime(), 60, Error)
			|| !CandidateSurfaceSimulation.Initialize(CandidateIdentity.WorldId, CandidateClock.GetGameSeconds(), Life.GetTime(), 0, Error)
			|| !FCCLEnvironmentInputsCodec::Prepare(CandidateInputs, Life.GetTime(), CandidateCelestial, CandidateSurfaces, Observation, Error))
		{
			return false;
		}
	}
	else
	{
		FCCLWorldSnapshot Snapshot;
		if (!FCCLWorldSnapshotCodec::Decode(Saved, Snapshot, Error))
		{
			return false;
		}

		if (Snapshot.bEnvironmentMigrated)
		{
			Snapshot.Environment = CandidateInputs;
		}

		if (!CheckMapInterest(GetWorld(), Snapshot.Environment, Error)
		|| !FCCLEnvironmentInputsCodec::CheckDefinition(Snapshot.Environment, CandidateInputs.Celestial.DefinitionId, CandidateInputs.Celestial.Version, Error)
			|| !FCCLEnvironmentInputsCodec::Prepare(Snapshot.Environment, Snapshot.Clock.WorldSeconds, CandidateCelestial, CandidateSurfaces, Observation, Error)
			|| !FCCLWorldSnapshotCodec::Restore(Snapshot, Domain, CandidateClock, Life, Error, &CandidateSurfaceSimulation))
		{
			return false;
		}

		CandidateIdentity = Snapshot.Identity;
		CandidateInputs = MoveTemp(Snapshot.Environment);
	}

	Clock = MoveTemp(CandidateClock);
	SurfaceSimulation = MoveTemp(CandidateSurfaceSimulation);
	EnvironmentInputs = MoveTemp(CandidateInputs);
	CelestialSystem = MoveTemp(CandidateCelestial);
	CandidateSurfaces.CopyGeometryProviders(SurfaceScene);
	SurfaceScene = MoveTemp(CandidateSurfaces);
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

	if (!CCLWorldAdvance::Advance(Clock, Agents->GetSimulation(), MaxGameSeconds, MaxWorldSeconds, Error, MaxLifeSlices, &SurfaceSimulation))
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
	if (!FCCLWorldSnapshotCodec::Capture(NextIdentity, Clock, Life, Snapshot, Error, &EnvironmentInputs, &SurfaceSimulation) ||
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
	FCCLCelestialSystem CandidateCelestial;
	FCCLSurfaceScene CandidateSurfaces;
	FCCLCelestialObservation Observation;
	if (!FCCLWorldSnapshotCodec::Decode(Bytes, Snapshot, Error))
	{
		return false;
	}

	if (Snapshot.bEnvironmentMigrated)
	{
		Snapshot.Environment = FCCLEnvironmentInputsCodec::MakeDefault(42);
		if (const auto* Config = ACCLWorldEnvironmentConfig::Find(GetWorld()); Config && !Config->BuildInputs(Snapshot.Environment, Error))
		{
			return false;
		}
	}

	if (!CheckMapInterest(GetWorld(), Snapshot.Environment, Error)
		|| !FCCLEnvironmentInputsCodec::CheckDefinition(Snapshot.Environment, EnvironmentInputs.Celestial.DefinitionId, EnvironmentInputs.Celestial.Version, Error)
		|| !FCCLEnvironmentInputsCodec::Prepare(Snapshot.Environment, Snapshot.Clock.WorldSeconds, CandidateCelestial, CandidateSurfaces, Observation, Error)
		|| !FCCLWorldSnapshotCodec::Restore(Snapshot, Identity.Domain, Clock, Life, Error, &SurfaceSimulation))
	{
		return false;
	}

	EnvironmentInputs = MoveTemp(Snapshot.Environment);
	CelestialSystem = MoveTemp(CandidateCelestial);
	CandidateSurfaces.CopyGeometryProviders(SurfaceScene);
	SurfaceScene = MoveTemp(CandidateSurfaces);
	Identity = Snapshot.Identity;
	Epoch = FGuid::NewGuid();
	UnqueuedGameSeconds = 0;
	LastError.Reset();
	Publish();
	return true;
}

bool UCCLWorldSimulationSubsystem::ReplaceEnvironmentInputs(const FCCLEnvironmentInputs& Candidate, FString& Error)
{
	if (!CheckAuthority(Error))
	{
		return false;
	}

	if (Candidate.Revision <= EnvironmentInputs.Revision || Candidate.SurfaceRevision <= EnvironmentInputs.SurfaceRevision)
	{
		Error = TEXT("Environment replacement requires newer input and surface revisions.");
		return false;
	}

	FCCLCelestialSystem CandidateCelestial;
	FCCLSurfaceScene CandidateSurfaces;
	FCCLCelestialObservation Observation;
	if (!CheckMapInterest(GetWorld(), Candidate, Error)
		|| !FCCLEnvironmentInputsCodec::CheckDefinition(Candidate, EnvironmentInputs.Celestial.DefinitionId, EnvironmentInputs.Celestial.Version, Error)
		|| !FCCLEnvironmentInputsCodec::Prepare(Candidate, Clock.GetWorldSeconds(), CandidateCelestial, CandidateSurfaces, Observation, Error))
	{
		return false;
	}

	EnvironmentInputs = Candidate;
	CelestialSystem = MoveTemp(CandidateCelestial);
	CandidateSurfaces.CopyGeometryProviders(SurfaceScene);
	SurfaceScene = MoveTemp(CandidateSurfaces);
	Publish();
	return true;
}

bool UCCLWorldSimulationSubsystem::ObserveCelestials(FCCLCelestialObservation& Observation, FString& Error) const
{
	if (!bRunning)
	{
		Error = TEXT("Celestial inputs are not initialized on this world.");
		return false;
	}

	return CelestialSystem.Observe(Clock.GetWorldSeconds(), EnvironmentInputs.Observer, Observation, Error);
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
		FString ViewError;
		BuildEnvironmentView(Time.Environment, ViewError);
		ReplicatedState->Publish(Time);
	}
}

bool UCCLWorldSimulationSubsystem::BuildEnvironmentView(FCCLEnvironmentView& OutView, FString& Error) const
{
	FCCLCelestialObservation Observation;
	if (!ObserveCelestials(Observation, Error))
	{
		return false;
	}

	FCCLEnvironmentView View;
	View.DefinitionId = EnvironmentInputs.Celestial.DefinitionId;
	View.DefinitionVersion = EnvironmentInputs.Celestial.Version;
	View.Seed = EnvironmentInputs.Celestial.Seed;
	View.Observer = EnvironmentInputs.Observer;
	View.InputRevision = EnvironmentInputs.Revision;
	View.SurfaceRevision = SurfaceScene.GetRevision();
	View.SurfaceEpoch = SurfaceScene.GetEpoch();
	View.DominantStarId = Observation.DominantStarId;
	for (const auto& Body : EnvironmentInputs.Celestial.Bodies)
	{
		if (Body.BodyId == View.Observer.BodyId)
		{
			View.ObliquityDegrees = Body.ObliquityDegrees;
		}
	}

	FVector ToSun = FVector::UpVector;
	for (const auto& Source : Observation.Stars)
	{
		FCCLCelestialSourceView Star;
		Star.BodyId = Source.BodyId;
		Star.LocalDirection = Source.LocalDirection;
		Star.SolarHours = Source.SolarHours;
		Star.ElevationDegrees = Source.ElevationDegrees;
		Star.NormalIrradiance = Source.NormalIrradianceWattsPerM2;
		Star.HorizontalIrradiance = Source.HorizontalIrradianceWattsPerM2;
		Star.bOcculted = Source.bOcculted;
		View.Stars.Add(Star);
		if (Star.BodyId == View.DominantStarId)
		{
			ToSun = Star.LocalDirection;
		}
	}

	for (const auto& Body : Observation.SkyBodies)
	{
		FCCLCelestialBodyView Sky;
		Sky.BodyId = Body.BodyId;
		Sky.LocalDirection = Body.LocalDirection;
		Sky.AngularRadiusDegrees = Body.AngularRadiusDegrees;
		Sky.IlluminatedFraction = Body.IlluminatedFraction;
		View.SkyBodies.Add(Sky);
	}

	if (const auto* Config = ACCLWorldEnvironmentConfig::Find(GetWorld()))
	{
		for (const auto& Probe : Config->Probes)
		{
			FCCLShelterQuery Query;
			Query.BodyId = View.Observer.BodyId;
			Query.PositionMeters = Probe.PositionMeters;
			Query.ToSun = ToSun;
			Query.ToPrecipitationSource = Probe.ToPrecipitationSource;
			Query.ToWindSource = Probe.ToWindSource;
			Query.RequiredEpoch = View.SurfaceEpoch;
			Query.RequiredRevision = View.SurfaceRevision;
			FCCLShelterSample Sample;
			if (!FCCLShelterEvaluator::Evaluate(SurfaceScene, Query, Sample, Error))
			{
				return false;
			}

			FCCLEnvironmentProbeView Result;
			Result.ProbeId = Probe.ProbeId;
			Result.PositionMeters = Probe.PositionMeters;
			Result.Transmission = Sample.Transmission;
			View.Probes.Add(Result);
		}

		for (const auto& Opening : EnvironmentInputs.Openings)
		{
			if (Config->ViewOpeningIds.Contains(Opening.OpeningId))
			{
				View.Openings.Add(Opening);
			}
		}

		for (const auto& Surface : EnvironmentInputs.Surfaces)
		{
			if (Config->ViewSurfaceIds.Contains(Surface.SurfaceId))
			{
				View.Surfaces.Add(Surface);
			}
		}
	}

	View.bValid = 1;
	OutView = MoveTemp(View);
	return true;
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

void UCCLWorldSimulationSubsystem::SetGeometryProvider(FGuid Id, TSharedPtr<const ICCLSurfaceProvider> Provider)
{
	SurfaceScene.SetGeometryProvider(Id, MoveTemp(Provider));
	if (bRunning && GetWorld()->GetNetMode() != NM_Client && !GetWorld()->bIsTearingDown)
	{
		Publish();
	}
}

bool UCCLWorldSimulationSubsystem::AddSurfaceRegion(FCCLSurfaceGrid Region, FString& Error)
{
	return CheckAuthority(Error) && SurfaceSimulation.AddRegion(MoveTemp(Region), Error);
}

bool UCCLWorldSimulationSubsystem::ChangeSurfaceForcing(FGuid RegionId, const FCCLSurfaceForcing& Forcing, FString& Error)
{
	return CheckAuthority(Error) && SurfaceSimulation.ChangeForcing(RegionId, Forcing, Error);
}

bool UCCLWorldSimulationSubsystem::ApplySnowContact(FGuid RegionId, FGuid SourceId, uint64 Sequence,
	const FVector& PositionMeters, double RadiusMeters, FString& Error)
{
	return CheckAuthority(Error) && bRunning
		&& SurfaceSimulation.ApplySnowContact(RegionId, SourceId, Sequence, PositionMeters, RadiusMeters, Error);
}
