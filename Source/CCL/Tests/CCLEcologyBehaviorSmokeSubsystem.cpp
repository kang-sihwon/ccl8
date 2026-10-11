#include "CCLEcologyBehaviorSmokeSubsystem.h"
#include "Environment/CCLEcologyRepresentationSubsystem.h"
#include "Environment/CCLCreatureActor.h"
#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MassEntitySubsystem.h"

namespace
{
	FGuid Id(uint32 N)
	{
		return FGuid(930, 931, 932, N);
	}

	FVector InitialPosition(uint32 N)
	{
		return FVector(12. + N * 5., 42., 2.);
	}
}

bool UCCLEcologyBehaviorSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLEcologyBehaviorSmoke"));
#endif
}

void UCCLEcologyBehaviorSmokeSubsystem::Tick(float DeltaTime)
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
	auto* R = World ? World->GetSubsystem<UCCLEcologyRepresentationSubsystem>() : nullptr;
	auto* Mass = World ? World->GetSubsystem<UMassEntitySubsystem>() : nullptr;
	auto* Director = World ? ACCLExperimentDirector::Find(World) : nullptr;
	FString Directory;
	FParse::Value(FCommandLine::Get(), TEXT("CCLEcologyBehaviorDir="), Directory);
	FString Error;
	auto Check = [&](bool bOK, const TCHAR* Label)
	{
		if (!bOK)
		{
			Finish(false, FString::Printf(TEXT("step=%d %s: %s"), Step, Label, *Error));
		}

		return bOK;
	};
	if (!World || !Runtime || !Agents || !R || !Mass)
	{
		return;
	}

	if (World->GetNetMode() == NM_Client)
	{
		FString ExpectedText;
		TArray<FString> Lines;
		const bool bFinal = FFileHelper::LoadFileToString(ExpectedText, *(Directory / TEXT("poses.txt")));
		ExpectedText.ParseIntoArrayLines(Lines);
		if (bFinal && Lines.Num() != 4)
		{
			return;
		}

		int32 Seen = 0;
		for (TActorIterator<ACCLCreatureActor> It(World); It; ++It)
		{
			for (uint32 N = 0; N < 4; ++N)
			{
				if (It->GetIndividualId() != Id(60 + N))
				{
					continue;
				}

				FVector Expected = InitialPosition(N);
				if (bFinal)
				{
					TArray<FString> Parts;
					Lines[N].ParseIntoArray(Parts, TEXT(","));
					if (!Check(Parts.Num() == 3, TEXT("expected position fixture")))
					{
						return;
					}

					Expected = FVector(FCString::Atod(*Parts[0]), FCString::Atod(*Parts[1]), FCString::Atod(*Parts[2]));
				}

				if (It->GetMovementMedium() == ECCLEcologyMovement(N) && It->GetActorLocation().Equals(Expected * 100., 1.))
				{
					++Seen;
				}
			}
		}

		if (Seen != 4)
		{
			return;
		}

		if (!bFinal)
		{
			if (!Step)
			{
				UE_LOG(LogTemp, Display, TEXT("CCL_ECOLOGY_BEHAVIOR BASE_OBSERVED"));
				Step = 1;
			}

			return;
		}
		FCCLEcologyBehavior Forbidden;
		Forbidden.Id = Id(60);
		if (!Check(!Runtime->AddEcologyBehavior(Forbidden, Error), TEXT("client behavior mutation refused")))
		{
			return;
		}

		Finish(true, TEXT("four server behavior positions restored and replicated; authority enforced"));
		return;
	}

	if (!Runtime->IsRunning() || !Director || !Director->IsReady() || Now < Next)
	{
		return;
	}

	const auto& E = Runtime->GetEcologySimulation();
	auto& Manager = Mass->GetMutableEntityManager();
	const bool bRead = FParse::Param(FCommandLine::Get(), TEXT("CCLEcologyBehaviorRead"));
	if (Step == 0)
	{
		if (!Check(Runtime->ChangeTimeScale(0., Error), TEXT("freeze")))
		{
			return;
		}

		Step = 1;
		Next = Now + 2.;
		return;
	}
	if (Step == 1)
	{
		if (bRead)
		{
			if (!Check(FFileHelper::LoadFileToArray(SavedWorld, *(Directory / TEXT("world.bin")))
				&& FFileHelper::LoadFileToArray(SavedEcology, *(Directory / TEXT("ecology.bin")))
				&& Runtime->Restore(Agents->GetSimulation(), SavedWorld, Error), TEXT("new process restore")))
			{
				return;
			}

			Step = 6;
			return;
		}
		InitialTime = Runtime->GetClock().GetWorldSeconds();
		for (uint32 N = 0; N < 4; ++N)
		{
			FCCLEcologySpecies S;
			S.Id = Id(10 + N);
			S.Movement = ECCLEcologyMovement(N);
			FCCLEcologyHabitat H;
			H.Id = Id(20 + N);
			H.MinimumMeters = InitialPosition(N) - FVector(2.);
			H.MaximumMeters = H.MinimumMeters + FVector(4.);
			H.MovementMask = 1 << N;
			FCCLEcologyPopulation P;
			P.Id = Id(30 + N);
			P.SpeciesId = S.Id;
			P.HabitatId = H.Id;
			P.Cohorts.Add(Id(40 + N), {Id(40 + N), InitialTime, 10});
			FCCLEcologyEvent Event;
			Event.Id = Id(50 + N);
			Event.Kind = ECCLEcologyEventKind::Materialize;
			Event.Source = P.Id;
			Event.CohortId = Id(40 + N);
			Event.IndividualId = Id(60 + N);
			Event.BirthWorldSeconds = InitialTime;
			Event.bPersistent = 1;
			FCCLEcologyGroup G;
			G.Id = Id(70 + N);
			G.HabitatId = H.Id;
			G.AnchorMeters = InitialPosition(N) - FVector(1., 1., 0.);
			G.RadiusMeters = .7;
			FCCLEcologyForage F;
			F.Id = Id(80 + N);
			F.HabitatId = H.Id;
			F.PositionMeters = InitialPosition(N) + FVector(1., 0., N == 1 || N == 2 ? 1. : 0.);
			F.FoodMask = 1 << N;
			FCCLEcologyBehavior B;
			B.Id = Event.IndividualId;
			B.GroupId = G.Id;
			B.DietMask = F.FoodMask;
			B.Energy = .2;
			B.MetabolismPerSecond = .02;
			if (!Check(Runtime->AddEcologySpecies(S, Error) && Runtime->AddEcologyHabitat(H, Error) && Runtime->AddEcologyPopulation(P, Error)
				&& Runtime->ApplyEcologyEvent(Event, Error) && Runtime->AddEcologyGroup(G, Error) && Runtime->AddEcologyForage(F, Error)
				&& Runtime->AddEcologyBehavior(B, Error) && R->SetRepresentation(B.Id, ECCLEcologyRepresentation::Actor, Error), TEXT("behavior fixture")))
			{
				return;
			}
		}
		UE_LOG(LogTemp, Display, TEXT("CCL_ECOLOGY_BEHAVIOR BASE_READY"));
		Step = 2;
		return;
	}
	if (Step == 2)
	{
		if (World->GetNetMode() != NM_Standalone && !FPaths::FileExists(Directory / TEXT("first-ready.txt")))
		{
			return;
		}

		if (!Check(Runtime->ChangeTimeScale(10., Error), TEXT("advance behavior")))
		{
			return;
		}

		Step = 3;
		return;
	}
	if (Step == 3)
	{
		if (Runtime->GetClock().GetWorldSeconds() < InitialTime + 20.)
		{
			return;
		}

		if (!Check(Runtime->ChangeTimeScale(0., Error), TEXT("pause after feeding")))
		{
			return;
		}

		Step = 4;
		Next = Now + 2.;
		return;
	}
	if (Step == 4)
	{
		if (!Check(R->Flush(Error), TEXT("publish behavior poses")))
		{
			return;
		}

		for (uint32 N = 0; N < 4; ++N)
		{
			const FGuid Individual = Id(60 + N);
			const FVector Position = E.FindIndividual(Individual)->PositionMeters;
			const auto Previous = *R->Find(Individual);
			if (!Check(E.GetBehaviors()[Individual].ConsumedUnits > 0. && !Position.Equals(InitialPosition(N))
				&& Previous.Actor.IsValid() && Previous.Actor->GetActorLocation().Equals(Position * 100., .01)
				&& !R->SetPosition(Individual, InitialPosition(N), Error)
				&& R->SetRepresentation(Individual, ECCLEcologyRepresentation::Mass, Error), TEXT("actual autonomous Actor and Mass handoff")))
			{
				return;
			}

			R->OnActorEnded(Previous.Lease);
			const auto Entry = *R->Find(Individual);
			if (!Check(Manager.GetFragmentDataChecked<FCCLEcologyMassFragment>(Entry.Entity).PositionMeters == Position
				&& E.IsLeaseCurrent(Entry.Lease) && E.LivingCount(Id(30 + N)) == 10
				&& R->SetRepresentation(Individual, ECCLEcologyRepresentation::Actor, Error), TEXT("Mass preserves behavior identity and counts")))
			{
				return;
			}
		}
		if (!Check(Runtime->Save(Agents->GetSimulation(), SavedWorld, Error) && E.Capture(SavedEcology, Error)
			&& FFileHelper::SaveArrayToFile(SavedWorld, *(Directory / TEXT("world.bin")))
			&& FFileHelper::SaveArrayToFile(SavedEcology, *(Directory / TEXT("ecology.bin")))
			&& Runtime->ChangeTimeScale(10., Error), TEXT("save real behavior and forage")))
		{
			return;
		}

		InitialTime = Runtime->GetClock().GetWorldSeconds();
		Step = 5;
		return;
	}
	if (Step == 5)
	{
		if (Runtime->GetClock().GetWorldSeconds() < InitialTime + 10.)
		{
			return;
		}

		if (!Check(Runtime->Restore(Agents->GetSimulation(), SavedWorld, Error), TEXT("restore behavior after further movement")))
		{
			return;
		}

		Step = 6;
		return;
	}
	if (Step == 6)
	{
		TArray<uint8> Actual;
		if (!Check(E.Capture(Actual, Error) && Actual == SavedEcology, TEXT("exact behavior and food history restore")))
		{
			return;
		}

		FString Poses;
		for (uint32 N = 0; N < 4; ++N)
		{
			const FVector Position = E.FindIndividual(Id(60 + N))->PositionMeters;
			if (!Check(R->SetRepresentation(Id(60 + N), ECCLEcologyRepresentation::Actor, Error), TEXT("recreate saved behavior Actor")))
			{
				return;
			}

			Poses += FString::Printf(TEXT("%.12f,%.12f,%.12f\n"), Position.X, Position.Y, Position.Z);
		}

		if (!Check(FFileHelper::SaveStringToFile(Poses, *(Directory / TEXT("poses.txt"))), TEXT("publish frozen position oracle")))
		{
			return;
		}

		UE_LOG(LogTemp, Display, TEXT("CCL_ECOLOGY_BEHAVIOR RESTORE_READY"));
		Step = 7;
		Next = Now + 1.;
		return;
	}
	if (Step == 7)
	{
		if (World->GetNetMode() != NM_Standalone && !FPaths::FileExists(Directory / TEXT("clients-ready.txt")))
		{
			return;
		}

		Finish(true, TEXT("four-media behavior feeding Actor/Mass handoff exact world save and restore"));
	}
}

void UCCLEcologyBehaviorSmokeSubsystem::Finish(bool bSuccess, const FString& Message)
{
	bComplete = 1;
	UE_LOG(LogTemp, Display, TEXT("CCL_ECOLOGY_BEHAVIOR %s %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Message);
	if (!bSuccess || !GetWorld() || GetWorld()->GetNetMode() == NM_Standalone || GetWorld()->GetNetMode() == NM_Client)
	{
		FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1, TEXT("CCLEcologyBehaviorSmoke"));
	}
}

TStatId UCCLEcologyBehaviorSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLEcologyBehaviorSmokeSubsystem, STATGROUP_Tickables);
}
