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

namespace
{
const TCHAR* LifeLabel(FGameplayTag Tag)
{
	if (Tag == CCLAgentTags::Injury) { return TEXT("다쳐서 쉬는 중이야"); }
	if (Tag == CCLAgentTags::Work) { return TEXT("일하는 중이야"); }
	if (Tag == CCLAgentTags::Trade) { return TEXT("거래하는 중이야"); }
	if (Tag == CCLAgentTags::Eat) { return TEXT("식사하는 중이야"); }
	if (Tag == CCLAgentTags::Rest) { return TEXT("쉬는 중이야"); }
	if (Tag == CCLAgentTags::Help) { return TEXT("이웃을 돕는 중이야"); }
	if (Tag == CCLAgentTags::Repay) { return TEXT("빚을 갚는 중이야"); }
	if (Tag == CCLAgentTags::Improve) { return TEXT("실력을 기르는 중이야"); }
	if (Tag == CCLAgentTags::Discover) { return TEXT("새로운 곳을 둘러보는 중이야"); }
	if (Tag == CCLAgentTags::Goal_Living) { return TEXT("생계 유지"); }
	if (Tag == CCLAgentTags::Goal_Debt) { return TEXT("빚 갚기"); }
	if (Tag == CCLAgentTags::Goal_Support) { return TEXT("가족 돕기"); }
	if (Tag == CCLAgentTags::Goal_Business) { return TEXT("사업 성장"); }
	if (Tag == CCLAgentTags::Goal_Mastery) { return TEXT("기술 숙련"); }
	return TEXT("잠시 생각하는 중이야");
}
}

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
		return TEXT("마을을 둘러보고 있어.");
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
	return FString::Printf(TEXT("나는 %s이야. 지금은 %s.\n가장 중요한 목표는 %s이고, %.0f%% 진행했어. 동전은 %lld개 있어.\n거래에 실패하거나 이웃의 도움을 받으면 다음 선택이 달라질 수도 있어."),
		*PublicName, LifeLabel(PublicActivity), Goal ? LifeLabel(Goal->GoalTag) : TEXT("휴식"),
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
		Label->SetText(FText::FromString(PublicName + TEXT("\n") + LifeLabel(PublicActivity)));
		Label->SetWorldSize(18.f);
	}
}
