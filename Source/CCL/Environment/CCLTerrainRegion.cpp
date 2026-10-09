#include "CCLTerrainRegion.h"

#include "CCLTerrainChunkComponent.h"
#include "CCLTerrainSurface.h"
#include "NavigationSystem.h"
#include "NavigationData.h"
#include "Net/UnrealNetwork.h"
#include "CCLWorldSimulationSubsystem.h"
#include "Async/Async.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Physics/PhysicsInterfaceCore.h"

ACCLTerrainRegion::ACCLTerrainRegion()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(true);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	PrimaryActorTick.EndTickGroup = TG_PrePhysics;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("TerrainRoot"));
	RootComponent->SetMobility(EComponentMobility::Movable);
}

void ACCLTerrainRegion::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateNavigationReadiness();
	if (bPending && IsInGameThread() && GetWorld() && GetWorld()->IsGameWorld())
	{
		AdvancePreparation();
	}
}

void ACCLTerrainRegion::EndPlay(const EEndPlayReason::Type Reason)
{
	CancelPendingEdit();
	if (auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		Nav->OnNavigationGenerationFinishedDelegate.RemoveDynamic(this, &ACCLTerrainRegion::NavigationFinished);
	}

	if (Store.IsInitialized())
	{
		if (auto* Simulation = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>())
		{
			Simulation->SetGeometryProvider(Store.GetSnapshot().Definition.RegionId, nullptr);
		}
	}

	SurfaceProvider.Reset();
	SurfaceMeshes.Reset();
	for (auto& Pair : ActiveChunks)
	{
		Pair.Value->Discard();
	}

	ActiveChunks.Reset();
	Super::EndPlay(Reason);
}

bool ACCLTerrainRegion::InitializeTerrain(const FCCLTerrainDefinition& Definition, FString& Error, const TArray<uint8>& SavedBytes)
{
	Error.Reset();
	if (!CanMutate() || bPublished || bPending || !GetActorScale3D().Equals(FVector::OneVector, 1.e-6)
		|| !FCCLTerrainStore::ValidateDefinition(Definition, Error))
	{
		if (Error.IsEmpty())
		{
			Error = TEXT("Terrain initialization requires a server region with unit scale and no active or pending terrain.");
		}

		return false;
	}

	const FIntVector First = FCCLTerrainStore::OwnerForSample(Definition.MinimumCell);
	const FIntVector Last = FCCLTerrainStore::OwnerForSample(Definition.MaximumCell - FIntVector(1));
	int64 Count = 1;
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		const int64 Size = int64(Last[Axis]) - First[Axis] + 1;
		if (Size <= 0 || Size > 256 || Count > 256 / Size)
		{
			Error = TEXT("A loaded terrain region is limited to 256 chunks; split larger regions before loading.");
			return false;
		}

		Count *= Size;
	}

	FCCLTerrainStore InitialStore;
	if (!InitialStore.Initialize(Definition, Error))
	{
		return false;
	}

	if (!SavedBytes.IsEmpty())
	{
		FCCLTerrainSnapshot Saved;
		FCCLTerrainSaveContext Context;
		if (!FCCLTerrainCodec::Decode(SavedBytes, Saved, Context, Error) || !InitialStore.Restore(SavedBytes, Context, Error))
		{
			return false;
		}
	}

	Store = MoveTemp(InitialStore);
	TArray<FIntVector> Chunks;
	for (int32 Z = First.Z; Z <= Last.Z; ++Z)
	{
		for (int32 Y = First.Y; Y <= Last.Y; ++Y)
		{
			for (int32 X = First.X; X <= Last.X; ++X)
			{
				Chunks.Add(FIntVector(X, Y, Z));
			}
		}
	}

	InitialTransform = GetActorTransform();
	PendingTicket = FGuid::NewGuid();
	bInitialPreparation = 1;
	StartPreparation(Store.GetSnapshot(), MoveTemp(Chunks));
	return true;
}

