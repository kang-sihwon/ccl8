#include "CCLVillagePopulationSubsystem.h"

#include "CCLCampaignState.h"
#include "CCLLifeVillager.h"
#include "CCLWorkshop.h"
#include "Agents/CCLAgentAIController.h"
#include "Agents/CCLAgentBiology.h"
#include "Agents/CCLAgentComponent.h"
#include "Agents/CCLAgentTags.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	const FPrimaryAssetId MerchantDefinition(TEXT("Agent"), TEXT("Merchant"));
}

// 부모 인터페이스 함수

bool UCCLVillagePopulationSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const auto* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UCCLVillagePopulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	auto* Agents = Collection.InitializeDependency<UCCLAgentWorldSubsystem>();

	if (Agents)
	{
		Agents->OnPopulationRetired.AddUObject(this, &ThisClass::RetirePopulation);
		Agents->OnPopulationReady.AddUObject(this, &ThisClass::BuildPopulation);
	}
}

void UCCLVillagePopulationSubsystem::Deinitialize()
{
	if (auto* Agents = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>())
	{
		Agents->OnPopulationRetired.RemoveAll(this);
		Agents->OnPopulationReady.RemoveAll(this);
	}

	Super::Deinitialize();
}

// 내 클래스 함수

void UCCLVillagePopulationSubsystem::RetirePopulation(UCCLAgentWorldSubsystem& Agents)
{
	if (!IsCampaignWorld())
	{
		return;
	}

	for (TActorIterator<ACCLLifeVillager> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}
}

void UCCLVillagePopulationSubsystem::BuildPopulation(UCCLAgentWorldSubsystem& Agents)
{
	// Experiment population stays in reduced execution until its Actor/Mass cases are implemented.
	if (!IsCampaignWorld())
	{
		return;
	}

	auto& Simulation = Agents.GetSimulation();
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
		if (Record.DefinitionId != MerchantDefinition)
		{
			continue;
		}

		const int32 MerchantIndex = Index++;
		if (!CCLAgentBiology::IsAlive(Record))
		{
			// The first merchant uses the map's steward body, which leaves when that merchant dies.
			if (MerchantIndex == 0)
			{
				for (TActorIterator<ACCLVillageSteward> It(GetWorld()); It; ++It)
				{
					if (!Cast<ACCLLifeVillager>(*It))
					{
						It->Destroy();
					}
				}
			}

			continue;
		}

		if (MerchantIndex == 0)
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

bool UCCLVillagePopulationSubsystem::IsCampaignWorld() const
{
	return GetWorld() && GetWorld()->GetNetMode() != NM_Client && GetWorld()->GetGameState<ACCLCampaignState>() != nullptr;
}
