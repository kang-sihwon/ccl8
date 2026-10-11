#include "CCLEcologyRepresentationSubsystem.h"

#include "CCLCreatureActor.h"
#include "CCLWorldSimulationSubsystem.h"
#include "Engine/World.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"

bool UCCLEcologyRepresentationSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const auto* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UCCLEcologyRepresentationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UCCLWorldSimulationSubsystem>();
	Collection.InitializeDependency<UMassEntitySubsystem>();
}

void UCCLEcologyRepresentationSubsystem::Deinitialize()
{
	DiscardRepresentations();
	Super::Deinitialize();
}

void UCCLEcologyRepresentationSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	if (GetWorld()->GetNetMode() == NM_Client || !Runtime || !Runtime->IsRunning())
	{
		return;
	}

	FString Error;
	if (!Flush(Error) && LastError != Error)
	{
		UE_LOG(LogTemp, Warning, TEXT("CCL_ECOLOGY representation update pending: %s"), *Error);
	}

	LastError = Error;
}

TStatId UCCLEcologyRepresentationSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLEcologyRepresentationSubsystem, STATGROUP_Tickables);
}

bool UCCLEcologyRepresentationSubsystem::SetRepresentation(FGuid Id, EKSIEcologyRepresentation Representation, FString& Error)
{
	if (!CanModify(Error))
	{
		return false;
	}

	if (!Flush(Error))
	{
		return false;
	}

	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	const auto& Ecology = Runtime->GetEcologySimulation();
	if (const auto* Existing = Entries.Find(Id); Existing && !Ecology.IsLeaseCurrent(Existing->Lease))
	{
		const auto Stale = *Existing;
		Entries.Remove(Id);
		Retire(Stale);
	}

	const auto* I = Ecology.FindIndividual(Id);
	if (!I || I->State != EKSIEcologyLifeState::Alive || uint8(Representation) > uint8(EKSIEcologyRepresentation::Mass))
	{
		Error = TEXT("Representation requires a live reserved individual and a supported mode.");
		return false;
	}

	const auto* Existing = Entries.Find(Id);
	if (Representation == EKSIEcologyRepresentation::Dormant)
	{
		if (!Existing)
		{
			return Runtime->ReturnEcologyIndividual(Id, Error);
		}

		const auto Old = *Existing;
		if (!Runtime->ReleaseEcologyIndividual(Old.Lease, Error))
		{
			return false;
		}

		Entries.Remove(Id);
		Retire(Old);
		return true;
	}

	auto* EntityManager = Manager();
	if (Existing && Existing->Lease.Representation == Representation
		&& (Representation == EKSIEcologyRepresentation::Actor ? Existing->Actor.IsValid() : EntityManager->IsEntityValid(Existing->Entity)))
	{
		Error.Reset();
		return true;
	}

	const FVector Position = I->PositionMeters;
	const auto Movement = Ecology.GetSpecies()[Ecology.GetPopulations()[I->PopulationId].SpeciesId].Movement;
	FCCLEcologyRepresentationEntry Prepared;
	if (Representation == EKSIEcologyRepresentation::Actor)
	{
		Prepared.Actor = GetWorld()->SpawnActorDeferred<ACCLCreatureActor>(ACCLCreatureActor::StaticClass(),
			FTransform(FRotator::ZeroRotator, Position * 100.), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Prepared.Actor.IsValid())
		{
			Error = TEXT("Failed to prepare ecology Actor.");
			return false;
		}
	}
	else
	{
		const TArray<const UScriptStruct*> Types = {FCCLEcologyMassFragment::StaticStruct()};
		Prepared.Entity = EntityManager->CreateEntity(EntityManager->CreateArchetype(Types));
		if (!EntityManager->IsEntityValid(Prepared.Entity))
		{
			Error = TEXT("Failed to prepare ecology Mass Entity.");
			return false;
		}
	}

	const FGuid Writer = FGuid::NewGuid();
	const bool bAcquired = Existing
		? Runtime->TransferEcologyIndividual(Existing->Lease, Representation, Writer, Prepared.Lease, Error)
		: Runtime->AcquireEcologyIndividual(Id, Representation, Writer, Prepared.Lease, Error);
	if (!bAcquired)
	{
		Retire(Prepared);
		return false;
	}

	const TOptional<FCCLEcologyRepresentationEntry> Old = Existing ? TOptional<FCCLEcologyRepresentationEntry>(*Existing) : TOptional<FCCLEcologyRepresentationEntry>();
	Entries.Add(Id, Prepared);
	if (auto* Actor = Prepared.Actor.Get())
	{
		Actor->Bind(Prepared.Lease, Movement);
		Actor->FinishSpawning(FTransform(FRotator::ZeroRotator, Position * 100.));
	}
	else
	{
		auto& Fragment = EntityManager->GetFragmentDataChecked<FCCLEcologyMassFragment>(Prepared.Entity);
		Fragment.Lease = Prepared.Lease;
		Fragment.PositionMeters = Position;
		Fragment.Medium = uint8(Movement);
	}

	if (Old.IsSet())
	{
		Retire(Old.GetValue());
	}

	return true;
}

bool UCCLEcologyRepresentationSubsystem::SetPosition(FGuid Id, const FVector& PositionMeters, FString& Error)
{
	if (!CanModify(Error))
	{
		return false;
	}

	const auto* E = Entries.Find(Id);
	if (!E || (E->Lease.Representation == EKSIEcologyRepresentation::Actor ? !E->Actor.IsValid() : !Manager()->IsEntityValid(E->Entity)))
	{
		Error = TEXT("A live ecology representation is required to move.");
		return false;
	}

	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	if (!Runtime->UpdateEcologyPosition(E->Lease, PositionMeters, Error))
	{
		return false;
	}

	if (auto* Actor = E->Actor.Get())
	{
		Actor->SetActorLocation(PositionMeters * 100.);
	}
	else
	{
		Manager()->GetFragmentDataChecked<FCCLEcologyMassFragment>(E->Entity).PositionMeters = PositionMeters;
	}

	return true;
}