bool ACCLTerrainRegion::SetEditAuthority(const FCCLTerrainAuthority& Authority, FString& Error)
{
	Error.Reset();
	const auto* Previous = Authorities.Find(Authority.PrincipalId);
	if (!CanMutate() || !Authority.PrincipalId.IsValid() || Authority.PolicyRevision == 0
		|| (Previous && Authority.PolicyRevision <= Previous->PolicyRevision))
	{
		Error = TEXT("Terrain authority requires a server principal and an increasing policy revision.");
		return false;
	}

	Authorities.Add(Authority.PrincipalId, Authority);
	return true;
}

ECCLTerrainPrepareResult ACCLTerrainRegion::RequestEdit(const FCCLTerrainEdit& Request, FString& Error)
{
	Error.Reset();
	const auto* Authority = Authorities.Find(Request.PrincipalId);
	if (!CanMutate() || !bPublished || bPending || !Authority || !InitialTransform.Equals(GetActorTransform()))
	{
		Error = TEXT("Terrain request requires a ready server region, registered authority and unchanged region transform.");
		return ECCLTerrainPrepareResult::Failed;
	}

	FCCLTerrainCandidate Candidate;
	const auto Result = Store.PrepareEdit(Request, *Authority, Candidate, Error);
	if (Result != ECCLTerrainPrepareResult::Prepared)
	{
		return Result;
	}

	PendingCandidate = MoveTemp(Candidate);
	PendingPrincipal = Request.PrincipalId;
	PendingTicket = PendingCandidate.GetTicket();
	PendingParticipants = Participants;
	for (const auto& Participant : PendingParticipants)
	{
		if (!Participant->Prepare(PendingCandidate, Error))
		{
			FailPending(Error);
			return ECCLTerrainPrepareResult::Failed;
		}
	}

	bInitialPreparation = 0;
	StartPreparation(PendingCandidate.GetSnapshot(), PendingCandidate.GetMeshChunks());
	return Result;
}

bool ACCLTerrainRegion::RequestRestore(const TArray<uint8>& Bytes, const FCCLTerrainSaveContext& Context, FString& Error, TFunction<bool(FString&)> BeforePublish, const FCCLWorldSnapshot* WorldState)
{
	Error.Reset();
	if (!CanMutate() || !bPublished || bPending || (BeforePublish && !Participants.IsEmpty() && !WorldState) || !InitialTransform.Equals(GetActorTransform()))
	{
		Error = TEXT("Terrain restore requires an idle authoritative region without unprepared dependent participants.");
		return false;
	}

	if (WorldState && (!BeforePublish || WorldState->Identity.WorldId != Context.WorldId
		|| WorldState->Identity.BaseWorldVersion != Context.BaseWorldVersion || WorldState->Identity.Generation != Context.WorldGeneration
		|| WorldState->Clock.GameSeconds != Context.GameSeconds || WorldState->Clock.WorldSeconds != Context.WorldSeconds))
	{
		Error = TEXT("Terrain restore context does not match the coordinated world snapshot.");
		return false;
	}

	FCCLTerrainStore Candidate = Store;
	if (BeforePublish && Context.WorldId != Store.GetSnapshot().Definition.WorldId)
	{
		auto Definition = Store.GetSnapshot().Definition;
		Definition.WorldId = Context.WorldId;
		if (!Candidate.Initialize(Definition, Error))
		{
			return false;
		}
	}

	if (!Candidate.Restore(Bytes, Context, Error))
	{
		return false;
	}

	for (const auto& Participant : Participants)
	{
		if (!Participant->ValidateRestore(Candidate.GetSnapshot(), WorldState, Error))
		{
			return false;
		}
	}

	// Reuse prepared collision only when the complete canonical terrain record is identical.
	auto Current = Store.GetSnapshot();
	Current.Definition.WorldId = Context.WorldId;
	TArray<uint8> CurrentBytes;
	if (BeforePublish && FCCLTerrainCodec::Encode(Current, Context, CurrentBytes, Error) && CurrentBytes == Bytes)
	{
		if (!BeforePublish(Error))
		{
			return false;
		}

		for (const auto& Participant : Participants)
		{
			Participant->CommitRestore();
		}
		const bool bNavigationWasReady = IsNavigationReady();
		Store = MoveTemp(Candidate);
		TArray<TSharedRef<const FCCLTerrainMesh>> Meshes;
		SurfaceMeshes.GenerateValueArray(Meshes);
		SurfaceProvider = MakeShared<FCCLTerrainSurface>(Store.GetSnapshot().Definition, Store.GetRevision(), Store.GetEpoch(), InitialTransform, MoveTemp(Meshes));
		if (auto* Simulation = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>())
		{
			Simulation->SetGeometryProvider(Store.GetSnapshot().Definition.RegionId, SurfaceProvider);
		}

		if (bNavigationWasReady)
		{
			NavigationEpoch = Store.GetEpoch();
			NavigationRevision = Store.GetRevision();
		}

		++PublicationSerial;
		bLastSucceeded = 1;
		LastError.Reset();
		ForceNetUpdate();
		return true;
	}

	RestoreStore = MoveTemp(Candidate);
	RestoreBeforePublish = MoveTemp(BeforePublish);
	RestoreWorldState.Reset();
	if (WorldState)
	{
		RestoreWorldState = MakeShared<FCCLWorldSnapshot>(*WorldState);
	}
	TArray<FIntVector> Chunks;
	ActiveChunks.GenerateKeyArray(Chunks);
	PendingTicket = FGuid::NewGuid();
	bInitialPreparation = 1;
	bRestoring = 1;
	StartPreparation(RestoreStore.GetSnapshot(), MoveTemp(Chunks));
	return true;
}

