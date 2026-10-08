#include "CCLLifeVillager.h"

#include "Agents/CCLAgentComponent.h"
#include "Agents/CCLAgentAIController.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Agents/CCLAgentTags.h"
#include "Actions/CCLActionComponent.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Map/CCLMapSystem.h"

ACCLLifeVillager::ACCLLifeVillager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
	Agent = CreateDefaultSubobject<UCCLAgentComponent>(TEXT("Agent"));
	AbilitySystem = CreateDefaultSubobject<UCCLAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	Health = CreateDefaultSubobject<UCCLHealthSet>(TEXT("Health"));
	Actions = CreateDefaultSubobject<UCCLActionComponent>(TEXT("Actions"));
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->MaxWalkSpeed = 180;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = ACCLAgentAIController::StaticClass();
}

void ACCLLifeVillager::BeginPlay()
{
	Super::BeginPlay();
	AbilitySystem->InitAbilityActorInfo(this, this);
	if (HasAuthority())
	{
		const auto* World = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
		const auto* Record = World ? World->GetSimulation().Find(Agent->AgentId) : nullptr;
		const auto* Needs = Record ? Record->Features.Find(CCLAgentTags::Feature_Needs) : nullptr;
		if (Needs)
		{
			const float Injury = Needs->Data.Get<FCCLAgentNeeds>().Urgency.FindRef(CCLAgentTags::Injury);
			AbilitySystem->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), (1 - Injury) * 100);
		}
	}
	RefreshLabel();
}

void ACCLLifeVillager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const bool bIncapacitated = AbilitySystem->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) <= 0;
	if (HasAuthority() && bIncapacitated)
	{
		if (auto* AI = Cast<AAIController>(GetController()))
		{
			AI->StopMovement();
			if (auto* LifeAI = Cast<ACCLAgentAIController>(AI))
			{
				LifeAI->AbortIntent();
			}
			if (AI->GetBrainComponent())
			{
				AI->GetBrainComponent()->StopLogic(TEXT("Incapacitated"));
			}
		}
		SetActivity(CCLAgentTags::Injury);
	}
	if (auto* PC = GetWorld()->GetFirstPlayerController(); PC && PC->PlayerCameraManager)
	{
		if (auto* Label = FindComponentByClass<UTextRenderComponent>())
		{
			Label->SetWorldRotation((PC->PlayerCameraManager->GetCameraLocation() - Label->GetComponentLocation()).Rotation());
		}
	}
}

void ACCLLifeVillager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACCLLifeVillager, PublicName);
	DOREPLIFETIME(ACCLLifeVillager, PublicActivity);
}

UAbilitySystemComponent* ACCLLifeVillager::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}

void ACCLLifeVillager::SetActivity(FGameplayTag Activity)
{
	if (HasAuthority() && PublicActivity != Activity)
	{
		PublicActivity = Activity;
		RefreshLabel();
		ForceNetUpdate();
	}
}

FString ACCLLifeVillager::DescribeLife() const
{
	const auto* World = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	if (!World)
	{
		return TEXT("I'm finding my way around the village.");
	}

	const auto* Record = World->GetSimulation().Find(Agent->AgentId);
	const auto* LifeFeature = Record ? Record->Features.Find(CCLAgentTags::Feature_Life) : nullptr;
	const auto* ResourceFeature = Record ? Record->Features.Find(CCLAgentTags::Feature_Resources) : nullptr;
	const auto* Life = LifeFeature ? LifeFeature->Data.GetPtr<FCCLLifeState>() : nullptr;
	const auto* Links = ResourceFeature ? ResourceFeature->Data.GetPtr<FCCLAgentResourceLinks>() : nullptr;
	const auto* Account = Links ? World->GetSimulation().GetEconomy().Accounts.Find(Links->Account) : nullptr;
	const FCCLLifeGoalState* Goal = nullptr;
	if (Life)
	{
		for (const auto& Candidate : Life->LifeGoals)
		{
			if (Candidate.Status == CCLAgentTags::Active && (!Goal || Candidate.Commitment > Goal->Commitment))
			{
				Goal = &Candidate;
			}
		}
	}
	return FString::Printf(TEXT("I'm %s. Today: %s.\nMy priority is %s (%.0f%%). I have %lld coins.\nA failed deal or a helpful neighbor can change my next decision."),
		*PublicName, *PublicActivity.ToString().RightChop(15), Goal ? *Goal->GoalTag.ToString().RightChop(11) : TEXT("rest"),
		Goal ? World->GetSimulation().GoalProgress(*Record, *Goal) * 100 : 0, Account ? Account->Balance : 0);
}

void ACCLLifeVillager::RefreshLabel()
{
	DisplayName = FText::FromString(PublicName);
	if (auto* Marker = FindComponentByClass<UCCLMapMarkerComponent>())
	{
		Marker->Label = DisplayName;
	}
	if (auto* Label = FindComponentByClass<UTextRenderComponent>())
	{
		Label->SetText(FText::FromString(PublicName + TEXT("\n") + PublicActivity.ToString().RightChop(15)));
		Label->SetWorldSize(18.f);
	}
}
