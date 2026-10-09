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
#include "Environment/CCLExperimentDefinition.h"
#include "Environment/CCLTerrainRegion.h"

void UCCLAgentSessionStore::ResetSession()
{
	Snapshot.Reset();
	TravelSnapshots.Remove(ECCLWorldDomain::Campaign);
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
	TravelSnapshots.Remove(Domain);
	ExperimentInitialSnapshots.Remove(Domain);
	++ExperimentSessions.FindOrAdd(Domain);
}

uint64 UCCLAgentSessionStore::SessionFor(ECCLWorldDomain Domain) const
{
	return Domain == ECCLWorldDomain::Campaign ? Session : ExperimentSessions.FindRef(Domain);
}

TArray<uint8>& UCCLAgentSessionStore::SnapshotFor(ECCLWorldDomain Domain)
{
	if (auto* Bundle = TravelSnapshots.Find(Domain))
	{
		return Bundle->World;
	}

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
	if (World.GetNetMode() == NM_Client || (!World.GetGameState<ACCLCampaignState>() &&
		UCCLWorldSimulationSubsystem::DomainForWorld(&World) == ECCLWorldDomain::Campaign))
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
	const auto* Experiment = Domain != ECCLWorldDomain::Campaign ? LoadObject<UCCLExperimentDefinition>(nullptr,
		TEXT("/Game/Environment/Experiments/DA_Environment_00.DA_Environment_00")) : nullptr;
	const auto Initial = Domain == ECCLWorldDomain::Campaign && Scenario ? Scenario->InitialState :
		FCCLLifeSimulation::MerchantScenario(Experiment ? Experiment->Seed : 42);
	const bool bReady = WorldSimulation && (!Saved.IsEmpty() || Simulation.Initialize(Initial, Error)) &&
		WorldSimulation->Start(Simulation, Domain, Saved, Error);
	if (!bReady)
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_AGENT world initialization failed: %s"), *Error);
		return;
	}

	bRunning = 1;
	SpawnVillage();
	FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &UCCLAgentWorldSubsystem::OnWorldBeginTearDown);
}

void UCCLAgentWorldSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldBeginTearDown.RemoveAll(this);
	if (bRunning && GetWorld()->GetGameInstance())
	{
		auto* Store = GetWorld()->GetGameInstance()->GetSubsystem<UCCLAgentSessionStore>();
		const ECCLWorldDomain Domain = UCCLWorldSimulationSubsystem::DomainForWorld(GetWorld());
		if (Store && Store->SessionFor(Domain) == Session && !bTravelPrepared && !Store->TravelSnapshots.Contains(Domain))
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

bool UCCLAgentWorldSubsystem::SaveSessionForTravel(FString& Error)
{
	Error.Reset();
	auto* SessionStore = GetWorld()->GetGameInstance()->GetSubsystem<UCCLAgentSessionStore>();
	const auto Domain = UCCLWorldSimulationSubsystem::DomainForWorld(GetWorld());
	if (!bRunning || GetWorld()->GetNetMode() == NM_Client || !SessionStore || SessionStore->SessionFor(Domain) != Session)
	{
		Error = TEXT("현재 서버 세션을 저장할 수 없다.");
		return false;
	}

	TArray<ACCLTerrainRegion*> Regions;
	for (TActorIterator<ACCLTerrainRegion> It(GetWorld()); It; ++It)
	{
		if (!It->IsTerrainReady() || It->IsPreparing())
		{
			Error = TEXT("지형 작업이 끝난 뒤 맵을 전환할 수 있다.");
			return false;
		}

		Regions.Add(*It);
	}

	FCCLWorldGenerationBundle Bundle;
	FCCLWorldSnapshot WorldState;
	if (!Save(Bundle.World) || !FCCLWorldSnapshotCodec::Decode(Bundle.World, WorldState, Error))
	{
		return false;
	}

	Bundle.Context = FCCLWorldGenerationStore::ContextFor(WorldState);
	for (const auto* Region : Regions)
	{
		const auto& Terrain = Region->GetTerrainStore();
		const FGuid RegionId = Terrain.GetSnapshot().Definition.RegionId;
		if (Bundle.Terrain.Contains(RegionId) || !Terrain.Capture(Bundle.Context, Bundle.Terrain.FindOrAdd(RegionId), Error))
		{
			return false;
		}
	}

	// Publish the complete in-memory checkpoint once; teardown must not replace only its world bytes.
	SessionStore->TravelSnapshots.Add(Domain, MoveTemp(Bundle));
	bTravelPrepared = 1;
	return true;
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
	// Experiment population stays in reduced execution until its Actor/Mass cases are implemented.
	if (UCCLWorldSimulationSubsystem::DomainForWorld(GetWorld()) != ECCLWorldDomain::Campaign)
	{
		return;
	}

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
			Villager->PublicName = FString::Printf(TEXT("주민 %d"), Index);
			Villager->FinishSpawning(Transform);
		}
	}
}

void UCCLAgentWorldSubsystem::OnWorldBeginTearDown(UWorld* World)
{
	if (World != GetWorld() || !bRunning || bTravelPrepared || World->GetNetMode() == NM_Client)
	{
		return;
	}

	// External travel has already started. Discard unfinished candidates and retain committed terrain.
	for (TActorIterator<ACCLTerrainRegion> It(World); It; ++It)
	{
		It->CancelPendingEdit();
	}

	FString Error;
	if (!SaveSessionForTravel(Error))
	{
		UE_LOG(LogTemp, Warning, TEXT("CCL_AGENT travel checkpoint retained previous state: %s"), *Error);
	}
}