bool ACCLTerrainRegion::AddParticipant(TSharedRef<ICCLTerrainEditParticipant> Participant, FString& Error)
{
	if (!CanMutate() || bPending || !bPublished || Participants.Contains(Participant))
	{
		Error = TEXT("Terrain participant registration requires an idle server region and a unique participant.");
		return false;
	}

	Participants.Add(Participant);
	return true;
}

void ACCLTerrainRegion::CancelPendingEdit()
{
	if (bPending || PendingCandidate.IsValid())
	{
		FailPending(TEXT("Terrain preparation cancelled; committed terrain retained."));
	}
}

void ACCLTerrainRegion::StartPreparation(const FCCLTerrainSnapshot& Snapshot, TArray<FIntVector> Chunks)
{
	PreparationEpoch = bRestoring ? RestoreStore.GetEpoch() : Store.GetEpoch();
	PendingSnapshot = MakeShared<FCCLTerrainSnapshot, ESPMode::ThreadSafe>(Snapshot);
	Cancellation = MakeShared<FThreadSafeBool, ESPMode::ThreadSafe>(false);
	PendingChunks = MoveTemp(Chunks);
	NextChunk = 0;
	PreparationStarted = FPlatformTime::Seconds();
	bPending = 1;
	bLastSucceeded = 0;
	LastError.Reset();
	AdvancePreparation();
}

