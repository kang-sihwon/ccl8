#include "CCLTerrainRegion.h"

#include "CCLTerrainChunkComponent.h"
#include "Async/Async.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Physics/PhysicsInterfaceCore.h"

ACCLTerrainRegion::ACCLTerrainRegion()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	PrimaryActorTick.EndTickGroup = TG_PrePhysics;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("TerrainRoot"));
	RootComponent->SetMobility(EComponentMobility::Movable);
}

void ACCLTerrainRegion::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bPending && CanMutate())
	{
		AdvancePreparation();
	}
}

void ACCLTerrainRegion::EndPlay(const EEndPlayReason::Type Reason)
{
	CancelPendingEdit();
	for (auto& Pair : ActiveChunks)
	{
		Pair.Value->Discard();
	}

	ActiveChunks.Reset();
	Super::EndPlay(Reason);
}

bool ACCLTerrainRegion::InitializeTerrain(const FCCLTerrainDefinition& Definition, FString& Error)
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

	if (!Store.Initialize(Definition, Error))
	{
		return false;
	}

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
		if (!Result.bSucceeded || Result.Mesh.Epoch != Store.GetEpoch() || Result.Mesh.WorldRevision != PendingSnapshot->Revision
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
		const FGuid Epoch = Store.GetEpoch();
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

	if (!CheckOccupancy(Error))
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

	PreparedChunks.Reset();
	PendingChunks.Reset();
	PendingParticipants.Reset();
	PendingSnapshot.Reset();
	PendingCandidate = FCCLTerrainCandidate();
	Cancellation.Reset();
	bInitialPreparation = 0;
	bPublished = 1;
	bPending = 0;
	bLastSucceeded = 1;
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
