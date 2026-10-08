#include "CCLAgentComponent.h"

#include "CCLAgentWorldSubsystem.h"
#include "CCLAgentTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

UCCLAgentComponent::UCCLAgentComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 1.f;
}

void UCCLAgentComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()->HasAuthority())
	{
		if (auto* World = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>(); World && World->IsRunning())
		{
			World->GetSimulation().SetActorActive(AgentId, true);
		}
	}
}

void UCCLAgentComponent::EndPlay(EEndPlayReason::Type Reason)
{
	if (GetOwner()->HasAuthority())
	{
		if (auto* World = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>(); World && World->IsRunning())
		{
			World->GetSimulation().UpdateLocation(AgentId, GetOwner()->GetActorLocation());
			World->GetSimulation().SetActorActive(AgentId, false);
		}
	}

	Super::EndPlay(Reason);
}

void UCCLAgentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function)
{
	Super::TickComponent(DeltaTime, TickType, Function);
	if (GetOwner()->HasAuthority())
	{
		if (auto* World = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>(); World && World->IsRunning())
		{
			auto& Simulation = World->GetSimulation();
			EnsureRecord();
			if (!bHealthRestored)
			{
				RestoreHealthFromRecord();
			}
			Simulation.SetActorActive(AgentId, true);
			Simulation.UpdateLocation(AgentId, GetOwner()->GetActorLocation());
		}
	}
}

void UCCLAgentComponent::EnsureRecord()
{
	auto* World = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	if (!GetOwner()->HasAuthority() || !World || !World->IsRunning())
	{
		return;
	}
	auto& Simulation = World->GetSimulation();
	if (bCreateStandaloneRecord && !Simulation.Find(AgentId))
	{
		if (!AgentId.IsValid())
		{
			AgentId = FGuid::NewGuid();
		}
		FCCLAgentRecord Record;
		Record.Id = AgentId;
		Record.DefinitionId = FPrimaryAssetId(TEXT("Agent"), TEXT("Combatant"));
		Record.Location.Position = GetOwner()->GetActorLocation();
		Record.LastSimulatedTime = Simulation.GetTime();
		Record.Features.Add(CCLAgentTags::Feature_Traits, {1, FInstancedStruct::Make(DefaultTraits)});
		Record.Features.Add(CCLAgentTags::Feature_Needs, {1, FInstancedStruct::Make(FCCLAgentNeeds())});
		Record.Features.Add(CCLAgentTags::Feature_Experience, {1, FInstancedStruct::Make(FCCLAgentExperience())});
		FString Error;
		Simulation.GetAgents().Add(Record, Simulation.GetRegistry(), Error);
	}
}

void UCCLAgentComponent::RestoreHealthFromRecord()
{
	EnsureRecord();
	auto* World = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	auto* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	const auto* Record = World && World->IsRunning() ? World->GetSimulation().Find(AgentId) : nullptr;
	const auto* Needs = Record ? Record->Features.Find(CCLAgentTags::Feature_Needs) : nullptr;
	if (!GetOwner()->HasAuthority() || !ASC || !Needs || ASC->GetAvatarActor() != GetOwner() ||
		!ASC->HasAttributeSetForAttribute(UCCLHealthSet::GetHealthAttribute()))
	{
		return;
	}
	const float Injury = Needs->Data.Get<FCCLAgentNeeds>().Urgency.FindRef(CCLAgentTags::Injury);
	ASC->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(),
		(1 - Injury) * ASC->GetNumericAttribute(UCCLHealthSet::GetMaxHealthAttribute()));
	bHealthRestored = 1;
}

void UCCLAgentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCCLAgentComponent, AgentId);
}

