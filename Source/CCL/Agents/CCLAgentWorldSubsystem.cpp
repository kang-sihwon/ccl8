#include "CCLAgentWorldSubsystem.h"

#include "CCLAgentComponent.h"
#include "CCLAgentTags.h"
#include "Campaign/CCLLifeVillager.h"
#include "Campaign/CCLCampaignState.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"

void UCCLAgentSessionStore::ResetSession()
{
	Snapshot.Reset();
	++Session;
}

bool UCCLAgentWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const auto* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UCCLAgentWorldSubsystem::OnWorldBeginPlay(UWorld& World)
{
	Super::OnWorldBeginPlay(World);
	if (World.GetNetMode() == NM_Client || !World.GetGameState<ACCLCampaignState>())
	{
		return;
	}

	auto* Store = World.GetGameInstance()->GetSubsystem<UCCLAgentSessionStore>();
	Session = Store->Session;
	FString Error;
	const auto* Scenario = LoadObject<UCCLPopulationScenario>(nullptr,
        TEXT("/Game/Progression/DA_MerchantLifeScenario.DA_MerchantLifeScenario"));
    const bool bReady = Store->Snapshot.IsEmpty() ? Simulation.Initialize(
        Scenario ? Scenario->InitialState : FCCLLifeSimulation::MerchantScenario(42), Error) : Simulation.Load(Store->Snapshot, Error);
	if (!bReady)
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_AGENT world initialization failed: %s"), *Error);
		return;
	}

	bRunning = 1;
	SpawnVillage();
}

void UCCLAgentWorldSubsystem::Deinitialize()
{
	if (bRunning && GetWorld()->GetGameInstance())
	{
		auto* Store = GetWorld()->GetGameInstance()->GetSubsystem<UCCLAgentSessionStore>();
		if (Store && Store->Session == Session)
		{
			Save(Store->Snapshot);
		}
	}

	bRunning = 0;
	Super::Deinitialize();
}

void UCCLAgentWorldSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bRunning)
	{
        PendingSeconds += DeltaTime;
        if (PendingSeconds >= 0.25)
        {
            Simulation.AdvanceTo(Simulation.GetTime() + PendingSeconds * 60.);
            PendingSeconds = 0;
        }
	}
}

TStatId UCCLAgentWorldSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLAgentWorldSubsystem, STATGROUP_Tickables);
}

bool UCCLAgentWorldSubsystem::Save(TArray<uint8>& Bytes)
{
	if (!bRunning || GetWorld()->GetNetMode() == NM_Client)
	{
		return false;
	}

	for (TActorIterator<ACCLLifeVillager> It(GetWorld()); It; ++It)
	{
		Simulation.UpdateLocation(It->GetAgent()->AgentId, It->GetActorLocation());
	}

	return Simulation.Save(Bytes);
}

bool UCCLAgentWorldSubsystem::Restore(const TArray<uint8>& Bytes, FString& Error)
{
	if (!bRunning || GetWorld()->GetNetMode() == NM_Client || !Simulation.Load(Bytes, Error))
	{
		return false;
	}

	bRunning = 0;
	for (TActorIterator<ACCLLifeVillager> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}

	bRunning = 1;
	SpawnVillage();
	return true;
}

void UCCLAgentWorldSubsystem::SpawnVillage()
{
	FCCLSimulationSnapshot Snapshot;
	if (!Simulation.Capture(Snapshot))
	{
		return;
	}

	int32 Index = 0;
	for (const auto& Record : Snapshot.Agents)
	{
		if (Index++ == 0)
		{
			for (TActorIterator<ACCLVillageSteward> It(GetWorld()); It; ++It)
			{
				if (!Cast<ACCLLifeVillager>(*It))
				{
					Simulation.SetActorActive(Record.Id, true);
					Simulation.UpdateLocation(Record.Id, It->GetActorLocation());
					break;
				}
			}

			continue;
		}

		const FTransform Transform(FRotator::ZeroRotator, Record.Location.Position);
		auto* Villager = GetWorld()->SpawnActorDeferred<ACCLLifeVillager>(ACCLLifeVillager::StaticClass(), Transform,
			nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (Villager)
		{
			Villager->GetAgent()->AgentId = Record.Id;
			Villager->PublicName = FString::Printf(TEXT("Villager %d"), Index);
			Villager->FinishSpawning(Transform);
		}
	}
}
