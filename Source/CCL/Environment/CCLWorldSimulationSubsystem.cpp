#include "CCLWorldSimulationSubsystem.h"

#include "CCLWorldAdvanceParticipant.h"

#include "Agents/CCLAgentWorldSubsystem.h"
#include "KSIWorldParticipant.h"
#include "CCLWorldAdvance.h"
#include "CCLThermalExposure.h"
#include "CCLEcologyRepresentationSubsystem.h"
#include "CCLEcologySpace.h"
#include "CCLWorldEnvironmentState.h"
#include "CCLWorldEnvironmentConfig.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "ProfilingDebugging/CsvProfiler.h"

namespace
{
	bool CheckMapInterest(const UWorld* World, const FKSIEnvironmentInputs& Inputs, FString& Error)
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
	Collection.InitializeDependency<UCCLThermalExposureSubsystem>();
}

void UCCLWorldSimulationSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bRunning || GetWorld()->GetNetMode() == NM_Client || GetWorld()->IsPaused() || ManualAdvanceOwner.IsValid())
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
	CSV_CUSTOM_STAT_GLOBAL(CCLWorldCompletedSeconds, Clock.GetWorldSeconds(), ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT_GLOBAL(CCLWorldPendingGameSeconds, Clock.GetPendingGameSeconds(), ECsvCustomStatOp::Set);
	CSV_CUSTOM_STAT_GLOBAL(CCLWorldUnqueuedGameSeconds, UnqueuedGameSeconds, ECsvCustomStatOp::Set);
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
	FKSIWorldClock CandidateClock;
	FKSISurfaceSimulation CandidateSurfaceSimulation;
	FKSIEcologySimulation CandidateEcologySimulation;
	FKSIEnvironmentInputs CandidateInputs = FKSIEnvironmentInputsCodec::MakeDefault(42);
	int32 ConfigCount = 0;
	for (TActorIterator<ACCLWorldEnvironmentConfig> It(GetWorld()); It; ++It)
	{
		if (++ConfigCount > 1 || !It->BuildInputs(CandidateInputs, Error))
		{
			Error = TEXT("A world requires one valid environment config at most.");
			return false;
		}
	}

	FKSICelestialSystem CandidateCelestial;
	FKSISurfaceScene CandidateSurfaces;
	FKSICelestialObservation Observation;
	FCCLWorldSnapshot Snapshot;
	const bool bSaved = !Saved.IsEmpty();
	if (!bSaved)
	{
		if (!CandidateClock.Reset(Life.GetTime(), 60, Error)
			|| !CandidateSurfaceSimulation.Initialize(CandidateIdentity.WorldId, CandidateClock.GetGameSeconds(), Life.GetTime(), 0, Error)
			|| !CandidateEcologySimulation.Initialize(CandidateIdentity.WorldId, Life.GetTime(), Error)
			|| !FKSIEnvironmentInputsCodec::Prepare(CandidateInputs, Life.GetTime(), CandidateCelestial, CandidateSurfaces, Observation, Error))
		{
			return false;
		}
	}
	else
	{
		if (!FCCLWorldSnapshotCodec::Decode(Saved, Snapshot, Error))
		{
			return false;
		}

		if (Snapshot.bEnvironmentMigrated)
		{
			Snapshot.Environment = CandidateInputs;
		}

		if (!CheckMapInterest(GetWorld(), Snapshot.Environment, Error)
			|| !FKSIEnvironmentInputsCodec::CheckDefinition(Snapshot.Environment, CandidateInputs.Celestial.DefinitionId, CandidateInputs.Celestial.Version, Error)
			|| !FKSIEnvironmentInputsCodec::Prepare(Snapshot.Environment, Snapshot.Clock.WorldSeconds, CandidateCelestial, CandidateSurfaces, Observation, Error))
		{
			return false;
		}
	}

	// A new world gives every participant an empty section; a saved world gives each its stored section.
	int32 Prepared = 0;
	const FGuid ParticipantWorld = bSaved ? Snapshot.Identity.WorldId : CandidateIdentity.WorldId;
	const double ParticipantSeconds = bSaved ? Snapshot.Clock.WorldSeconds : Life.GetTime();
	if (!PrepareParticipants(bSaved ? &Snapshot : nullptr, ParticipantWorld, ParticipantSeconds, true, Prepared, Error))
	{
		return false;
	}

	auto CommitCore = [&]()
	{
		if (bSaved)
		{
			if (!FCCLWorldSnapshotCodec::Restore(Snapshot, Domain, CandidateClock, Life, Error, &CandidateSurfaceSimulation, &CandidateEcologySimulation))
			{
				return false;
			}

			CandidateIdentity = Snapshot.Identity;
			CandidateInputs = MoveTemp(Snapshot.Environment);
		}

		Clock = MoveTemp(CandidateClock);
		SurfaceSimulation = MoveTemp(CandidateSurfaceSimulation);
		EcologySimulation = MoveTemp(CandidateEcologySimulation);
		EnvironmentInputs = MoveTemp(CandidateInputs);
		CelestialSystem = MoveTemp(CandidateCelestial);
		CandidateSurfaces.CopyGeometryProviders(SurfaceScene);
		SurfaceScene = MoveTemp(CandidateSurfaces);
		Identity = CandidateIdentity;
		Epoch = FGuid::NewGuid();
		bRunning = 1;
		return true;
	};

	if (!CommitParticipants(0, Prepared, CommitCore))
	{
		CancelParticipants(Prepared);
		return false;
	}

	for (int32 Index = 0; Index < Prepared; ++Index)
	{
		Participants[Index]->PublishWorldRestore();
	}

	return true;
}