void UCCLAgentComponent::RecordDamage(AActor* PerceivedSource, float HealthFraction)
{
	EnsureRecord();
	auto* World = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	if (!GetOwner()->HasAuthority() || !World || !World->IsRunning() || !FMath::IsFinite(HealthFraction))
	{
		return;
	}
	auto& Simulation = World->GetSimulation();
	const auto* Current = Simulation.Find(AgentId);
	if (!Current)
	{
		return;
	}
	auto Updated = *Current;
	if (auto* Feature = Updated.Features.Find(CCLAgentTags::Feature_Needs))
	{
		auto& Needs = Feature->Data.GetMutable<FCCLAgentNeeds>();
		Needs.Urgency.Add(CCLAgentTags::Injury, FMath::Clamp(1 - HealthFraction, 0.f, 1.f));
		Needs.Urgency.Add(CCLAgentTags::Safety, FMath::Clamp(1 - HealthFraction, 0.f, 1.f));
		auto& Fear = Needs.Emotions.FindOrAdd(CCLAgentTags::Fear);
		Fear.Intensity = FMath::Min(1.f, Fear.Intensity + 0.35f);
		Fear.HalfLifeSeconds = 3600;
	}
	if (auto* Feature = Updated.Features.Find(CCLAgentTags::Feature_Experience))
	{
		FCCLObservation Observation;
		Observation.EventId = Observation.EvidenceId = FGuid::NewGuid();
		Observation.Time = Simulation.GetTime();
		Observation.EventType = CCLAgentTags::Failure;
		const auto* Source = PerceivedSource ? PerceivedSource->FindComponentByClass<UCCLAgentComponent>() : nullptr;
		bool bIdentified = false;
		if (Source && Simulation.Find(Source->AgentId))
		{
			FVector Eye;
			FRotator Facing;
			GetOwner()->GetActorEyesViewPoint(Eye, Facing);
			const FVector Direction = PerceivedSource->GetActorLocation() - Eye;
			FHitResult Obstruction;
			FCollisionQueryParams Query(SCENE_QUERY_STAT(CCLAgentIdentity), false, GetOwner());
			const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Obstruction, Eye, PerceivedSource->GetActorLocation(), ECC_Visibility, Query);
			bIdentified = Direction.SizeSquared() < FMath::Square(1800.f) && FVector::DotProduct(Facing.Vector(), Direction.GetSafeNormal()) > 0.3f &&
				(!bBlocked || Obstruction.GetActor() == PerceivedSource);
		}
		Observation.PerceivedSubject.Kind = bIdentified ? CCLAgentTags::Agent : CCLAgentTags::Unknown;
		Observation.PerceivedSubject.Id = bIdentified ? Source->AgentId : FGuid();
		CCLAgentFeatures::Observe(Feature->Data.GetMutable<FCCLAgentExperience>(), Observation);
	}
	auto& Store = Simulation.GetAgents();
	const auto Lease = Store.Acquire(Store.GetHandle(AgentId), FGuid::NewGuid());
	FString Error;
	Store.Commit(Lease, Updated, Simulation.GetRegistry(), Error);
	Store.Release(Lease);
}

bool UCCLAgentComponent::ShouldEngage() const
{
	const auto* World = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	const auto* Record = World && World->IsRunning() ? World->GetSimulation().Find(AgentId) : nullptr;
	if (!Record)
	{
		return true;
	}
	FCCLDecisionInput Input;
	Input.Time = World->GetSimulation().GetTime();
	if (const auto* Feature = Record->Features.Find(CCLAgentTags::Feature_Traits))
	{
		Input.Traits = Feature->Data.Get<FCCLAgentTraits>();
	}
	if (const auto* Feature = Record->Features.Find(CCLAgentTags::Feature_Needs))
	{
		Input.Needs = Feature->Data.Get<FCCLAgentNeeds>();
	}
	FCCLDecisionCandidate Attack;
	Attack.OpportunityId = FGuid(0xCC18, 9, 0, 1);
	Attack.Activity = CCLAgentTags::Aggressiveness;
	Attack.BaseUtility = 0.4f;
	Attack.TraitSignals.Add(CCLAgentTags::Aggressiveness, 0.7f);
	Attack.TraitSignals.Add(CCLAgentTags::RiskTolerance, 0.7f);
	FCCLDecisionCandidate Withdraw;
	Withdraw.OpportunityId = FGuid(0xCC18, 9, 0, 2);
	Withdraw.Activity = CCLAgentTags::Safety;
	Withdraw.NeedRelief.Add(CCLAgentTags::Safety, 1);
	Withdraw.TraitSignals.Add(CCLAgentTags::SelfControl, 0.3f);
	Input.Candidates = {Attack, Withdraw};
	return CCLDecision::Evaluate(Input).Intent.OpportunityId == Attack.OpportunityId;
}
