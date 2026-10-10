#include "CCLEcologyRepresentationSmokeSubsystem.h"

#include "Environment/CCLEcologyRepresentationSubsystem.h"
#include "Environment/CCLCreatureActor.h"
#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/Paths.h"
#include "MassEntitySubsystem.h"

namespace
{
	FGuid RepresentationId(uint32 N)
	{
		return FGuid(910, 911, 912, N);
	}
}

bool UCCLEcologyRepresentationSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLEcologyRepresentationSmoke"));
#endif
}

void UCCLEcologyRepresentationSmokeSubsystem::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	if (!Started)
	{
		Started = Now;
	}

	if (Now - Started > 180.)
	{
		Finish(false, FString::Printf(TEXT("timeout step=%d"), Step));
		return;
	}

	auto* World = GetWorld();
	auto* Runtime = World ? World->GetSubsystem<UCCLWorldSimulationSubsystem>() : nullptr;
	auto* Agents = World ? World->GetSubsystem<UCCLAgentWorldSubsystem>() : nullptr;
	auto* Representations = World ? World->GetSubsystem<UCCLEcologyRepresentationSubsystem>() : nullptr;
	auto* Mass = World ? World->GetSubsystem<UMassEntitySubsystem>() : nullptr;
	auto* Director = World ? ACCLExperimentDirector::Find(World) : nullptr;
	if (World && World->GetNetMode() == NM_Client && Representations)
	{
		int32 Seen = 0;
		for (TActorIterator<ACCLCreatureActor> It(World); It; ++It)
		{
			for (uint32 I = 0; I < 4; ++I)
			{
				if (It->GetIndividualId() == RepresentationId(90 + I))
				{
					const FVector Expected(11. + I * 3., 41., 1.5);
					if (It->GetMovementMedium() != ECCLEcologyMovement(I) || !It->GetActorLocation().Equals(Expected * 100., .1))
					{
						return;
					}

					++Seen;
				}
			}
		}

		if (Step == 0 && Seen == 4)
		{
			FString Error;
			if (Representations->SetRepresentation(RepresentationId(90), ECCLEcologyRepresentation::Mass, Error))
			{
				Finish(false, TEXT("client changed server ecology representation"));
				return;
			}

			UE_LOG(LogTemp, Display, TEXT("CCL_ECOLOGY_REPRESENTATION OBSERVED_READY four actor IDs media and positions; client writes rejected"));
			Step = 100;
		}
		else if (Step == 100 && Seen == 0)
		{
			Finish(true, TEXT("client received four creature Actors and authoritative death removal"));
		}

		return;
	}

	if (!Runtime || !Runtime->IsRunning() || !Agents || !Agents->IsRunning() || !Representations || !Mass
		|| !Director || !Director->IsReady() || Now < Next)
	{
		return;
	}

	FString Error;
	auto Check = [&](bool bOK, const TCHAR* Label)
	{
		if (!bOK)
		{
			Finish(false, FString::Printf(TEXT("step=%d %s: %s"), Step, Label, *Error));
		}

		return bOK;
	};
	const auto& Ecology = Runtime->GetEcologySimulation();
	auto& Manager = Mass->GetMutableEntityManager();
	if (Step == 0)
	{
		if (!Check(Runtime->ChangeTimeScale(0., Error), TEXT("freeze")))
		{
			return;
		}

		UE_LOG(LogTemp, Display, TEXT("CCL_ECOLOGY_REPRESENTATION BASE_READY"));
		Step = 1;
		Next = Now + 2.;
		return;
	}

	if (Step == 1)
	{
		InitialTime = Runtime->GetClock().GetWorldSeconds();
		auto Reserve = [&](uint32 Population, uint32 Id, bool bPersistent)
		{
			FCCLEcologyEvent E;
			E.Id = RepresentationId(Id + 100);
			E.Kind = ECCLEcologyEventKind::Materialize;
			E.Source = RepresentationId(Population);
			E.CohortId = RepresentationId(Population + 10);
			E.IndividualId = RepresentationId(Id);
			E.BirthWorldSeconds = InitialTime - 20.;
			E.bPersistent = bPersistent;
			return Runtime->ApplyEcologyEvent(E, Error);
		};
		for (uint32 I = 0; I < 4; ++I)
		{
			FCCLEcologySpecies S;
			S.Id = RepresentationId(10 + I);
			S.Movement = ECCLEcologyMovement(I);
			S.MaturitySeconds = 10.;
			S.LifespanSeconds = 200.;
			FCCLEcologyHabitat H;
			H.Id = RepresentationId(20 + I);
			H.MinimumMeters = FVector(10. + I * 3., 40., 0.);
			H.MaximumMeters = H.MinimumMeters + FVector(2., 2., 3.);
			H.MovementMask = 1 << I;
			FCCLEcologyPopulation P;
			P.Id = RepresentationId(30 + I);
			P.SpeciesId = S.Id;
			P.HabitatId = H.Id;
			P.Cohorts.Add(RepresentationId(40 + I), {RepresentationId(40 + I), InitialTime - 20., 10});
			if (!Check(Runtime->AddEcologySpecies(S, Error) && Runtime->AddEcologyHabitat(H, Error)
				&& Runtime->AddEcologyPopulation(P, Error) && Reserve(30 + I, 50 + I, I == 0), TEXT("four-media fixture")))
			{
				return;
			}

			const FGuid Id = RepresentationId(50 + I);
			if (!Check(Representations->SetRepresentation(Id, ECCLEcologyRepresentation::Actor, Error), TEXT("spawn Actor")))
			{
				return;
			}

			const auto ActorEntry = *Representations->Find(Id);
			if (!Check(ActorEntry.Actor.IsValid() && ActorEntry.Actor->GetIndividualId() == Id
				&& Ecology.AggregateCount(P.Id) == 9 && Ecology.LivingCount(P.Id) == 10, TEXT("actual Actor reservation")))
			{
				return;
			}

			const FVector Position = (H.MinimumMeters + H.MaximumMeters) * .5 + FVector(.25, 0., 0.);
			ActorEntry.Actor->SetActorLocation(Position * 100.);
			if (!Check(Representations->SetRepresentation(Id, ECCLEcologyRepresentation::Mass, Error), TEXT("Actor to Mass flushes pending position")))
			{
				return;
			}

			const auto MassEntry = *Representations->Find(Id);
			Representations->OnActorEnded(ActorEntry.Lease);
			if (!Check(Manager.IsEntityValid(MassEntry.Entity) && Ecology.IsLeaseCurrent(MassEntry.Lease)
				&& (!ActorEntry.Actor.IsValid() || ActorEntry.Actor->IsActorBeingDestroyed())
				&& Manager.GetFragmentDataChecked<FCCLEcologyMassFragment>(MassEntry.Entity).PositionMeters == Position
				&& Ecology.LivingCount(P.Id) == 10 && Ecology.AggregateCount(P.Id) == 9, TEXT("Mass identity and stale Actor end")))
			{
				return;
			}

			{
				auto Processing = Manager.NewProcessingScope();
				if (!Check(!Representations->SetRepresentation(Id, ECCLEcologyRepresentation::Actor, Error)
					&& Ecology.IsLeaseCurrent(MassEntry.Lease), TEXT("processing guard")))
				{
					return;
				}
			}

			const FVector MassPosition = Position + FVector(0., .1, 0.);
			Manager.GetFragmentDataChecked<FCCLEcologyMassFragment>(MassEntry.Entity).PositionMeters = MassPosition;
			if (!Check(Representations->SetRepresentation(Id, ECCLEcologyRepresentation::Actor, Error)
				&& !Manager.IsEntityValid(MassEntry.Entity)
				&& Representations->Find(Id)->Actor->GetActorLocation().Equals(MassPosition * 100., .001)
				&& !Representations->SetPosition(Id, H.MaximumMeters + FVector(1.), Error)
				&& Representations->SetRepresentation(Id, ECCLEcologyRepresentation::Dormant, Error)
				&& Ecology.LivingCount(P.Id) == 10 && Ecology.AggregateCount(P.Id) == (I == 0 ? 9 : 10), TEXT("Mass to Actor to aggregate")))
			{
				return;
			}
		}

		const FGuid Important = RepresentationId(50);
		if (!Check(Representations->SetRepresentation(Important, ECCLEcologyRepresentation::Mass, Error), TEXT("prepare saved Mass")))
		{
			return;
		}

		const auto SavedEntry = *Representations->Find(Important);
		auto& Fragment = Manager.GetFragmentDataChecked<FCCLEcologyMassFragment>(SavedEntry.Entity);
		Fragment.PositionMeters += FVector(.1, .2, 0.);
		const FVector SavedPosition = Fragment.PositionMeters;
		TArray<uint8> Saved;
		if (!Check(Agents->Save(Saved) && Ecology.FindIndividual(Important)->PositionMeters == SavedPosition
			&& Representations->SetPosition(Important, SavedPosition + FVector(.1, 0., 0.), Error)
			&& Agents->Restore(Saved, Error) && Representations->Count() == 0 && !Manager.IsEntityValid(SavedEntry.Entity)
			&& Ecology.FindIndividual(Important)->PositionMeters == SavedPosition
			&& !Runtime->ReleaseEcologyIndividual(SavedEntry.Lease, Error)
			&& Representations->SetRepresentation(Important, ECCLEcologyRepresentation::Actor, Error), TEXT("save flush and restore invalidation")))
		{
			return;
		}

		FCCLEcologyEvent Death;
		Death.Id = RepresentationId(250);
		Death.Kind = ECCLEcologyEventKind::Death;
		Death.Source = RepresentationId(30);
		Death.CohortId = RepresentationId(40);
		Death.IndividualId = Important;
		Death.BirthWorldSeconds = InitialTime - 20.;
		const auto Dying = *Representations->Find(Important);
		if (!Check(Representations->FinishIndividual(Important, Death, Error)
			&& Runtime->FinishEcologyIndividual(Dying.Lease, Death, Error)
			&& Ecology.LivingCount(Death.Source) == 9 && !Representations->Find(Important), TEXT("actual Actor death exactly once")))
		{
			return;
		}

		if (!Check(Reserve(31, 75, false) && Representations->SetRepresentation(RepresentationId(75), ECCLEcologyRepresentation::Mass, Error), TEXT("external Entity removal fixture")))
		{
			return;
		}

		Manager.DestroyEntity(Representations->Find(RepresentationId(75))->Entity);
		if (!Check(Representations->Flush(Error) && Ecology.AggregateCount(RepresentationId(31)) == 10, TEXT("removed Entity returned once")))
		{
			return;
		}

		if (!Check(Reserve(32, 76, false) && Representations->SetRepresentation(RepresentationId(76), ECCLEcologyRepresentation::Actor, Error), TEXT("external Actor removal fixture")))
		{
			return;
		}

		Representations->Find(RepresentationId(76))->Actor->Destroy();
		if (!Check(Ecology.AggregateCount(RepresentationId(32)) == 10 && !Representations->Find(RepresentationId(76)), TEXT("removed Actor returned once")))
		{
			return;
		}

		FCCLEcologyLease OtherWriter;
		if (!Check(Reserve(33, 77, false) && Runtime->AcquireEcologyIndividual(RepresentationId(77), ECCLEcologyRepresentation::Actor, RepresentationId(90), OtherWriter, Error), TEXT("conflicting writer fixture")))
		{
			return;
		}

#if WITH_MASSENTITY_DEBUG
		const int32 BeforeEntities = Manager.DebugGetEntityCount();
#endif
		if (!Check(!Representations->SetRepresentation(RepresentationId(77), ECCLEcologyRepresentation::Mass, Error)
			&& Ecology.IsLeaseCurrent(OtherWriter) && Ecology.AggregateCount(RepresentationId(33)) == 9, TEXT("prepared failure retains old writer")))
		{
			return;
		}

#if WITH_MASSENTITY_DEBUG
		if (!Check(Manager.DebugGetEntityCount() == BeforeEntities, TEXT("failed preparation leaks no Entity")))
		{
			return;
		}
#endif
		if (!Check(Runtime->ReleaseEcologyIndividual(OtherWriter, Error)
			&& Reserve(31, 80, false) && Reserve(32, 81, true)
			&& Representations->SetRepresentation(RepresentationId(80), ECCLEcologyRepresentation::Mass, Error)
			&& Representations->SetRepresentation(RepresentationId(81), ECCLEcologyRepresentation::Actor, Error), TEXT("natural death fixture")))
		{
			return;
		}

		for (uint32 I = 0; I < 4; ++I)
		{
			if (!Check(Reserve(30 + I, 90 + I, true)
				&& Representations->SetRepresentation(RepresentationId(90 + I), ECCLEcologyRepresentation::Actor, Error), TEXT("replicated four-media Actors")))
			{
				return;
			}
		}

		UE_LOG(LogTemp, Display, TEXT("CCL_ECOLOGY_REPRESENTATION HANDOFF_READY four media actual Actor Mass save restore and failure verified"));
		Step = 2;
		Next = Now + 2.;
		return;
	}

	if (Step == 2)
	{
		FString Directory;
		FParse::Value(FCommandLine::Get(), TEXT("CCLEcologyRepresentationDir="), Directory);
		if (World->GetNetMode() != NM_Standalone && !FPaths::FileExists(Directory / TEXT("clients-ready.txt")))
		{
			return;
		}

		if (!Check(Runtime->ChangeTimeScale(60., Error), TEXT("resume mortality after observers are ready")))
		{
			return;
		}

		Step = 3;
		return;
	}

	if (Step == 3 && Runtime->GetClock().GetWorldSeconds() >= InitialTime + 200.)
	{
		if (!Check(Representations->Flush(Error) && Representations->Count() == 0
			&& Ecology.FindIndividual(RepresentationId(80))->State == ECCLEcologyLifeState::Dead
			&& Ecology.FindIndividual(RepresentationId(81))->State == ECCLEcologyLifeState::Dead
			&& Ecology.LivingCount(RepresentationId(31)) == 0 && Ecology.LivingCount(RepresentationId(32)) == 0, TEXT("lifespan removes both representations")))
		{
			return;
		}

		Finish(true, TEXT("actual four-media Actor/Mass handoff, positions, save/restore, stale callbacks, cleanup and natural death"));
	}
}

void UCCLEcologyRepresentationSmokeSubsystem::Finish(bool bSuccess, const FString& Message)
{
	bComplete = 1;
	UE_LOG(LogTemp, Display, TEXT("CCL_ECOLOGY_REPRESENTATION %s %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Message);
	if (!bSuccess || !GetWorld() || GetWorld()->GetNetMode() == NM_Standalone || GetWorld()->GetNetMode() == NM_Client)
	{
		FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1, TEXT("CCLEcologyRepresentationSmoke"));
	}
}

TStatId UCCLEcologyRepresentationSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLEcologyRepresentationSmokeSubsystem, STATGROUP_Tickables);
}
