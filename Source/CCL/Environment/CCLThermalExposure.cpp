#include "CCLThermalExposure.h"

#include "CCLSurfaceSimulation.h"
#include "CCLSurfaceQuery.h"
#include "CCLWorldSimulationSubsystem.h"
#include "Agents/CCLAgentComponent.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystem/CCLEffects.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/World.h"

bool FCCLThermalDamagePolicy::Validate() const
{
	return FMath::IsFinite(ThresholdCelsius) && ThresholdCelsius >= 0. && ThresholdCelsius <= 10000. &&
		   FMath::IsFinite(ContactDamagePerDegreeSecond) && ContactDamagePerDegreeSecond >= 0. &&
		   ContactDamagePerDegreeSecond <= 1000. && FMath::IsFinite(AirDamagePerDegreeSecond) &&
		   AirDamagePerDegreeSecond >= 0. && AirDamagePerDegreeSecond <= 1000. &&
		   FMath::IsFinite(MaximumDamagePerSecond) && MaximumDamagePerSecond >= 0. &&
		   MaximumDamagePerSecond <= 100000. && FMath::IsFinite(ContactHeightMeters) && ContactHeightMeters > 0. &&
		   ContactHeightMeters <= 10.;
}

bool CCLThermalExposure::Sample(const FCCLSurfaceSimulation& Surface, const FVector& Position,
								const FCCLThermalDamagePolicy& Policy, const ICCLSurfaceProvider* Shelter, FName BodyId,
								double& DamagePerSecond, FString& Error)
{
	if (!Policy.Validate() || Position.ContainsNaN() || !Surface.IsInitialized())
	{
		Error = TEXT("Invalid thermal receiver policy or position.");
		return false;
	}
	double Contact = 0.;
	for (const auto& Pair : Surface.GetRegions())
	{
		const auto& G = Pair.Value;
		if (G.Fire.Cells.IsEmpty())
		{
			continue;
		}
		const FVector Local = Position - G.OriginMeters;
		if (Local.X < 0. || Local.Y < 0. || Local.X >= G.Size.X * G.SpacingMeters ||
			Local.Y >= G.Size.Y * G.SpacingMeters)
		{
			continue;
		}
		const int32 Index =
			FMath::FloorToInt(Local.X / G.SpacingMeters) + FMath::FloorToInt(Local.Y / G.SpacingMeters) * G.Size.X;
		const auto& Cell = G.Cells[Index];
		const double Height = Position.Z - Cell.BedMeters;
		if (Height < 0. || Height > Policy.ContactHeightMeters || Position.Z >= Cell.CeilingMeters)
		{
			continue;
		}
		const double Rate =
			FMath::Max(0., G.Fire.Cells[Index].TemperatureCelsius(G.Fire.Material) - Policy.ThresholdCelsius) *
			Policy.ContactDamagePerDegreeSecond;
		if (Rate == 0.)
		{
			continue;
		}
		double Transmission = 1.;
		if (Shelter)
		{
			FCCLSurfaceQuery Q;
			Q.BodyId = BodyId;
			Q.OriginMeters = FCCLSurfaceThermalContext::CellPosition(G, Index);
			const FVector Segment = Position - Q.OriginMeters;
			if (Segment.Length() > 2.e-5)
			{
				Q.Direction = Segment.GetSafeNormal();
				Q.MaximumDistanceMeters = Segment.Length() - 1.e-5;
				Q.KindMask = uint8(ECCLSurfaceKind::Terrain) | uint8(ECCLSurfaceKind::Structure);
				Q.RequiredRevision = Shelter->GetRevision();
				Q.RequiredEpoch = Shelter->GetEpoch();
				TArray<FCCLSurfaceSample> Hits;
				if (!Shelter->QuerySurfaces(Q, Hits, Error))
				{
					return false;
				}
				for (const auto& Hit : Hits)
				{
					Transmission *= Hit.Transmission.Wind;
				}
			}
		}
		Contact = FMath::Max(Contact, Rate * Transmission);
	}
	const auto* Room = Surface.GetAirSimulation().FindSpaceAt(Position);
	const double Air =
		Room ? FMath::Max(0., Room->TemperatureCelsius() - Policy.ThresholdCelsius) * Policy.AirDamagePerDegreeSecond
			 : 0.;
	const double Rate = FMath::Min(Policy.MaximumDamagePerSecond, Contact + Air);
	if (!FMath::IsFinite(Rate))
	{
		Error = TEXT("Thermal exposure overflow.");
		return false;
	}
	DamagePerSecond = Rate;
	return true;
}

void UCCLThermalReceiverComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()->HasAuthority())
	{
		GetWorld()->GetSubsystem<UCCLThermalExposureSubsystem>()->Register(this);
	}
}

void UCCLThermalReceiverComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (auto* Subsystem = GetWorld()->GetSubsystem<UCCLThermalExposureSubsystem>())
	{
		Subsystem->Unregister(this);
	}
	Super::EndPlay(Reason);
}

