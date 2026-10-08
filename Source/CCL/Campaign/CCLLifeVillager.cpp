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

ACCLLifeVillager::ACCLLifeVillager()
{
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
	RefreshLabel();
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
	if (HasAuthority())
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

	return FString::Printf(TEXT("I'm %s. Today I'm working on %s. My plans can change when supplies run out or someone needs help."),
		*PublicName, *PublicActivity.ToString().RightChop(15));
}

void ACCLLifeVillager::RefreshLabel()
{
	DisplayName = FText::FromString(PublicName);
	if (auto* Label = FindComponentByClass<UTextRenderComponent>())
	{
		Label->SetText(FText::FromString(PublicName + TEXT("\n") + PublicActivity.ToString().RightChop(15)));
		Label->SetWorldSize(18.f);
	}
}