void ACCLTerrainRegion::AdvancePreparation()
{
	if (FPlatformTime::Seconds() - PreparationStarted > 60. || !InitialTransform.Equals(GetActorTransform()))
	{
		FailPending(TEXT("Terrain preparation timed out or the region transform changed."));
		return;
	}

	for (int32 Index = MeshJobs.Num() - 1; Index >= 0; --Index)
	{
		if (!MeshJobs[Index].Future.IsReady())
		{
			continue;
		}

		auto Result = MeshJobs[Index].Future.Get();
		MeshJobs.RemoveAtSwap(Index);
		if (!Result.bSucceeded || Result.Mesh.Epoch != PreparationEpoch || Result.Mesh.WorldRevision != PendingSnapshot->Revision
			|| Result.Mesh.WorldId != PendingSnapshot->Definition.WorldId || Result.Mesh.RegionId != PendingSnapshot->Definition.RegionId)
		{
			FailPending(Result.Error.IsEmpty() ? TEXT("Stale terrain mesh result discarded.") : Result.Error);
			return;
		}

		auto* Component = NewObject<UCCLTerrainChunkComponent>(this);
		Component->SetupAttachment(RootComponent);
		PreparedChunks.Add(Result.Mesh.Chunk, Component);
#if WITH_DEV_AUTOMATION_TESTS
		if (bRejectNextCook && !Result.Mesh.Triangles.IsEmpty())
		{
			Component->RejectCookForTesting();
			bRejectNextCook = 0;
		}
#endif
		FString Error;
		if (!Component->Prepare(MoveTemp(Result.Mesh), Error))
		{
			FailPending(Error);
			return;
		}
	}

	bool bAllReady = true;
	for (const auto& Pair : PreparedChunks)
	{
		const auto State = Pair.Value->GetPreparationState();
		if (State == ECCLTerrainChunkState::Failed || State == ECCLTerrainChunkState::Cancelled)
		{
			FailPending(Pair.Value->GetFailure());
			return;
		}

		bAllReady &= State == ECCLTerrainChunkState::Ready;
	}

	while (NextChunk < PendingChunks.Num() && MeshJobs.Num() < 2)
	{
		const FIntVector Chunk = PendingChunks[NextChunk++];
		const FGuid Epoch = PreparationEpoch;
		auto Snapshot = PendingSnapshot;
		auto Cancel = Cancellation;
		FCCLTerrainMeshJob Job;
		Job.Future = Async(EAsyncExecution::ThreadPool, [Snapshot, Cancel, Chunk, Epoch]()
		{
			FCCLTerrainMeshJobResult Result;
			Result.bSucceeded = FCCLTerrainMesher::BuildChunk(*Snapshot, Chunk, Epoch, Result.Mesh, Result.Error,
				[Cancel]() { return bool(*Cancel); }) ? 1 : 0;
			return Result;
		});
		MeshJobs.Add(MoveTemp(Job));
	}

	if (bAllReady && MeshJobs.IsEmpty() && NextChunk == PendingChunks.Num() && PreparedChunks.Num() == PendingChunks.Num())
	{
		PublishPrepared();
	}
}

