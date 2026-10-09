#include "CCLAgentWorldSubsystem.h"

#include "CCLAgentComponent.h"
#include "CCLAgentAIController.h"
#include "Campaign/CCLWorkshop.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CCLAgentTags.h"
#include "Campaign/CCLLifeVillager.h"
#include "Campaign/CCLCampaignState.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Environment/CCLWorldSimulationSubsystem.h"

void UCCLAgentSessionStore::ResetSession()
{
	Snapshot.Reset();
	++Session;
}

void UCCLAgentSessionStore::ResetDomain(ECCLWorldDomain Domain)
{
	if (Domain == ECCLWorldDomain::Campaign)
	{
		ResetSession();
		return;
	}

	Experiments.Remove(Domain);
	++ExperimentSessions.FindOrAdd(Domain);
}

uint64 UCCLAgentSessionStore::SessionFor(ECCLWorldDomain Domain) const
{
	return Domain == ECCLWorldDomain::Campaign ? Session : ExperimentSessions.FindRef(Domain);
}

TArray<uint8>& UCCLAgentSessionStore::SnapshotFor(ECCLWorldDomain Domain)
{
	return Domain == ECCLWorldDomain::Campaign ? Snapshot : Experiments.FindOrAdd(Domain);
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
	FString Error;
	const auto* Scenario = LoadObject<UCCLPopulationScenario>(nullptr,
		TEXT("/Game/Progression/DA_MerchantLifeScenario.DA_MerchantLifeScenario"));
	auto* WorldSimulation = World.GetSubsystem<UCCLWorldSimulationSubsystem>();
	const ECCLWorldDomain Domain = UCCLWorldSimulationSubsystem::DomainForWorld(&World);
	Session = Store->SessionFor(Domain);
	const auto& Saved = Store->SnapshotFor(Domain);
	const bool bReady = WorldSimulation && (Saved.IsEmpty() ? Simulation.Initialize(
		Scenario ? Scenario->InitialState : FCCLLifeSimulation::MerchantScenario(42), Error) : true) &&
		WorldSimulation->Start(Simulation, Domain, Saved, Error);
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
		const ECCLWorldDomain Domain = UCCLWorldSimulationSubsystem::DomainForWorld(GetWorld());
		if (Store && Store->SessionFor(Domain) == Session)
		{
			Save(Store->SnapshotFor(Domain));
		}
	}

	bRunning = 0;
	Super::Deinitialize();
}

bool UCCLAgentWorldSubsystem::Save(TArray<uint8>& Bytes)
{
	if (!bRunning || GetWorld()->GetNetMode() == NM_Client)
	{
		return false;
	}

	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		if (const auto* Agent = It->FindComponentByClass<UCCLAgentComponent>())
		{
			Simulation.UpdateLocation(Agent->AgentId, It->GetActorLocation());
		}
	}

	FString Error;
	auto* WorldSimulation = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	return WorldSimulation && WorldSimulation->Save(Simulation, Bytes, Error);
}

bool UCCLAgentWorldSubsystem::Restore(const TArray<uint8>& Bytes, FString& Error)
{
	auto* WorldSimulation = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	if (!bRunning || GetWorld()->GetNetMode() == NM_Client || !WorldSimulation ||
		!WorldSimulation->Restore(Simulation, Bytes, Error))
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
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		if (auto* Agent = It->FindComponentByClass<UCCLAgentComponent>())
		{
			Agent->RestoreHealthFromRecord();
		}
	}
	return true;
}

void UCCLAgentWorldSubsystem::SpawnVillage()
{
	FCCLSimulationSnapshot Snapshot;
	if (!Simulation.Capture(Snapshot))
	{
		return;
	}

	for (TActorIterator<ACCLWorkshop> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}
	for (const auto& Pair : Snapshot.Economy.Ownerships)
	{
		const auto* Owner = Simulation.Find(Pair.Value.OwnerId);
		const auto* Life = Owner ? Owner->Features.Find(CCLAgentTags::Feature_Life) : nullptr;
		if (Life)
		{
			const FVector Position = Life->Data.Get<FCCLLifeState>().Workplace.Position + FVector(0, 100, -65);
			auto* Workshop = GetWorld()->SpawnActor<ACCLWorkshop>(Position, FRotator::ZeroRotator);
			if (Workshop)
			{
				Workshop->OwnershipId = Pair.Key;
			}
		}
	}
	int32 Index = 0;
	for (const auto& Record : Snapshot.Agents)
	{
		if (Record.DefinitionId != FPrimaryAssetId(TEXT("Agent"), TEXT("Merchant")))
		{
			continue;
		}
		if (Index++ == 0)
		{
			for (TActorIterator<ACCLVillageSteward> It(GetWorld()); It; ++It)
			{
				if (!Cast<ACCLLifeVillager>(*It))
				{
					auto* Component = It->FindComponentByClass<UCCLAgentComponent>();
					if (!Component)
					{
						Component = NewObject<UCCLAgentComponent>(*It);
						It->AddInstanceComponent(Component);
						Component->AgentId = Record.Id;
						Component->RegisterComponent();
					}
					if (auto* Controller = It->GetController())
					{
						Controller->UnPossess();
						Controller->Destroy();
					}
					It->SetActorLocation(Record.Location.Position);
					It->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
					It->GetCharacterMovement()->MaxWalkSpeed = 150;
					It->AIControllerClass = ACCLAgentAIController::StaticClass();
					It->SpawnDefaultController();
					Simulation.SetActorActive(Record.Id, true);
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