bool UCCLEcologyRepresentationSubsystem::FinishIndividual(FGuid Id, const FKSIEcologyEvent& Event, FString& Error)
{
	if (!CanModify(Error))
	{
		return false;
	}

	const auto* E = Entries.Find(Id);
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	if (!E || !Runtime->FinishEcologyIndividual(E->Lease, Event, Error))
	{
		return false;
	}

	const auto Old = *E;
	Entries.Remove(Id);
	Retire(Old);
	return true;
}

bool UCCLEcologyRepresentationSubsystem::Flush(FString& Error)
{
	if (!CanModify(Error))
	{
		return false;
	}

	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	const auto& Ecology = Runtime->GetEcologySimulation();
	TArray<FGuid> Ids;
	Entries.GetKeys(Ids);
	for (FGuid Id : Ids)
	{
		const auto E = Entries[Id];
		if (!Ecology.IsLeaseCurrent(E.Lease))
		{
			Entries.Remove(Id);
			Retire(E);
			continue;
		}

		const bool bActor = E.Lease.Representation == EKSIEcologyRepresentation::Actor;
		auto* Actor = E.Actor.Get();
		const bool bExists = bActor ? Actor && !Actor->IsActorBeingDestroyed() : Manager()->IsEntityValid(E.Entity);
		if (!bExists)
		{
			if (!Runtime->ReleaseEcologyIndividual(E.Lease, Error))
			{
				return false;
			}

			Entries.Remove(Id);
			Retire(E);
			continue;
		}

		if (Ecology.HasBehavior(Id))
		{
			const FVector Committed = Ecology.FindIndividual(Id)->PositionMeters;
			if (bActor)
			{
				const FVector Direction = Committed * 100. - Actor->GetActorLocation();
				if (!Direction.IsNearlyZero())
				{
					Actor->SetActorRotation(Direction.Rotation());
				}

				Actor->SetActorLocation(Committed * 100.);
			}
			else
			{
				Manager()->GetFragmentDataChecked<FCCLEcologyMassFragment>(E.Entity).PositionMeters = Committed;
			}
			continue;
		}

		const FVector Position = bActor ? Actor->GetActorLocation() / 100. : Manager()->GetFragmentDataChecked<FCCLEcologyMassFragment>(E.Entity).PositionMeters;
		if (!Runtime->UpdateEcologyPosition(E.Lease, Position, Error))
		{
			const auto* I = Ecology.FindIndividual(Id);
			if (bActor)
			{
				Actor->SetActorLocation(I->PositionMeters * 100.);
			}
			else
			{
				Manager()->GetFragmentDataChecked<FCCLEcologyMassFragment>(E.Entity).PositionMeters = I->PositionMeters;
			}

			return false;
		}
	}

	Error.Reset();
	return true;
}

bool UCCLEcologyRepresentationSubsystem::CanModify(FString& Error) const
{
	const auto* World = GetWorld();
	const auto* Runtime = World ? World->GetSubsystem<UCCLWorldSimulationSubsystem>() : nullptr;
	if (!IsInGameThread() || !World || World->GetNetMode() == NM_Client || !Runtime || !Runtime->IsRunning()
		|| !Manager() || Manager()->IsProcessing())
	{
		Error = TEXT("Ecology representations require a running server and an idle game-thread Mass manager.");
		return false;
	}

	Error.Reset();
	return true;
}

void UCCLEcologyRepresentationSubsystem::DiscardRepresentations()
{
	if (!IsInGameThread() || (Manager() && Manager()->IsProcessing()))
	{
		return;
	}

	auto Previous = MoveTemp(Entries);
	Entries.Reset();
	auto* Runtime = GetWorld() ? GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>() : nullptr;
	for (const auto& Pair : Previous)
	{
		FString Error;
		if (Runtime && Runtime->GetEcologySimulation().IsLeaseCurrent(Pair.Value.Lease))
		{
			Runtime->SuspendEcologyIndividual(Pair.Value.Lease, Error);
		}

		Retire(Pair.Value);
	}
}

void UCCLEcologyRepresentationSubsystem::OnActorEnded(const FKSIEcologyLease& Lease)
{
	const auto* Existing = Entries.Find(Lease.IndividualId);
	if (!Existing || Existing->Lease.Epoch != Lease.Epoch || Existing->Lease.Sequence != Lease.Sequence || Existing->Lease.Writer != Lease.Writer)
	{
		return;
	}

	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	FString Error;
	if (Runtime && Runtime->GetEcologySimulation().IsLeaseCurrent(Lease))
	{
		if (!Runtime->ReleaseEcologyIndividual(Lease, Error))
		{
			return;
		}
	}

	Entries.Remove(Lease.IndividualId);
}

FMassEntityManager* UCCLEcologyRepresentationSubsystem::Manager() const
{
	auto* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UMassEntitySubsystem>() : nullptr;
	return Subsystem ? &Subsystem->GetMutableEntityManager() : nullptr;
}

void UCCLEcologyRepresentationSubsystem::Retire(const FCCLEcologyRepresentationEntry& Entry)
{
	if (auto* Actor = Entry.Actor.Get())
	{
		Actor->Destroy();
	}

	if (auto* EntityManager = Manager(); EntityManager && EntityManager->IsEntityValid(Entry.Entity))
	{
		EntityManager->DestroyEntity(Entry.Entity);
	}
}
