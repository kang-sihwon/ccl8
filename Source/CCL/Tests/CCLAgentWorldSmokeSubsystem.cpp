#include "CCLAgentWorldSmokeSubsystem.h"

#include "Agents/CCLAgentWorldSubsystem.h"
#include "Agents/CCLAgentComponent.h"
#include "Campaign/CCLLifeVillager.h"
#include "Agents/CCLAgentTags.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Combat/CCLHitRule.h"
#include "Combat/CCLCombatDefinition.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

bool UCCLAgentWorldSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	const auto* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && FParse::Param(FCommandLine::Get(), TEXT("CCLAgentSmoke"));
#endif
}

TStatId UCCLAgentWorldSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLAgentWorldSmokeSubsystem, STATGROUP_Tickables);
}

void UCCLAgentWorldSmokeSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bComplete || !GetWorld()->HasBegunPlay() || GetWorld()->GetNetMode() == NM_Client)
	{
		return;
	}

	Elapsed += DeltaTime;
	if (Elapsed < 85)
	{
		return;
	}

	bComplete = 1;
	bool bPassed = true;
	auto Check = [&](bool bCondition, const TCHAR* Label)
	{
		bPassed &= bCondition;
		UE_LOG(LogTemp, Display, TEXT("CCL_AGENT_WORLD %s %s"), bCondition ? TEXT("CHECK") : TEXT("FAIL"), Label);
	};
	auto* World = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	Check(World && World->IsRunning(), TEXT("authoritative village simulation is running"));
	if (World && World->IsRunning())
	{
		int32 Actors = 0;
		for (TActorIterator<ACCLVillageSteward> It(GetWorld()); It; ++It)
		{
			++Actors;
		}

		Check(Actors == 8, TEXT("seven life Actors and the quest steward form the village"));
		FGuid InjuredId;
		float InjuredHealth = 0;
		for (TActorIterator<ACCLLifeVillager> It(GetWorld()); It; ++It)
		{
			APawn* Source = GetWorld()->GetFirstPlayerController()->GetPawn();
			auto* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Source);
			auto* Definition = LoadObject<UCCLCombatDefinition>(nullptr, TEXT("/Game/Progression/DA_PistolHit.DA_PistolHit"));
			FCCLHitContext Hit{Source, *It, SourceASC, It->GetAbilitySystemComponent(), Definition};
			Check(CCLHit::Apply(Hit) == CCLTags::Outcome_Damage, TEXT("common damage path hits a life Agent"));
			InjuredId = It->GetAgent()->AgentId;
			InjuredHealth = It->GetAbilitySystemComponent()->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute());
			const auto* Record = World->GetSimulation().Find(InjuredId);
			Check(Record && Record->Features[CCLAgentTags::Feature_Needs].Data.Get<FCCLAgentNeeds>().Urgency.FindRef(CCLAgentTags::Injury) > 0,
				TEXT("real damage updates persistent injury"));
			Check(Record && Record->Features[CCLAgentTags::Feature_Experience].Data.Get<FCCLAgentExperience>().Memories.ContainsByPredicate(
				[](const FCCLMemory& Memory) { return Memory.Observation.EventType == CCLAgentTags::Failure; }),
				TEXT("real damage leaves an observed memory"));
			break;
		}
		FCCLSimulationSnapshot Before;
		World->GetSimulation().Capture(Before);
		int32 Villagers = 0;
		for (const auto& Record : Before.Agents)
		{
			Villagers += Record.DefinitionId == FPrimaryAssetId(TEXT("Agent"), TEXT("Merchant")) ? 1 : 0;
		}
		Check(Villagers == 8, TEXT("eight persistent life records"));
		Check(Before.Economy.Journal.Num() > 0, TEXT("StateTree movement and work publish real execution results"));
		TArray<uint8> Saved;
		Check(World->Save(Saved), TEXT("collect Actor channels and save"));
		FString Error;
		Check(World->Restore(Saved, Error), TEXT("restore rebuilds Actors and transient execution"));
		FCCLSimulationSnapshot After;
		World->GetSimulation().Capture(After);
		Actors = 0;
		bool bRestoredInjury = false;
		for (TActorIterator<ACCLVillageSteward> It(GetWorld()); It; ++It)
		{
			++Actors;
			if (const auto* Villager = Cast<ACCLLifeVillager>(*It); Villager && Villager->GetAgent()->AgentId == InjuredId)
			{
				bRestoredInjury = FMath::IsNearlyEqual(Villager->GetAbilitySystemComponent()->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()), InjuredHealth);
			}
		}
		Check(Actors == 8, TEXT("combatant records never become duplicate village Actors on restore"));
		Check(bRestoredInjury, TEXT("restored Actor health matches persistent injury"));
		Check(After.Time == Before.Time && After.Economy.Journal.Num() == Before.Economy.Journal.Num(), TEXT("time and committed results survive Actor replacement"));
		for (TActorIterator<ACCLLifeVillager> It(GetWorld()); It; ++It)
		{
			const FGuid Id = It->GetAgent()->AgentId;
			It->Destroy();
			const double OldTime = World->GetSimulation().Find(Id)->LastSimulatedTime;
			World->GetSimulation().AdvanceTo(World->GetSimulation().GetTime() + 3600);
			Check(World->GetSimulation().Find(Id) && World->GetSimulation().Find(Id)->LastSimulatedTime > OldTime,
				TEXT("unloaded Actor retains its record and advances through reduced execution"));
			break;
		}

		FFileHelper::SaveStringToFile(World->GetSimulation().DailyReport(), *(FPaths::ProjectSavedDir() / TEXT("Tests/agent-world-state.txt")));
	}

	UE_LOG(LogTemp, Display, TEXT("CCL_AGENT_WORLD %s"), bPassed ? TEXT("PASS") : TEXT("FAIL"));
	if (auto* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->ConsoleCommand(TEXT("quit"));
	}
}