void ACCLTerrainRegion::PublishPrepared()
{
	check(IsInGameThread());
	FString Error;
	const auto* Authority = Authorities.Find(PendingPrincipal);
	if (!bInitialPreparation && (!Authority || !Store.ValidateCandidate(PendingCandidate, *Authority, Error)))
	{
		FailPending(Error.IsEmpty() ? TEXT("Terrain authority no longer exists.") : Error);
		return;
	}

	if (HasAuthority() && !CheckOccupancy(Error))
	{
		FailPending(Error);
		return;
	}

	for (const auto& Participant : PendingParticipants)
	{
		if (!Participant->ValidateCommit(PendingCandidate, Error))
		{
			FailPending(Error);
			return;
		}
	}

	if (bRestoring)
	{
		for (const auto& Participant : Participants)
		{
			if (!Participant->ValidateRestore(RestoreStore.GetSnapshot(), RestoreWorldState.Get(), Error))
			{
				FailPending(Error);
				return;
			}
		}
	}

	bool bActivated = false;
	FPhysicsCommand::ExecuteWrite(GetWorld()->GetPhysicsScene(), [&]()
	{
		auto ActivateBatch = [&]()
		{
			for (auto& Pair : PreparedChunks)
			{
				if (!Pair.Value->ActivatePrepared(Error))
				{
					return false;
				}
			}

			if (!bInitialPreparation && !Store.CommitEdit(PendingCandidate, *Authority, Error))
			{
				return false;
			}

			if (bRestoring && RestoreBeforePublish && !RestoreBeforePublish(Error))
			{
				return false;
			}

			if (bRestoring)
			{
				Store = MoveTemp(RestoreStore);
				for (const auto& Participant : Participants)
				{
					Participant->CommitRestore();
				}
			}

			for (auto& Pair : PreparedChunks)
			{
				if (auto* Old = ActiveChunks.Find(Pair.Key))
				{
					(*Old)->Discard();
				}

				ActiveChunks.Add(Pair.Key, Pair.Value);
			}

			for (const auto& Participant : PendingParticipants)
			{
				Participant->Commit(PendingCandidate);
			}

			return true;
		};
		bActivated = ActivateBatch();
		if (!bActivated)
		{
			// Roll back before releasing the scene lock, while old committed bodies still exist.
			for (auto& Pair : PreparedChunks)
			{
				Pair.Value->Discard();
			}
		}
	});
	if (!bActivated)
	{
		FailPending(Error.IsEmpty() ? TEXT("Terrain physics scene unavailable at publication.") : Error);
		return;
	}

	NavigationRevision = 0;
	NavigationEpoch.Invalidate();
	bNavigationObserved = 0;
	NavigationPublicationFrame = GFrameCounter;
	if (auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		Nav->OnNavigationGenerationFinishedDelegate.AddUniqueDynamic(this, &ACCLTerrainRegion::NavigationFinished);
		for (const auto& Pair : PreparedChunks)
		{
			Pair.Value->SetCanEverAffectNavigation(true);
		}

		Nav->AddDirtyArea(GetWorldTerrainBounds(), ENavigationDirtyFlag::All, TEXT("CommittedTerrain"));
	}

	for (const auto& Pair : PreparedChunks)
	{
		SurfaceMeshes.Add(Pair.Key, MakeShared<FCCLTerrainMesh>(Pair.Value->GetSourceMesh()));
	}

	TArray<TSharedRef<const FCCLTerrainMesh>> Meshes;
	SurfaceMeshes.GenerateValueArray(Meshes);
	SurfaceProvider = MakeShared<FCCLTerrainSurface>(Store.GetSnapshot().Definition, Store.GetRevision(),
		Store.GetEpoch(), InitialTransform, MoveTemp(Meshes));
	if (auto* Simulation = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>())
	{
		Simulation->SetGeometryProvider(Store.GetSnapshot().Definition.RegionId, SurfaceProvider);
	}

	PreparedChunks.Reset();
	PendingChunks.Reset();
	PendingParticipants.Reset();
	PendingSnapshot.Reset();
	PendingCandidate = FCCLTerrainCandidate();
	Cancellation.Reset();
	bInitialPreparation = 0;
	RestoreBeforePublish = {};
	RestoreWorldState.Reset();
	bRestoring = 0;
	bPublished = 1;
	bPending = 0;
	bLastSucceeded = 1;
	if (HasAuthority())
	{
		++PublicationSerial;
		ForceNetUpdate();
	}
	else
	{
		ReplicaSerial = PendingReplicaSerial;
	}

	LastError.Reset();
}

void ACCLTerrainRegion::FailPending(const FString& Error)
{
	LastError = Error;
	if (Cancellation)
	{
		*Cancellation = true;
	}

	for (auto& Pair : PreparedChunks)
	{
		Pair.Value->Discard();
	}

	for (const auto& Participant : PendingParticipants)
	{
		Participant->Abort(PendingCandidate);
	}

	PreparedChunks.Reset();
	PendingParticipants.Reset();
	MeshJobs.Reset();
	PendingSnapshot.Reset();
	PendingChunks.Reset();
	PendingCandidate = FCCLTerrainCandidate();
	Cancellation.Reset();
	bPending = 0;
	RestoreBeforePublish = {};
	RestoreWorldState.Reset();
	bRestoring = 0;
	RestoreStore = FCCLTerrainStore();
	bInitialPreparation = 0;
	bLastSucceeded = 0;
}