bool UCCLWorldSimulationSubsystem::AcquireManualAdvance(UObject* Owner, FString& Error)
{
	if (!CheckAuthority(Error) || !IsValid(Owner) || Owner->GetWorld() != GetWorld() ||
		(ManualAdvanceOwner.IsValid() && ManualAdvanceOwner.Get() != Owner))
	{
		Error = TEXT("Manual time control requires a unique live owner in this server world.");
		return false;
	}
	ManualAdvanceOwner = Owner;
	return true;
}

void UCCLWorldSimulationSubsystem::ReleaseManualAdvance(const UObject* Owner)
{
	if (ManualAdvanceOwner.Get() == Owner)
	{
		ManualAdvanceOwner.Reset();
	}
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

	if (!Agents->FlushExternalDeaths(Error))
	{
		return false;
	}

	TArray<ICCLWorldAdvanceParticipant*> Advancing;
	for (auto* Participant : AdvanceParticipants)
	{
		if (Participant->NeedsWorldAdvance())
		{
			Advancing.Add(Participant);
		}
	}

	// Environment-dependent participant state advances one world second at a time.
	if (!Advancing.IsEmpty())
	{
		MaxWorldSeconds = FMath::Min(MaxWorldSeconds, FMath::FloorToDouble(Clock.GetWorldSeconds()) + 1. - Clock.GetWorldSeconds());
	}

	auto* Thermal = GetWorld()->GetSubsystem<UCCLThermalExposureSubsystem>();
	MaxGameSeconds = FMath::Min(MaxGameSeconds, .25);
	TFunction<bool(const FKSIWorldStep&, const FKSISurfaceSimulation*, FString&)> Prepare =
		[this, &Advancing, Thermal](const FKSIWorldStep& Step, const FKSISurfaceSimulation* Surface, FString& Reason)
	{
		if (!Thermal || !Thermal->Prepare(Step, SurfaceSimulation, Reason))
		{
			return false;
		}

		for (auto* Participant : Advancing)
		{
			if (!Surface || !Participant->PrepareWorldAdvance(Step, *Surface, Reason))
			{
				return false;
			}
		}

		return true;
	};
	const FCCLEcologyWorldSpace Space(GetWorld());
	if (!CCLWorldAdvance::Advance(Clock, Agents->GetSimulation(), MaxGameSeconds, MaxWorldSeconds, Error, MaxLifeSlices, &SurfaceSimulation, &CelestialSystem, &EnvironmentInputs.Observer, &SurfaceScene, &EnvironmentInputs.Openings, &EcologySimulation, 256, &Space, Prepare))
	{
		Thermal->Cancel();
		for (auto* Participant : Advancing)
		{
			Participant->CancelWorldAdvance();
		}

		ReportFailure(Error);
		Publish();
		return false;
	}

	// Every participant commits before any of them notifies listeners.
	for (auto* Participant : Advancing)
	{
		Participant->CommitWorldAdvance();
	}

	for (auto* Participant : Advancing)
	{
		Participant->PublishWorldAdvance();
	}

	Thermal->Commit();

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

bool UCCLWorldSimulationSubsystem::Save(const FCCLLifeSimulation& Life, TArray<uint8>& Bytes, FString& Error, const UObject* SessionContainer)
{
	if (!CheckAuthority(Error) || UnqueuedGameSeconds != 0)
	{
		Error = TEXT("Cannot save a client, unstarted world or unaccepted input.");
		return false;
	}

	if (auto* Agents = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>(); Agents && !Agents->FlushExternalDeaths(Error))
	{
		return false;
	}
	if (auto* Representations = GetWorld()->GetSubsystem<UCCLEcologyRepresentationSubsystem>(); Representations && !Representations->Flush(Error))
	{
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
	if (!FCCLWorldSnapshotCodec::Capture(NextIdentity, Clock, Life, Snapshot, Error, &EnvironmentInputs, &SurfaceSimulation, &EcologySimulation) ||
		!CaptureParticipants(Snapshot, SessionContainer, Error) || !FCCLWorldSnapshotCodec::Encode(Snapshot, Bytes, Error))
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

	if (auto* Representations = GetWorld()->GetSubsystem<UCCLEcologyRepresentationSubsystem>(); Representations && !Representations->CanModify(Error))
	{
		return false;
	}

	FCCLWorldSnapshot Snapshot;
	FKSICelestialSystem CandidateCelestial;
	FKSISurfaceScene CandidateSurfaces;
	FKSICelestialObservation Observation;
	if (!FCCLWorldSnapshotCodec::Decode(Bytes, Snapshot, Error))
	{
		return false;
	}

	if (Snapshot.bEnvironmentMigrated)
	{
		Snapshot.Environment = FKSIEnvironmentInputsCodec::MakeDefault(42);
		if (const auto* Config = ACCLWorldEnvironmentConfig::Find(GetWorld()); Config && !Config->BuildInputs(Snapshot.Environment, Error))
		{
			return false;
		}
	}

	if (!CheckMapInterest(GetWorld(), Snapshot.Environment, Error)
		|| !FKSIEnvironmentInputsCodec::CheckDefinition(Snapshot.Environment, EnvironmentInputs.Celestial.DefinitionId, EnvironmentInputs.Celestial.Version, Error)
		|| !FKSIEnvironmentInputsCodec::Prepare(Snapshot.Environment, Snapshot.Clock.WorldSeconds, CandidateCelestial, CandidateSurfaces, Observation, Error))
	{
		return false;
	}

	int32 Prepared = 0;
	if (!PrepareParticipants(&Snapshot, Snapshot.Identity.WorldId, Snapshot.Clock.WorldSeconds, false, Prepared, Error))
	{
		return false;
	}

	auto CommitCore = [&]()
	{
		if (!FCCLWorldSnapshotCodec::Restore(Snapshot, Identity.Domain, Clock, Life, Error, &SurfaceSimulation, &EcologySimulation))
		{
			return false;
		}

		EnvironmentInputs = MoveTemp(Snapshot.Environment);
		CelestialSystem = MoveTemp(CandidateCelestial);
		CandidateSurfaces.CopyGeometryProviders(SurfaceScene);
		SurfaceScene = MoveTemp(CandidateSurfaces);
		Identity = Snapshot.Identity;
		Epoch = FGuid::NewGuid();
		if (auto* Representations = GetWorld()->GetSubsystem<UCCLEcologyRepresentationSubsystem>())
		{
			Representations->DiscardRepresentations();
		}

		UnqueuedGameSeconds = 0;
		LastError.Reset();
		return true;
	};

	if (!CommitParticipants(0, Prepared, CommitCore))
	{
		CancelParticipants(Prepared);
		return false;
	}

	for (int32 Index = 0; Index < Prepared; ++Index)
	{
		Participants[Index]->PublishWorldRestore();
	}

	Publish();
	return true;
}

void UCCLWorldSimulationSubsystem::RegisterParticipant(IKSIWorldParticipant& Participant)
{
	check(!bRunning && !Participants.Contains(&Participant));
	Participants.Add(&Participant);
	Participants.Sort([](const IKSIWorldParticipant& A, const IKSIWorldParticipant& B)
	{
		return A.GetWorldOrder() < B.GetWorldOrder();
	});
}

void UCCLWorldSimulationSubsystem::UnregisterParticipant(IKSIWorldParticipant& Participant)
{
	Participants.Remove(&Participant);
}

void UCCLWorldSimulationSubsystem::RegisterAdvanceParticipant(ICCLWorldAdvanceParticipant& Participant)
{
	check(!bRunning && !AdvanceParticipants.Contains(&Participant));
	AdvanceParticipants.Add(&Participant);
	AdvanceParticipants.Sort([](const ICCLWorldAdvanceParticipant& A, const ICCLWorldAdvanceParticipant& B)
	{
		return A.GetWorldAdvanceOrder() < B.GetWorldAdvanceOrder();
	});
}

void UCCLWorldSimulationSubsystem::UnregisterAdvanceParticipant(ICCLWorldAdvanceParticipant& Participant)
{
	AdvanceParticipants.Remove(&Participant);
}

bool UCCLWorldSimulationSubsystem::ReplaceEnvironmentInputs(const FKSIEnvironmentInputs& Candidate, FString& Error)
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

	FKSICelestialSystem CandidateCelestial;
	FKSISurfaceScene CandidateSurfaces;
	FKSICelestialObservation Observation;
	if (!CheckMapInterest(GetWorld(), Candidate, Error)
		|| !FKSIEnvironmentInputsCodec::CheckDefinition(Candidate, EnvironmentInputs.Celestial.DefinitionId, EnvironmentInputs.Celestial.Version, Error)
		|| !FKSIEnvironmentInputsCodec::Prepare(Candidate, Clock.GetWorldSeconds(), CandidateCelestial, CandidateSurfaces, Observation, Error)
		|| !SurfaceSimulation.ValidateAirContext(Candidate.Openings, Error))
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

bool UCCLWorldSimulationSubsystem::ObserveCelestials(FKSICelestialObservation& Observation, FString& Error) const
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
	static_assert(static_cast<uint8>(ECCLWorldContentDomain::Campaign) == static_cast<uint8>(ECCLWorldDomain::Campaign));
	static_assert(static_cast<uint8>(ECCLWorldContentDomain::Playground) == static_cast<uint8>(ECCLWorldDomain::Playground));
	static_assert(static_cast<uint8>(ECCLWorldContentDomain::Scenario) == static_cast<uint8>(ECCLWorldDomain::Scenario));

	// Maps declare their store in data. A map without environment config uses the campaign store.
	const auto* Config = ACCLWorldEnvironmentConfig::Find(World);
	return Config ? static_cast<ECCLWorldDomain>(Config->Domain) : ECCLWorldDomain::Campaign;
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

bool UCCLWorldSimulationSubsystem::BuildEnvironmentView(FKSIEnvironmentView& OutView, FString& Error) const
{
	FKSICelestialObservation Observation;
	if (!ObserveCelestials(Observation, Error))
	{
		return false;
	}

	FKSIEnvironmentView View;
	View.DefinitionId = EnvironmentInputs.Celestial.DefinitionId;
	View.DefinitionVersion = EnvironmentInputs.Celestial.Version;
	View.Seed = EnvironmentInputs.Celestial.Seed;
	View.Observer = EnvironmentInputs.Observer;
	View.CelestialEpochSeconds = EnvironmentInputs.Celestial.EpochWorldSeconds;
	View.CelestialBodies = EnvironmentInputs.Celestial.Bodies;
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
		FKSICelestialSourceView Star;
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
		FKSICelestialBodyView Sky;
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
			FKSIShelterQuery Query;
			Query.BodyId = View.Observer.BodyId;
			Query.PositionMeters = Probe.PositionMeters;
			Query.ToSun = ToSun;
			Query.ToPrecipitationSource = Probe.ToPrecipitationSource;
			Query.ToWindSource = Probe.ToWindSource;
			Query.RequiredEpoch = View.SurfaceEpoch;
			Query.RequiredRevision = View.SurfaceRevision;
			FKSIShelterSample Sample;
			if (!FKSIShelterEvaluator::Evaluate(SurfaceScene, Query, Sample, Error))
			{
				return false;
			}

			FKSIEnvironmentProbeView Result;
			Result.ProbeId = Probe.ProbeId;
			Result.PositionMeters = Probe.PositionMeters;
			Result.ToSun = ToSun;
			Result.ToPrecipitationSource = Probe.ToPrecipitationSource;
			Result.ToWindSource = Probe.ToWindSource;
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

bool UCCLWorldSimulationSubsystem::CaptureParticipants(FCCLWorldSnapshot& Snapshot, const UObject* SessionContainer, FString& Error)
{
	FKSIWorldCaptureContext Context;
	Context.WorldId = Identity.WorldId;
	Context.WorldSeconds = Clock.GetWorldSeconds();
	Context.SessionContainer = SessionContainer;
	for (auto* Participant : Participants)
	{
		TArray<uint8> Section;
		if (!Participant->CaptureWorldSection(Context, Section, Error))
		{
			return false;
		}

		Snapshot.SetSection(Participant->GetWorldSection(), MoveTemp(Section));
	}

	return true;
}

bool UCCLWorldSimulationSubsystem::PrepareParticipants(const FCCLWorldSnapshot* Snapshot, FGuid WorldId, double WorldSeconds,
	bool bStarting, int32& OutPrepared, FString& Error)
{
	FKSIWorldRestoreContext Context;
	Context.WorldId = WorldId;
	Context.WorldSeconds = WorldSeconds;
	Context.bStarting = bStarting;
	OutPrepared = 0;
	for (auto* Participant : Participants)
	{
		const TArray<uint8>* Section = Snapshot ? Snapshot->FindSection(Participant->GetWorldSection()) : nullptr;
		if (!Participant->PrepareWorldRestore(Context, Section, Error))
		{
			CancelParticipants(OutPrepared);
			return false;
		}

		++OutPrepared;
	}

	return true;
}

bool UCCLWorldSimulationSubsystem::CommitParticipants(int32 Index, int32 Prepared, TFunctionRef<bool()> CommitCore)
{
	// The lowest participant wraps every later commit and the core world in its own commit.
	if (Index >= Prepared)
	{
		return CommitCore();
	}

	return Participants[Index]->CommitWorldRestore([this, Index, Prepared, CommitCore]()
	{
		return CommitParticipants(Index + 1, Prepared, CommitCore);
	});
}

void UCCLWorldSimulationSubsystem::CancelParticipants(int32 Prepared)
{
	for (int32 Index = Prepared - 1; Index >= 0; --Index)
	{
		Participants[Index]->CancelWorldRestore();
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

void UCCLWorldSimulationSubsystem::SetGeometryProvider(FGuid Id, TSharedPtr<const IKSISurfaceProvider> Provider)
{
	SurfaceScene.SetGeometryProvider(Id, MoveTemp(Provider));
	if (bRunning && GetWorld()->GetNetMode() != NM_Client && !GetWorld()->bIsTearingDown)
	{
		Publish();
	}
}

bool UCCLWorldSimulationSubsystem::AddEcologySpecies(const FKSIEcologySpecies& Species, FString& Error)
{
	return CheckAuthority(Error) && EcologySimulation.AddSpecies(Species, Error);
}

bool UCCLWorldSimulationSubsystem::AddEcologyHabitat(const FKSIEcologyHabitat& Habitat, FString& Error)
{
	return CheckAuthority(Error) && EcologySimulation.AddHabitat(Habitat, Error);
}

bool UCCLWorldSimulationSubsystem::AddEcologyPopulation(FKSIEcologyPopulation Population, FString& Error)
{
	return CheckAuthority(Error) && EcologySimulation.AddPopulation(MoveTemp(Population), Error);
}

bool UCCLWorldSimulationSubsystem::ApplyEcologyEvent(const FKSIEcologyEvent& Event, FString& Error)
{
	return CheckAuthority(Error) && EcologySimulation.Apply(Event, Error);
}

bool UCCLWorldSimulationSubsystem::AcquireEcologyIndividual(FGuid Id, EKSIEcologyRepresentation Representation,
	FGuid Writer, FKSIEcologyLease& Lease, FString& Error)
{
	return CheckAuthority(Error) && EcologySimulation.Acquire(Id, Representation, Writer, Lease, Error);
}

bool UCCLWorldSimulationSubsystem::ReleaseEcologyIndividual(const FKSIEcologyLease& Lease, FString& Error)
{
	return CheckAuthority(Error) && EcologySimulation.Release(Lease, Error);
}

bool UCCLWorldSimulationSubsystem::SuspendEcologyIndividual(const FKSIEcologyLease& Lease, FString& Error)
{
	return CheckAuthority(Error) && EcologySimulation.Suspend(Lease, Error);
}

bool UCCLWorldSimulationSubsystem::ReturnEcologyIndividual(FGuid Id, FString& Error)
{
	return CheckAuthority(Error) && EcologySimulation.ReturnToAggregate(Id, Error);
}

bool UCCLWorldSimulationSubsystem::TransferEcologyIndividual(const FKSIEcologyLease& Lease,
	EKSIEcologyRepresentation Representation, FGuid Writer, FKSIEcologyLease& OutLease, FString& Error)
{
	return CheckAuthority(Error) && EcologySimulation.Transfer(Lease, Representation, Writer, OutLease, Error);
}

bool UCCLWorldSimulationSubsystem::UpdateEcologyPosition(const FKSIEcologyLease& Lease, const FVector& PositionMeters, FString& Error)
{
	return CheckAuthority(Error) && EcologySimulation.UpdatePosition(Lease, PositionMeters, Error);
}

bool UCCLWorldSimulationSubsystem::FinishEcologyIndividual(const FKSIEcologyLease& Lease, const FKSIEcologyEvent& Event, FString& Error)
{
	return CheckAuthority(Error) && EcologySimulation.Finish(Lease, Event, Error);
}

bool UCCLWorldSimulationSubsystem::AddSurfaceRegion(FKSISurfaceGrid Region, FString& Error)
{
	return CheckAuthority(Error) && SurfaceSimulation.AddRegion(MoveTemp(Region), Error);
}

bool UCCLWorldSimulationSubsystem::AddAirSpace(const FKSIAirSpace& Space, FString& Error)
{
	if (!CheckAuthority(Error))
	{
		return false;
	}
	FKSISurfaceSimulation Candidate = SurfaceSimulation;
	if (!Candidate.AddAirSpace(Space, Error) || !Candidate.ValidateAirContext(EnvironmentInputs.Openings, Error))
	{
		return false;
	}
	SurfaceSimulation = MoveTemp(Candidate);
	return true;
}

bool UCCLWorldSimulationSubsystem::IgniteSurface(FGuid RegionId, int32 CellIndex, double HeatJoules, FString& Error)
{
	return CheckAuthority(Error) && SurfaceSimulation.Ignite(RegionId, CellIndex, HeatJoules, Error);
}

bool UCCLWorldSimulationSubsystem::ChangeClimate(FGuid RegionId, const FKSIClimate& Climate, FString& Error)
{
	return CheckAuthority(Error) && SurfaceSimulation.ChangeClimate(RegionId, Climate, Error);
}

bool UCCLWorldSimulationSubsystem::ChangeSurfaceForcing(FGuid RegionId, const FKSISurfaceForcing& Forcing, FString& Error)
{
	return CheckAuthority(Error) && SurfaceSimulation.ChangeForcing(RegionId, Forcing, Error);
}

bool UCCLWorldSimulationSubsystem::ApplySnowContact(FGuid RegionId, FGuid SourceId, uint64 Sequence,
	const FVector& PositionMeters, double RadiusMeters, FString& Error)
{
	return CheckAuthority(Error) && bRunning
		&& SurfaceSimulation.ApplySnowContact(RegionId, SourceId, Sequence, PositionMeters, RadiusMeters, Error);
}


bool UCCLWorldSimulationSubsystem::AddEcologyGroup(const FKSIEcologyGroup& Group, FString& Error)
{
	return CheckAuthority(Error) && EcologySimulation.AddGroup(Group, Error);
}

bool UCCLWorldSimulationSubsystem::AddEcologyForage(FKSIEcologyForage Forage, FString& Error)
{
	return CheckAuthority(Error) && EcologySimulation.AddForage(Forage, Error);
}

bool UCCLWorldSimulationSubsystem::AddEcologyBehavior(FKSIEcologyBehavior Behavior, FString& Error)
{
	return CheckAuthority(Error) && EcologySimulation.AddBehavior(Behavior, Error);
}


bool UCCLWorldSimulationSubsystem::SeedEcology(const FKSIEcologySimulation& Candidate,FString& Error)
{
	if (!CheckAuthority(Error)||!EcologySimulation.GetSpecies().IsEmpty()||!EcologySimulation.GetIndividuals().IsEmpty()
		||!EcologySimulation.GetHabitats().IsEmpty()||!EcologySimulation.GetPopulations().IsEmpty()
		||!EcologySimulation.GetGroups().IsEmpty()||!EcologySimulation.GetForage().IsEmpty()
		||!EcologySimulation.GetBehaviors().IsEmpty()||EcologySimulation.ReceiptCount()!=0
		||Candidate.GetWorldId()!=Identity.WorldId||Candidate.GetWorldSeconds()!=Clock.GetWorldSeconds()||!Candidate.Validate(Error))
	{
		Error=TEXT("Ecology seeding requires an empty server model and matching world/time.");
		return false;
	}
	TArray<uint8> Bytes;
	FKSIEcologySimulation Prepared;
	if (!Candidate.Capture(Bytes,Error)||!Prepared.Restore(Bytes,Error))
	{
		return false;
	}

	EcologySimulation=MoveTemp(Prepared);
	return true;
}

bool UCCLWorldSimulationSubsystem::ReadWorldTime(FKSIWorldTimeView& OutTime) const
{
    if (!IsRunning())
    {
        return false;
    }
    OutTime.WorldId = Identity.WorldId;
    OutTime.CompletedStepId = Clock.GetCompletedStepId();
    OutTime.GameSeconds = Clock.GetGameSeconds();
    OutTime.WorldSeconds = Clock.GetWorldSeconds();
    return true;
}