bool UCCLThermalExposureSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const auto* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UCCLThermalExposureSubsystem::Register(UCCLThermalReceiverComponent* Receiver)
{
	Receivers.AddUnique(Receiver);
}

void UCCLThermalExposureSubsystem::Unregister(UCCLThermalReceiverComponent* Receiver)
{
	Receivers.Remove(Receiver);
}

void UCCLThermalExposureSubsystem::Cancel()
{
	Pending.Reset();
	PendingEpoch.Invalidate();
	PendingStep = 0;
}

bool UCCLThermalExposureSubsystem::Prepare(const FCCLWorldStep& Step, const FCCLSurfaceSimulation& Previous,
										   FString& Error)
{
	Cancel();
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	auto* Agents = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	const double Seconds = Step.GameToSeconds - Step.GameFromSeconds;
	if (GetWorld()->GetNetMode() == NM_Client || !Runtime || !Runtime->IsRunning() || !Agents ||
		!FMath::IsFinite(Seconds) || Seconds < 0. || Seconds > .250000001 || !Step.Ticket.IsValid() ||
		Step.StepId != Previous.GetStepId() + 1 || Step.GameFromSeconds != Previous.GetGameSeconds() ||
		Step.WorldFromSeconds != Previous.GetWorldSeconds() || Previous.GetWorldId() != Runtime->GetSurfaceSimulation().GetWorldId() ||
		Previous.GetStepId() != Runtime->GetSurfaceSimulation().GetStepId())
	{
		Error = TEXT("Invalid thermal step or authority.");
		return false;
	}
	TArray<FCCLPendingThermalDamage> Candidate;
	for (const auto& Weak : Receivers)
	{
		auto* Receiver = Weak.Get();
		auto* Owner = Receiver ? Receiver->GetOwner() : nullptr;
		if (!Owner || Owner->IsActorBeingDestroyed() || !Receiver->bEnabled)
		{
			continue;
		}
		auto* ASC = Cast<UCCLAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner));
		if (!ASC || ASC->GetAvatarActor() != Owner ||
			!ASC->HasAttributeSetForAttribute(UCCLHealthSet::GetHealthAttribute()) ||
			ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) <= 0.f)
		{
			continue;
		}
		double Rate = 0.;
		if (!CCLThermalExposure::Sample(Previous, Owner->GetActorLocation() / 100., Receiver->Policy,
										&Runtime->GetSurfaceProvider(), Runtime->GetEnvironmentInputs().Observer.BodyId,
										Rate, Error))
		{
			return false;
		}
		if (Rate * Seconds <= 0.)
		{
			continue;
		}
		if (const auto* Agent = Owner->FindComponentByClass<UCCLAgentComponent>())
		{
			auto& Life = Agents->GetSimulation();
			const auto* Record = Life.Find(Agent->AgentId);
			if (!Record || !CCLAgentBiology::IsAlive(*Record))
			{
				continue;
			}
			auto& Store = Life.GetAgents();
			const auto Lease = Store.Acquire(Store.GetHandle(Agent->AgentId), FGuid::NewGuid());
			if (!Lease.Writer.IsValid())
			{
				Error = TEXT("World awaits a thermal receiver Agent writer.");
				return false;
			}
			Store.Release(Lease);
		}
		Candidate.Add({Receiver, ASC, float(Rate * Seconds)});
	}
	Pending = MoveTemp(Candidate);
	PendingEpoch = Runtime->GetEpoch();
	PendingStep = Step.StepId;
	return true;
}

void UCCLThermalExposureSubsystem::Commit()
{
	const auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	if (!Runtime || GetWorld()->GetNetMode() == NM_Client || Runtime->GetEpoch() != PendingEpoch ||
		Runtime->GetSurfaceSimulation().GetStepId() != PendingStep)
	{
		Cancel();
		return;
	}
	auto Work = MoveTemp(Pending);
	Cancel();
	for (const auto& Item : Work)
	{
		auto* Receiver = Item.Receiver.Get();
		auto* Owner = Receiver ? Receiver->GetOwner() : nullptr;
		auto* ASC = Item.AbilitySystem.Get();
		if (!Owner || Owner->IsActorBeingDestroyed() || !ASC || ASC->GetAvatarActor() != Owner)
		{
			continue;
		}
		auto* Agent = Owner->FindComponentByClass<UCCLAgentComponent>();
		if (Agent)
		{
			const auto& Life = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>()->GetSimulation();
			const auto* Record = Life.Find(Agent->AgentId);
			if (!Record || !CCLAgentBiology::IsAlive(*Record))
			{
				continue;
			}
		}
		ASC->ApplyEffect(UCCLHealthChangeEffect::StaticClass(), -Item.Damage);
		if (Agent && IsValid(Owner))
		{
			const float Max = ASC->GetNumericAttribute(UCCLHealthSet::GetMaxHealthAttribute());
			if (Max > 0.f)
			{
				Agent->RecordDamage(nullptr, ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) / Max);
			}
		}
	}
}
