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

	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		if (const auto* Agent = It->FindComponentByClass<UCCLAgentComponent>())
		{
			Simulation.UpdateLocation(Agent->AgentId, It->GetActorLocation());
		}
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