bool ACCLTerrainRegion::CheckOccupancy(FString& Error) const
{
	const auto& Snapshot = *PendingSnapshot;
	const FBox Bounds = bInitialPreparation ? FCCLTerrainStore::EditableBoundsMeters(Snapshot.Definition)
		: PendingCandidate.GetAffectedBoundsMeters();
	for (TActorIterator<ACharacter> It(GetWorld()); It; ++It)
	{
		const auto* Capsule = It->GetCapsuleComponent();
		if (!Capsule || Capsule->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
		{
			continue;
		}

		const FVector WorldCenter = Capsule->GetComponentLocation();
		const FVector RegionCenter = GetActorTransform().InverseTransformPosition(WorldCenter) * 0.01;
		const double Radius = FMath::Max(0.01, double(Capsule->GetScaledCapsuleRadius()) - 0.1);
		const double HalfHeight = FMath::Max(Radius, double(Capsule->GetScaledCapsuleHalfHeight()) - 0.1);
		if (!Bounds.ExpandBy(HalfHeight * 0.01).IsInsideOrOn(RegionCenter))
		{
			continue;
		}

		double Density = 0.;
		if (!FCCLTerrainStore::ReadDensityMeters(Snapshot, RegionCenter, Density, Error) || Density < -0.001)
		{
			Error = TEXT("Terrain publication would bury a character inside solid material.");
			return false;
		}

		for (const auto& Pair : PreparedChunks)
		{
			if (Pair.Value->OverlapsCapsule(WorldCenter, Capsule->GetUpVector(), Radius, HalfHeight))
			{
				Error = TEXT("Terrain publication would overlap a character capsule.");
				return false;
			}
		}
	}

	return true;
}

bool ACCLTerrainRegion::CanMutate() const
{
	return IsInGameThread() && HasAuthority() && GetWorld() && GetWorld()->IsGameWorld() && GetNetMode() != NM_Client;
}

const ICCLSurfaceProvider* ACCLTerrainRegion::GetSurfaceProvider() const
{
	return SurfaceProvider.Get();
}

FBox ACCLTerrainRegion::GetWorldTerrainBounds() const
{
	if (!Store.IsInitialized())
	{
		return FBox(EForceInit::ForceInit);
	}

	const FBox Local = FCCLTerrainStore::EditableBoundsMeters(Store.GetSnapshot().Definition);
	return FBox(Local.Min * 100., Local.Max * 100.).TransformBy(GetActorTransform());
}

void ACCLTerrainRegion::NavigationFinished(ANavigationData* NavData)
{
	if (NavData && NavData->GetRuntimeGenerationMode() == ERuntimeGenerationType::Dynamic && GFrameCounter > NavigationPublicationFrame)
	{
		bNavigationObserved = 1;
	}
}

void ACCLTerrainRegion::UpdateNavigationReadiness()
{
	if (bPublished && !IsNavigationReady() && FParse::Param(FCommandLine::Get(), TEXT("CCLTerrainSmoke")) && GFrameCounter % 300 == 0)
	{
		auto* DebugNav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		UE_LOG(LogTemp, Display, TEXT("CCL_TERRAIN_NAV Region=%s observed=%d dirty=%d building=%d data=%s bounds=%s"),
			*GetName(), int32(bNavigationObserved), DebugNav ? int32(DebugNav->IsNavigationDirty()) : -1,
			int32(UNavigationSystemV1::IsNavigationBeingBuiltOrLocked(GetWorld())),
			*GetNameSafe(DebugNav ? DebugNav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate) : nullptr), *GetWorldTerrainBounds().ToString());
	}

	if (!bPublished || !bNavigationObserved || GFrameCounter <= NavigationPublicationFrame + 1 || GetNetMode() == NM_Client)
	{
		return;
	}

	auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (Nav && Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate) && Nav->GetNumRemainingBuildTasks() == 0
		&& !UNavigationSystemV1::IsNavigationBeingBuiltOrLocked(GetWorld()))
	{
		NavigationRevision = Store.GetRevision();
		NavigationEpoch = Store.GetEpoch();
	}
}

void ACCLTerrainRegion::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACCLTerrainRegion, PublicationSerial);
}

void ACCLTerrainRegion::OnRep_Publication()
{
	if (bPending && PendingReplicaSerial < PublicationSerial)
	{
		CancelPendingEdit();
	}
}

