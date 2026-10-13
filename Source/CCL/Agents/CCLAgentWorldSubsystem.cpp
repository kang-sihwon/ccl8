#include "CCLAgentWorldSubsystem.h"

#include "CCLAgentComponent.h"
#include "CCLAgentEmbodiment.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Environment/CCLTerrainRegion.h"
#include "Environment/CCLWorldEnvironmentConfig.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameModeBase.h"

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
	if (World.GetNetMode() == NM_Client)
	{
		return;
	}

	// The GameState or one map Actor opts the world into the life simulation and owns its initial population.
	const ICCLAgentWorldHost* Host = Cast<ICCLAgentWorldHost>(World.GetGameState());
	for (TActorIterator<AActor> It(&World); !Host && It; ++It)
	{
		Host = Cast<ICCLAgentWorldHost>(*It);
	}

	if (!Host)
	{
		return;
	}

	auto* Store = World.GetGameInstance()->GetSubsystem<UCCLAgentSessionStore>();
	auto* WorldSimulation = World.GetSubsystem<UCCLWorldSimulationSubsystem>();
	Domain = UCCLWorldSimulationSubsystem::DomainForWorld(&World);
	Session = Store->SessionFor(Domain);
	const bool bLaboratory = ACCLWorldEnvironmentConfig::IsLaboratory(&World);
	if (bLaboratory)
	{
		auto* Config = const_cast<ACCLWorldEnvironmentConfig*>(ACCLWorldEnvironmentConfig::Find(&World));
		const auto* Mode = World.GetAuthGameMode();
		if (Mode)
		{
			const FString Variant = UGameplayStatics::ParseOption(Mode->OptionsString, TEXT("LabVariant"));
			const FString Count = UGameplayStatics::ParseOption(Mode->OptionsString, TEXT("LabCount"));
			if (!Variant.IsEmpty()) { Config->LaboratoryVariant = FMath::Clamp(FCString::Atoi(*Variant), 0, ACCLWorldEnvironmentConfig::LaboratoryVariants(Config->LaboratoryZone, Config->LaboratorySubject).Num() - 1); }
			if (!Count.IsEmpty()) { Config->LaboratoryCount = FMath::Clamp(FCString::Atoi(*Count), 1, 16); }
		}
	}
	const TArray<uint8> Empty;
	const auto& Saved = bLaboratory ? Empty : Store->SnapshotFor(Domain);
	FString Error;
	FCCLSimulationSnapshot Initial;
	const bool bReady = WorldSimulation &&
		(!Saved.IsEmpty() || (Host->BuildInitialPopulation(Initial, Error) && Simulation.Initialize(Initial, Error))) &&
		WorldSimulation->Start(Simulation, Domain, Saved, Error);
	if (!bReady)
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_AGENT world initialization failed: %s"), *Error);
		return;
	}

	bRunning = 1;
	OnPopulationReady.Broadcast(*this);
	FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &UCCLAgentWorldSubsystem::OnWorldBeginTearDown);
}

void UCCLAgentWorldSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldBeginTearDown.RemoveAll(this);
	if (bRunning && GetWorld()->GetGameInstance() && !ACCLWorldEnvironmentConfig::IsLaboratory(GetWorld()))
	{
		auto* Store = GetWorld()->GetGameInstance()->GetSubsystem<UCCLAgentSessionStore>();
		if (Store && Store->SessionFor(Domain) == Session && !bTravelPrepared && !Store->TravelSnapshots.Contains(Domain))
		{
			// Replace the stored world only with a complete capture; a rejected capture keeps the previous state.
			TArray<uint8> Bytes;
			FString Error;
			if (Save(Bytes, Error))
			{
				Store->SnapshotFor(Domain) = MoveTemp(Bytes);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("CCL_AGENT unload save rejected; the previous world state is kept: %s"), *Error);
			}
		}
	}

	bRunning = 0;
	Super::Deinitialize();
}

bool UCCLAgentWorldSubsystem::Save(TArray<uint8>& Bytes, FString& Error, const UObject* SessionContainer)
{
	if (!bRunning || GetWorld()->GetNetMode() == NM_Client)
	{
		Error = TEXT("Only a running server world can be saved.");
		return false;
	}

	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		if (const auto* Agent = It->FindComponentByClass<UCCLAgentComponent>())
		{
			Simulation.UpdateLocation(Agent->AgentId, It->GetActorLocation());
		}
	}

	auto* WorldSimulation = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	return WorldSimulation && WorldSimulation->Save(Simulation, Bytes, Error, SessionContainer);
}

bool UCCLAgentWorldSubsystem::Save(TArray<uint8>& Bytes)
{
	FString Error;
	if (Save(Bytes, Error))
	{
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("CCL_AGENT world save rejected: %s"), *Error);
	return false;
}

bool UCCLAgentWorldSubsystem::SaveSessionForTravel(FString& Error)
{
	Error.Reset();
	auto* SessionStore = GetWorld()->GetGameInstance()->GetSubsystem<UCCLAgentSessionStore>();
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
	if (!Save(Bundle.World, Error) || !FCCLWorldSnapshotCodec::Decode(Bundle.World, WorldState, Error))
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

	// Representations leave without writing their old positions into the restored records.
	bRunning = 0;
	OnPopulationRetired.Broadcast(*this);
	bRunning = 1;
	OnPopulationReady.Broadcast(*this);
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		if (auto* Agent = It->FindComponentByClass<UCCLAgentComponent>())
		{
			Agent->RestoreHealthFromRecord();
		}
	}
	return true;
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


bool UCCLAgentWorldSubsystem::RequestExternalDeath(FGuid Id, FString& Error)
{
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	const auto* Record = Simulation.Find(Id);
	if (!bRunning || GetWorld()->GetNetMode() == NM_Client || !Runtime || !Runtime->IsRunning() ||
		!Record || !CCLAgentBiology::Find(*Record))
	{
		Error = TEXT("External death requires an authoritative biological Agent.");
		return false;
	}
	if (!CCLAgentBiology::IsAlive(*Record))
	{
		PendingExternalDeaths.Remove(Id);
		return true;
	}
	PendingExternalDeaths.Add(Id, Runtime->GetEpoch());
	if (!Simulation.EndLife(Id, ECCLAgentDeathCause::External, Error))
	{
		return false;
	}
	PendingExternalDeaths.Remove(Id);
	return true;
}

bool UCCLAgentWorldSubsystem::FlushExternalDeaths(FString& Error)
{
	if (PendingExternalDeaths.IsEmpty())
	{
		return true;
	}

	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	if (!bRunning || !Runtime || !Runtime->IsRunning() || GetWorld()->GetNetMode() == NM_Client)
	{
		Error = TEXT("World awaits the Agent death owner.");
		return false;
	}
	for (auto It = PendingExternalDeaths.CreateIterator(); It; ++It)
	{
		if (It.Value() != Runtime->GetEpoch())
		{
			It.RemoveCurrent();
			continue;
		}
		if (Simulation.IsAlive(It.Key()) && !Simulation.EndLife(It.Key(), ECCLAgentDeathCause::External, Error))
		{
			Error = TEXT("World awaits a pending Agent death: ") + Error;
			return false;
		}
		It.RemoveCurrent();
	}
	return true;
}
