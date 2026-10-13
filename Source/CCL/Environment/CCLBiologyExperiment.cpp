#include "CCLBiologyExperiment.h"

#include "CCLExperimentDefinition.h"
#include "CCLWorldEnvironmentConfig.h"
#include "CCLWorldSnapshot.h"
#include "CCLWorldSimulationSubsystem.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Agents/CCLAgentComponent.h"
#include "Agents/CCLAgentTags.h"
#include "Agents/CCLAgentEmbodiment.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystem/CCLEffects.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

namespace
{
FKSIAgentRecord MakeRecord(int32 Index, double Time, const ACCLWorldEnvironmentConfig* Config)
{
	FKSIAgentRecord R;
	R.Id = ACCLBiologyExperiment::AgentId(Index);
	R.DefinitionId = FPrimaryAssetId(TEXT("Agent"), TEXT("BiologyExperiment"));
	R.LastSimulatedTime = Time;
	R.Location.Position = UCCLExperimentDefinition::ZoneCenter(7) + (Config && Config->LaboratoryZone == 7 ? FVector((Index % 4 - 1.5) * 180., (Index / 4) * 180., 98.) : FVector(Index ? 260. : -260., 700., 90.));
	FCCLAgentBiology B;
	B.BirthWorldSeconds = Time - 20.;
	B.MaturitySeconds = 10.;
	B.LifespanSeconds = 300.;
	B.bProtectNaturalAging = Config && Config->LaboratoryZone == 7 ? uint8(Config->LaboratoryVariant != 0) : uint8(Index == 1);
	R.Features.Add(CCLAgentTags::Feature_Biology, {1, FInstancedStruct::Make(B)});
	R.Features.Add(CCLAgentTags::Feature_Needs, {1, FInstancedStruct::Make(FCCLAgentNeeds())});
	return R;
}
} // namespace

ACCLBiologyExperiment::ACCLBiologyExperiment()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = .2f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ACCLBiologyExperiment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACCLBiologyExperiment, View);
}

bool ACCLBiologyExperiment::Initialize(bool bSeed, FCCLWorldSnapshot& Baseline, FString& Error)
{
	auto* Agents = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	if (!HasAuthority() || !Agents || !Agents->IsRunning())
	{
		return false;
	}
	auto& Live = Agents->GetSimulation();
	FCCLLifeSimulation Initial;
	if (!Initial.Load(Baseline.Life, Error))
	{
		return false;
	}
	for (int32 Index = 0; Index < (ACCLWorldEnvironmentConfig::IsLaboratory(GetWorld()) ? ACCLWorldEnvironmentConfig::Find(GetWorld())->SampleCount() : 2); ++Index)
	{
		if (!Initial.Find(AgentId(Index)) &&
			!Initial.GetAgents().Add(MakeRecord(Index, Initial.GetTime(), ACCLWorldEnvironmentConfig::Find(GetWorld())), Initial.GetRegistry(), Error).Id.IsValid())
		{
			return false;
		}
		if (bSeed && !Live.Find(AgentId(Index)) &&
			!Live.GetAgents().Add(MakeRecord(Index, Live.GetTime(), ACCLWorldEnvironmentConfig::Find(GetWorld())), Live.GetRegistry(), Error).Id.IsValid())
		{
			return false;
		}
	}
	if (!Initial.Save(Baseline.Life))
	{
		return false;
	}
	bInitialized = 1;
	Reconcile();
	UpdateView();
	return true;
}

ACharacter* ACCLBiologyExperiment::FindActor(int32 Index) const
{
	for (TActorIterator<ACharacter> It(GetWorld()); It; ++It)
	{
		const auto* Embodiment = Cast<ICCLAgentEmbodiment>(*It);
		const auto* Agent = Embodiment ? Embodiment->GetAgentComponent() : nullptr;
		if (!It->IsActorBeingDestroyed() && Agent && Agent->AgentId == AgentId(Index))
		{
			return *It;
		}
	}
	return nullptr;
}

void ACCLBiologyExperiment::Reconcile()
{
	auto* Agents = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	FString Error;
	if (!Agents || !Agents->IsRunning() || !Runtime || !Runtime->IsRunning() || !Agents->FlushExternalDeaths(Error))
	{
		return;
	}
	const bool bNewEpoch = ActorEpoch != Runtime->GetEpoch();
	for (int32 Index = 0; Index < (ACCLWorldEnvironmentConfig::IsLaboratory(GetWorld()) ? ACCLWorldEnvironmentConfig::Find(GetWorld())->SampleCount() : 2); ++Index)
	{
		auto* Actor = FindActor(Index);
		const auto* Record = Agents->GetSimulation().Find(AgentId(Index));
		const bool bAlive = Record && CCLAgentBiology::IsAlive(*Record);
		if (Actor && (bNewEpoch || !bAlive))
		{
			Actor->Destroy();
			Actor = nullptr;
		}
		if (bAlive && !Actor && ResidentClass && ResidentClass->ImplementsInterface(UCCLAgentEmbodiment::StaticClass()))
		{
			const FTransform Transform(FRotator(0., -90., 0.), Record->Location.Position);
			Actor = GetWorld()->SpawnActorDeferred<ACharacter>(ResidentClass, Transform,
																	 nullptr, nullptr,
																	 ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!Actor)
			{
				continue;
			}
			Actor->AutoPossessAI = EAutoPossessAI::Disabled;
			Actor->bAlwaysRelevant = true;
			auto* Embodiment = Cast<ICCLAgentEmbodiment>(Actor);
			Embodiment->GetAgentComponent()->AgentId = AgentId(Index);
			Embodiment->SetAgentPublicName(ACCLWorldEnvironmentConfig::IsLaboratory(GetWorld()) ? FString::Printf(TEXT("SAMPLE %02d"), Index + 1) : (Index ? TEXT("AGING PROTECTED") : TEXT("NATURAL AGE")));
			Actor->FinishSpawning(Transform);
			Actor->GetCharacterMovement()->SetMovementMode(MOVE_None);
		}
	}
	ActorEpoch = Runtime->GetEpoch();
}

