#include "CCLBiologyExperiment.h"

#include "CCLExperimentDefinition.h"
#include "CCLWorldSnapshot.h"
#include "CCLWorldSimulationSubsystem.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Agents/CCLAgentComponent.h"
#include "Agents/CCLAgentTags.h"
#include "Campaign/CCLLifeVillager.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystem/CCLEffects.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

namespace
{
FCCLAgentRecord MakeRecord(int32 Index, double Time)
{
    FCCLAgentRecord R;
    R.Id = ACCLBiologyExperiment::AgentId(Index);
    R.DefinitionId = FPrimaryAssetId(TEXT("Agent"), TEXT("BiologyExperiment"));
    R.LastSimulatedTime = Time;
    R.Location.Position = UCCLExperimentDefinition::ZoneCenter(7) + FVector(Index ? 260. : -260., 700., 90.);
    FCCLAgentBiology B;
    B.BirthWorldSeconds = Time - 20.;
    B.MaturitySeconds = 10.;
    B.LifespanSeconds = 300.;
    B.bProtectNaturalAging = Index == 1;
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
    for (int32 Index = 0; Index < 2; ++Index)
    {
        if (!Initial.Find(AgentId(Index)) &&
            !Initial.GetAgents().Add(MakeRecord(Index, Initial.GetTime()), Initial.GetRegistry(), Error).Id.IsValid())
        {
            return false;
        }
        if (bSeed && !Live.Find(AgentId(Index)) &&
            !Live.GetAgents().Add(MakeRecord(Index, Live.GetTime()), Live.GetRegistry(), Error).Id.IsValid())
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

ACCLLifeVillager* ACCLBiologyExperiment::FindActor(int32 Index) const
{
    for (TActorIterator<ACCLLifeVillager> It(GetWorld()); It; ++It)
    {
        if (!It->IsActorBeingDestroyed() && It->GetAgent()->AgentId == AgentId(Index))
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
    for (int32 Index = 0; Index < 2; ++Index)
    {
        auto* Actor = FindActor(Index);
        const auto* Record = Agents->GetSimulation().Find(AgentId(Index));
        const bool bAlive = Record && CCLAgentBiology::IsAlive(*Record);
        if (Actor && (bNewEpoch || !bAlive))
        {
            Actor->Destroy();
            Actor = nullptr;
        }
        if (bAlive && !Actor)
        {
            const FTransform Transform(FRotator(0., -90., 0.), Record->Location.Position);
            Actor = GetWorld()->SpawnActorDeferred<ACCLLifeVillager>(ACCLLifeVillager::StaticClass(), Transform,
                                                                     nullptr, nullptr,
                                                                     ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
            if (!Actor)
            {
                continue;
            }
            Actor->AutoPossessAI = EAutoPossessAI::Disabled;
            Actor->bAlwaysRelevant = true;
            Actor->GetAgent()->AgentId = AgentId(Index);
            Actor->PublicName = Index ? TEXT("AGING PROTECTED") : TEXT("NATURAL AGE");
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
    for (int32 Index = 0; Index < 2; ++Index)
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
            Row.Health = Actor->GetAbilitySystemComponent()->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute());
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
    const int32 Index = Action == ECCLExperimentAction::BiologyAge ? 0 : 1;
    const auto* Current = Life.Find(AgentId(Index));
    if (!Current || !CCLAgentBiology::IsAlive(*Current))
    {
        Error = TEXT("살아 있는 생애 표본이 없다. 전체 초기화로 다시 시작해 줘.");
        return false;
    }
    if (Action == ECCLExperimentAction::BiologyDamage)
    {
        auto* Actor = FindActor(1);
        auto* ASC = Actor ? Cast<UCCLAbilitySystemComponent>(Actor->GetAbilitySystemComponent()) : nullptr;
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
