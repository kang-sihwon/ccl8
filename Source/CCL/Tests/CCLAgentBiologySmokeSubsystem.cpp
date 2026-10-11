#include "CCLAgentBiologySmokeSubsystem.h"

#include "Agents/CCLAgentWorldSubsystem.h"
#include "Agents/CCLAgentComponent.h"
#include "Agents/CCLAgentTags.h"
#include "Campaign/CCLLifeVillager.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLEffects.h"
#include "Combat/CCLHitRule.h"
#include "Combat/CCLCombatDefinition.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

namespace
{
FGuid BiologySmokeId(uint32 N)
{
	return FGuid(950, 951, 952, N);
}
} // namespace

bool UCCLAgentBiologySmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLAgentBiologySmoke"));
#endif
}

void UCCLAgentBiologySmokeSubsystem::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	if (!Started)
	{
		Started = Now;
	}

	if (Now - Started > 120.)
	{
		Finish(false, FString::Printf(TEXT("timeout step=%d"), Step));
		return;
	}

	auto* World = GetWorld();
	auto* Agents = World ? World->GetSubsystem<UCCLAgentWorldSubsystem>() : nullptr;
	auto* Runtime = World ? World->GetSubsystem<UCCLWorldSimulationSubsystem>() : nullptr;
	auto* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!Agents || !Agents->IsRunning() || !Runtime || !Runtime->IsRunning() || !PC || !PC->GetPawn() || Now < Next)
	{
		return;
	}

	FString Error;
	auto Check = [&](bool bOK, const TCHAR* Message)
	{
		if (!bOK)
		{
			Finish(false, FString::Printf(TEXT("step=%d %s: %s"), Step, Message, *Error));
		}

		return bOK;
	};
	auto& Life = Agents->GetSimulation();
	auto Spawn = [&](uint32 N)
	{
		const FVector Anchor = PC->GetPawn()->GetActorLocation() + FVector(300., N * 140., 0.);
		FHitResult Ground;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(AgentBiologyTestGround), false, PC->GetPawn());
		if (!World->LineTraceSingleByChannel(Ground, Anchor + FVector(0., 0., 500.), Anchor - FVector(0., 0., 2000.),
											 ECC_Visibility, Query) ||
			Ground.ImpactNormal.Z < .7)
		{
			Error = TEXT("No walkable ground beneath the biology test spawn.");
			return static_cast<ACCLLifeVillager*>(nullptr);
		}

		const FTransform Transform(Ground.ImpactPoint + FVector(0., 0., 100.));
		auto* Actor =
			World->SpawnActorDeferred<ACCLLifeVillager>(ACCLLifeVillager::StaticClass(), Transform, nullptr, nullptr,
														ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Actor)
		{
			Actor->GetAgent()->AgentId = BiologySmokeId(N);
			Actor->FinishSpawning(Transform);
		}

		return Actor;
	};
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
		InitialTime = Life.GetTime();
		for (uint32 N = 1; N <= 2; ++N)
		{
			FKSIAgentRecord Record;
			Record.Id = BiologySmokeId(N);
			Record.DefinitionId = FPrimaryAssetId(TEXT("Agent"), TEXT("BiologySmoke"));
			Record.LastSimulatedTime = InitialTime;
			FCCLAgentBiology B;
			B.BirthWorldSeconds = InitialTime - 20.;
			B.MaturitySeconds = 10.;
			B.LifespanSeconds = 100.;
			B.bProtectNaturalAging = N == 2;
			Record.Features.Add(CCLAgentTags::Feature_Biology, {1, FInstancedStruct::Make(B)});
			Record.Features.Add(CCLAgentTags::Feature_Needs, {1, FInstancedStruct::Make(FCCLAgentNeeds())});
			if (!Check(Life.GetAgents().Add(Record, Life.GetRegistry(), Error).Id.IsValid(),
					   TEXT("add biological Agent")))
			{
				return;
			}
		}

		Ordinary = Spawn(1);
		Protected = Spawn(2);
		if (!Check(Ordinary.IsValid() && Protected.IsValid() && Agents->Save(SavedBefore),
				   TEXT("spawn and save living Actors")))
		{
			return;
		}

		auto* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PC->GetPawn());
		auto* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Protected.Get());
		auto* HitDefinition =
			LoadObject<UCCLCombatDefinition>(nullptr, TEXT("/Game/Progression/DA_PistolHit.DA_PistolHit"));
		FKSIHitContext Hit{PC->GetPawn(), Protected.Get(), SourceASC, TargetASC, HitDefinition};
		if (!Check(SourceASC && TargetASC && HitDefinition && KSIHit::Apply(Hit) == CCLTags::Outcome_Damage &&
					   TargetASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) < 100.,
				   TEXT("aging protection does not prevent actual damage")) ||
			!Check(Runtime->ChangeTimeScale(60., Error), TEXT("accelerate world time")))
		{
			return;
		}

		Step = 2;
		return;
	}

	if (Step == 2 && Life.GetTime() >= InitialTime + 100. && !Ordinary.IsValid())
	{
		Runtime->SetTickableTickType(ETickableTickType::Never);
		const auto* B = CCLAgentBiology::Find(*Life.Find(BiologySmokeId(1)));
		if (!Check(!Life.IsAlive(BiologySmokeId(1)) && Protected.IsValid() && Life.IsAlive(BiologySmokeId(2)) &&
					   FMath::IsNearlyEqual(B->DeathWorldSeconds, InitialTime + 80., 1.e-8),
				   TEXT("exact natural death and protected survival")))
		{
			return;
		}
		auto* ASC = Cast<UCCLAbilitySystemComponent>(
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Protected.Get()));
		auto& Store = Life.GetAgents();
		const auto Lease = Store.Acquire(Store.GetHandle(BiologySmokeId(2)), FGuid::NewGuid());
		if (!Check(ASC && Lease.Writer.IsValid(), TEXT("reserve biological writer")))
		{
			return;
		}
		const double DeathTime = Life.GetTime();
		ASC->ApplyEffect(UCCLHealthChangeEffect::StaticClass(), -10000.f);
		TArray<uint8> Rejected;
		const bool bBlockedSave = !Runtime->Save(Life, Rejected, Error) && Error.Contains(TEXT("awaits"));
		const bool bBlockedAdvance = !Runtime->AdvancePending(.25, 1., Error) && Error.Contains(TEXT("awaits"));
		const bool bFatalHealth = ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) == 0.f;
		const bool bDestroyed = Protected->Destroy();
		Store.Release(Lease);
		if (!Check(bBlockedSave && bBlockedAdvance && bFatalHealth && bDestroyed &&
					   Life.IsAlive(BiologySmokeId(2)) && Life.GetTime() == DeathTime &&
					   Runtime->Save(Life, SavedAfter, Error) && !Life.IsAlive(BiologySmokeId(2)),
				   TEXT("fatal GAS death survives Actor removal and commits before save")))
		{
			return;
		}
		const auto* Death = CCLAgentBiology::Find(*Life.Find(BiologySmokeId(2)));
		if (!Check(Death && Death->DeathCause == ECCLAgentDeathCause::External && Death->DeathWorldSeconds == DeathTime,
				   TEXT("external death keeps impact time despite natural protection")))
		{
			return;
		}

		Step = 3;
		return;
	}

	if (Step == 3 && !Protected.IsValid())
	{
		if (!Check(Agents->Save(SavedAfter) && Agents->Restore(SavedBefore, Error) && Life.IsAlive(BiologySmokeId(1)) &&
					   Life.IsAlive(BiologySmokeId(2)) && Life.GetTime() == InitialTime,
				   TEXT("restore earlier biology and common time")))
		{
			return;
		}

		Ordinary = Spawn(1);
		Protected = Spawn(2);
		auto* ASC = Cast<UCCLAbilitySystemComponent>(
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Protected.Get()));
		auto& Store = Life.GetAgents();
		const auto Lease = Store.Acquire(Store.GetHandle(BiologySmokeId(2)), FGuid::NewGuid());
		if (!Check(ASC && Lease.Writer.IsValid(), TEXT("reserve stale death writer")))
		{
			return;
		}
		ASC->ApplyEffect(UCCLHealthChangeEffect::StaticClass(), -10000.f);
		Store.Release(Lease);
		if (!Check(Agents->Restore(SavedBefore, Error) && Agents->FlushExternalDeaths(Error) &&
					   Life.IsAlive(BiologySmokeId(2)),
				   TEXT("restore discards old epoch pending death")))
		{
			return;
		}
		Ordinary = Spawn(1);
		Protected = Spawn(2);
		if (!Check(Ordinary.IsValid() && Protected.IsValid() && Agents->Restore(SavedAfter, Error) &&
					   !Ordinary.IsValid() && !Protected.IsValid() && !Life.IsAlive(BiologySmokeId(1)) &&
					   !Life.IsAlive(BiologySmokeId(2)),
				   TEXT("restore terminal records and discard prior Actors")))
		{
			return;
		}

		Ordinary = Spawn(1);
		Protected = Spawn(2);
		Step = 4;
		return;
	}

	if (Step == 4 && !Ordinary.IsValid() && !Protected.IsValid())
	{
		const auto* Natural = CCLAgentBiology::Find(*Life.Find(BiologySmokeId(1)));
		const auto* External = CCLAgentBiology::Find(*Life.Find(BiologySmokeId(2)));
		Finish(Natural->DeathCause == ECCLAgentDeathCause::NaturalAge &&
				   External->DeathCause == ECCLAgentDeathCause::External,
			   TEXT("world age, exact death, protected NPC damage, external death, Actor cleanup and save restore"));
	}
}

void UCCLAgentBiologySmokeSubsystem::Finish(bool bSuccess, const FString& Message)
{
	bComplete = 1;
	UE_LOG(LogTemp, Display, TEXT("CCL_AGENT_BIOLOGY %s %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Message);
	FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1, TEXT("CCLAgentBiologySmoke"));
}

TStatId UCCLAgentBiologySmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLAgentBiologySmokeSubsystem, STATGROUP_Tickables);
}