void ACCLBiologyExperiment::UpdateView()
{
	auto* Agents = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	if (!Agents || !Runtime || !Runtime->IsRunning())
	{
		return;
	}
	FCCLBiologyExperimentView Next;
	Next.Epoch = Runtime->GetEpoch();
	Next.WorldSeconds = Runtime->GetClock().GetWorldSeconds();
	for (int32 Index = 0; Index < (ACCLWorldEnvironmentConfig::IsLaboratory(GetWorld()) ? ACCLWorldEnvironmentConfig::Find(GetWorld())->SampleCount() : 2); ++Index)
	{
		auto& Row = Next.Rows.AddDefaulted_GetRef();
		const auto* Record = Agents->GetSimulation().Find(AgentId(Index));
		const auto* B = Record ? CCLAgentBiology::Find(*Record) : nullptr;
		if (!B)
		{
			continue;
		}
		Row.bRecorded = 1;
		Row.bProtected = B->bProtectNaturalAging;
		Row.DeathCause = uint8(B->DeathCause);
		Row.Age = CCLAgentBiology::AgeSeconds(*B, Next.WorldSeconds);
		Row.Lifespan = B->LifespanSeconds;
		if (auto* Actor = FindActor(Index))
		{
			const auto* ActorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
			Row.Health = ActorASC ? ActorASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) : 0.f;
		}
	}
	View = MoveTemp(Next);
}

bool ACCLBiologyExperiment::Execute(ECCLExperimentAction Action, FString& Error)
{
	auto* Agents = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	if (!HasAuthority() || !bInitialized || !Agents || !Agents->IsRunning() || !Agents->FlushExternalDeaths(Error))
	{
		return false;
	}
	auto& Life = Agents->GetSimulation();
	const int32 Index = ACCLWorldEnvironmentConfig::IsLaboratory(GetWorld()) ? 0 : (Action == ECCLExperimentAction::BiologyAge ? 0 : 1);
	const auto* Current = Life.Find(AgentId(Index));
	if (!Current || !CCLAgentBiology::IsAlive(*Current))
	{
		Error = TEXT("살아 있는 생애 표본이 없다. 전체 초기화로 다시 시작해 줘.");
		return false;
	}
	if (Action == ECCLExperimentAction::BiologyDamage)
	{
		auto* Actor = FindActor(Index);
		auto* ASC = Actor ? Cast<UCCLAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor)) : nullptr;
		if (!ASC)
		{
			Error = TEXT("주민 표현 준비를 기다려 줘.");
			return false;
		}
		ASC->ApplyEffect(UCCLHealthChangeEffect::StaticClass(), -10000.f);
	}
	else if (Action == ECCLExperimentAction::BiologyAge || Action == ECCLExperimentAction::BiologyProtect)
	{
		auto Updated = *Current;
		auto* B = Updated.Features[CCLAgentTags::Feature_Biology].Data.GetMutablePtr<FCCLAgentBiology>();
		if (!B)
		{
			return false;
		}
		const double Age = CCLAgentBiology::AgeSeconds(*B, Life.GetTime());
		if (Action == ECCLExperimentAction::BiologyAge)
		{
			B->LifespanSeconds = FMath::Max(B->MaturitySeconds + 1., Age + 10.);
		}
		else
		{
			if (B->bProtectNaturalAging && B->LifespanSeconds > 0. && Age >= B->LifespanSeconds)
			{
				Error = TEXT("수명을 넘긴 표본의 보호는 해제할 수 없다. 전체 초기화 뒤 비교해 줘.");
				return false;
			}
			B->bProtectNaturalAging = !B->bProtectNaturalAging;
		}
		if (!CCLAgentBiology::ValidateAt(Updated, Life.GetTime(), Error))
		{
			return false;
		}
		auto& Store = Life.GetAgents();
		const auto Lease = Store.Acquire(Store.GetHandle(Updated.Id), FGuid::NewGuid());
		const bool bOK = Store.Commit(Lease, Updated, Life.GetRegistry(), Error);
		Store.Release(Lease);
		if (!bOK)
		{
			return false;
		}
	}
	else
	{
		return false;
	}
	Reconcile();
	UpdateView();
	ForceNetUpdate();
	return true;
}

void ACCLBiologyExperiment::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority() && bInitialized)
	{
		Reconcile();
		UpdateView();
	}
}

ACCLBiologyExperiment* ACCLBiologyExperiment::Find(const UWorld* World)
{
	for (TActorIterator<ACCLBiologyExperiment> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}