bool ACCLTerrainRegion::CaptureReplica(TArray<uint8>& Bytes, FString& Error) const
{
	if (!HasAuthority() || !bPublished)
	{
		return false;
	}

	FCCLTerrainSaveContext Context;
	Context.WorldId = Store.GetSnapshot().Definition.WorldId;
	Context.BaseWorldVersion = Store.GetSnapshot().Definition.BaseWorldVersion;
	Context.WorldGeneration = 1;
	return Store.Capture(Context, Bytes, Error);
}

bool ACCLTerrainRegion::ApplyReplica(const TArray<uint8>& Bytes, uint64 Serial, FString& Error)
{
	if (GetNetMode() != NM_Client || Serial == 0 || Serial < PublicationSerial || Serial < ReplicaSerial || Serial < PendingReplicaSerial)
	{
		Error = TEXT("Stale or non-client terrain replica rejected.");
		return false;
	}

	if (Serial == ReplicaSerial || (bPending && Serial == PendingReplicaSerial))
	{
		const auto& Existing = Serial == ReplicaSerial ? Store : RestoreStore;
		FCCLTerrainSaveContext ExistingContext;
		ExistingContext.WorldId = Existing.GetSnapshot().Definition.WorldId;
		ExistingContext.BaseWorldVersion = Existing.GetSnapshot().Definition.BaseWorldVersion;
		ExistingContext.WorldGeneration = 1;
		TArray<uint8> ExistingBytes;
		if (Existing.Capture(ExistingContext, ExistingBytes, Error) && ExistingBytes == Bytes)
		{
			// The transfer may have expired while collision was cooking or its ready ACK was in flight.
			// Keep the existing preparation/publication; the transport still waits for IsReplicaReady().
			return true;
		}

		Error = TEXT("Conflicting terrain bytes for an existing publication serial.");
		return false;
	}

	FCCLTerrainSnapshot Snapshot;
	FCCLTerrainSaveContext Context;
	FCCLTerrainStore Candidate;
	if (!FCCLTerrainCodec::Decode(Bytes, Snapshot, Context, Error)) { return false; }
    if (Store.IsInitialized())
    {
        auto ExpectedDefinition = Store.GetSnapshot().Definition;
        // A newer server publication may restore a different saved world identity.
        ExpectedDefinition.WorldId = Snapshot.Definition.WorldId;
        if (!FCCLTerrainCodec::SameDefinition(ExpectedDefinition, Snapshot.Definition))
        {
            Error = TEXT("Replica region definition does not match the loaded region.");
            return false;
        }
    }
    if (!Candidate.Initialize(Snapshot.Definition, Error) || !Candidate.Restore(Bytes, Context, Error)) { return false; }

	const auto& D = Snapshot.Definition;
	const FIntVector First = FCCLTerrainStore::OwnerForSample(D.MinimumCell);
	const FIntVector Last = FCCLTerrainStore::OwnerForSample(D.MaximumCell - FIntVector(1));
	int64 Count = 1;
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		const int64 Size = int64(Last[Axis]) - First[Axis] + 1;
		if (Size <= 0 || Size > 256 || Count > 256 / Size)
		{
			Error = TEXT("Replica exceeds loaded region budget.");
			return false;
		}

		Count *= Size;
	}

	CancelPendingEdit();
	RestoreStore = MoveTemp(Candidate);
	TArray<FIntVector> Chunks;
	for (int32 Z = First.Z; Z <= Last.Z; ++Z)
	{
		for (int32 Y = First.Y; Y <= Last.Y; ++Y)
		{
			for (int32 X = First.X; X <= Last.X; ++X)
			{
				Chunks.Add(FIntVector(X, Y, Z));
			}
		}
	}

	PendingReplicaSerial = Serial;
	InitialTransform = GetActorTransform();
	PendingTicket = FGuid::NewGuid();
	bInitialPreparation = 1;
	bRestoring = 1;
	StartPreparation(RestoreStore.GetSnapshot(), MoveTemp(Chunks));
	return true;
}
