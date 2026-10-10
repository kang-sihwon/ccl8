#include "CCLEcologySmokeSubsystem.h"

#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Crc.h"

namespace
{
	FGuid EcologySmokeId(uint32 N)
	{
		return FGuid(898, 899, 900, N);
	}
}

bool UCCLEcologySmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLEcologySmoke"));
#endif
}

void UCCLEcologySmokeSubsystem::Tick(float DeltaTime)
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
	auto* Director = World ? ACCLExperimentDirector::Find(World) : nullptr;
	if (!Runtime || !Runtime->IsRunning() || !Agents || !Agents->IsRunning() || !Director || !Director->IsReady() || Now < Next)
	{
		return;
	}

	FString Error, Directory;
	FParse::Value(FCommandLine::Get(), TEXT("CCLEcologySmokeDir="), Directory);
	const FString SavePath = Directory / TEXT("ecology-world.bin");
	auto Check = [&](bool bOK)
	{
		if (!bOK)
		{
			Finish(false, FString::Printf(TEXT("step=%d %s"), Step, *Error));
		}

		return bOK;
	};
	if (Step == 0)
	{
		if (!Check(Runtime->ChangeTimeScale(0., Error)))
		{
			return;
		}

		Step = 1;
		Next = Now + 2.;
		return;
	}

	if (Step == 1)
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("CCLEcologyResume")))
		{
			FCCLWorldSnapshot Decoded;
			if (!Check(FFileHelper::LoadFileToArray(Saved, *SavePath)
				&& FCCLWorldSnapshotCodec::Decode(Saved, Decoded, Error) && Agents->Restore(Saved, Error)
				&& Runtime->GetEcologySimulation().Capture(EcologyBefore, Error) && EcologyBefore == Decoded.Ecology))
			{
				return;
			}

			InitialTime = Runtime->GetClock().GetWorldSeconds();
			BornBefore = Runtime->GetEcologySimulation().GetPopulations()[EcologySmokeId(30)].Born;
			UE_LOG(LogTemp, Display, TEXT("CCL_ECOLOGY RESTART_RESTORED world=%.6f crc=%u"), InitialTime,
				FCrc::MemCrc32(EcologyBefore.GetData(), EcologyBefore.Num() - 4));
			if (!Check(Runtime->ChangeTimeScale(60., Error)))
			{
				return;
			}

			Step = 5;
			return;
		}

		InitialTime = Runtime->GetClock().GetWorldSeconds();
		for (uint32 I = 0; I < 4; ++I)
		{
			FCCLEcologySpecies Species;
			Species.Id = EcologySmokeId(10 + I);
			Species.Movement = ECCLEcologyMovement(I);
			Species.MaturitySeconds = 50.;
			Species.LifespanSeconds = 400.;
			Species.BirthIntervalSeconds = 100.;
			FCCLEcologyHabitat Habitat;
			Habitat.Id = EcologySmokeId(20 + I);
			Habitat.MovementMask = 1 << I;
			Habitat.Capacity = 100;
			FCCLEcologyPopulation Population;
			Population.Id = EcologySmokeId(30 + I);
			Population.SpeciesId = Species.Id;
			Population.HabitatId = Habitat.Id;
			Population.Cohorts.Add(EcologySmokeId(40 + I), {EcologySmokeId(40 + I), InitialTime - 100., 10});
			if (!Check(Runtime->AddEcologySpecies(Species, Error) && Runtime->AddEcologyHabitat(Habitat, Error)
				&& Runtime->AddEcologyPopulation(Population, Error)))
			{
				return;
			}
		}

		FCCLEcologyEvent Reservation;
		Reservation.Id = EcologySmokeId(60);
		Reservation.Kind = ECCLEcologyEventKind::Materialize;
		Reservation.Source = EcologySmokeId(30);
		Reservation.CohortId = EcologySmokeId(40);
		Reservation.IndividualId = EcologySmokeId(50);
		Reservation.BirthWorldSeconds = InitialTime - 100.;
		Reservation.bPersistent = 1;
		if (!Check(Runtime->ApplyEcologyEvent(Reservation, Error)
			&& Runtime->AcquireEcologyIndividual(Reservation.IndividualId, ECCLEcologyRepresentation::Actor, EcologySmokeId(70), Lease, Error)
			&& Runtime->ChangeTimeScale(60., Error)))
		{
			return;
		}

		Step = 2;
		return;
	}

	if (Step == 2 || Step == 5)
	{
		if (Runtime->GetClock().GetWorldSeconds() < InitialTime + (Step == 2 ? 200. : 100.))
		{
			return;
		}

		if (!Check(Runtime->ChangeTimeScale(0., Error)))
		{
			return;
		}

		Step = Step == 2 ? 3 : 6;
		Next = Now + 2.;
		return;
	}

	const auto& Ecology = Runtime->GetEcologySimulation();
	if (!Check(Ecology.Validate(Error) && Ecology.GetWorldId() == Runtime->GetIdentity().WorldId
		&& Ecology.GetWorldSeconds() == Runtime->GetClock().GetWorldSeconds()
		&& Ecology.GetWorldSeconds() == Agents->GetSimulation().GetTime()
		&& Ecology.GetWorldSeconds() == Runtime->GetSurfaceSimulation().GetWorldSeconds()))
	{
		return;
	}

	if (Step == 3)
	{
		for (uint32 I = 0; I < 4; ++I)
		{
			if (!Check(Ecology.GetPopulations()[EcologySmokeId(30 + I)].Born > 0 && Ecology.LivingCount(EcologySmokeId(30 + I)) > 10))
			{
				return;
			}
		}

		if (!Check(Ecology.Capture(EcologyBefore, Error) && Agents->Save(Saved)
			&& FFileHelper::SaveArrayToFile(Saved, *SavePath)))
		{
			return;
		}

		UE_LOG(LogTemp, Display, TEXT("CCL_ECOLOGY SAVED world=%.6f crc=%u"), Ecology.GetWorldSeconds(),
			FCrc::MemCrc32(EcologyBefore.GetData(), EcologyBefore.Num() - 4));
		FCCLEcologyEvent Capture;
		Capture.Id = EcologySmokeId(61);
		Capture.Kind = ECCLEcologyEventKind::Capture;
		Capture.Source = EcologySmokeId(30);
		Capture.CohortId = EcologySmokeId(40);
		Capture.BirthWorldSeconds = InitialTime - 100.;
		Capture.Count = 2;
		TArray<uint8> After;
		if (!Check(Runtime->ApplyEcologyEvent(Capture, Error) && Runtime->ReleaseEcologyIndividual(Lease, Error)
			&& Ecology.Capture(After, Error) && After != EcologyBefore && Agents->Restore(Saved, Error)
			&& Runtime->GetEcologySimulation().Capture(After, Error) && After == EcologyBefore
			&& !Runtime->ReleaseEcologyIndividual(Lease, Error)))
		{
			return;
		}

		Finish(true, TEXT("four media births, common time, full save/restore and old-writer rejection"));
		return;
	}

	if (Step == 6 && Check(Ecology.GetPopulations()[EcologySmokeId(30)].Born > BornBefore))
	{
		Finish(true, TEXT("new process restored exact ecology and resumed scheduled births"));
	}
}

void UCCLEcologySmokeSubsystem::Finish(bool bSuccess, const FString& Message)
{
	bComplete = 1;
	UE_LOG(LogTemp, Display, TEXT("CCL_ECOLOGY %s %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Message);
	FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1, TEXT("CCLEcologySmoke"));
}

TStatId UCCLEcologySmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLEcologySmokeSubsystem, STATGROUP_Tickables);
}
