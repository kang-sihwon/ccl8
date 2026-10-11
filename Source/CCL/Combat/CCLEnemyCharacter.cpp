#include "CCLEnemyCharacter.h"

#include "CCLCombatComponent.h"
#include "CCLCombatDefinition.h"
#include "CCLEnemyAIController.h"
#include "CCLEnemyDefinition.h"
#include "CCLFighterComponent.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "Actions/CCLActionComponent.h"
#include "Agents/CCLAgentComponent.h"
#include "Agents/CCLAgentTags.h"
#include "Environment/CCLThermalExposure.h"
#include "Items/CCLItemDefinition.h"
#include "Map/CCLMapSystem.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

// 생성자

ACCLEnemyCharacter::ACCLEnemyCharacter()
{
	CreateDefaultSubobject<UCCLThermalReceiverComponent>(TEXT("ThermalReceiver"));
	auto* MapMarker = CreateDefaultSubobject<UCCLMapMarkerComponent>(TEXT("MapMarker"));
	MapMarker->Kind = ECCLMapKind::Enemy;

	CreateDefaultSubobject<UCCLActionComponent>(TEXT("Actions"));
	auto* Agent = CreateDefaultSubobject<UCCLAgentComponent>(TEXT("Agent"));
	Agent->bCreateStandaloneRecord = 1;
	Agent->DefaultTraits.Axes.Add(CCLAgentTags::Aggressiveness, 0.8f);
	Agent->DefaultTraits.Axes.Add(CCLAgentTags::RiskTolerance, 0.6f);
	bReplicates = true;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = ACCLEnemyAIController::StaticClass();
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	GetCharacterMovement()->MaxWalkSpeed = 260.f;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	AbilitySystem = CreateDefaultSubobject<UCCLAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	Health = CreateDefaultSubobject<UCCLHealthSet>(TEXT("Health"));
	Fighter = CreateDefaultSubobject<UCCLFighterComponent>(TEXT("Fighter"));
	Fighter->Team = 1;
	Fighter->InitialHealth = 80.f;
	Fighter->DefaultItem = TSoftObjectPtr<UCCLItemDefinition>(FSoftObjectPath(TEXT("/Game/Combat/DA_EnemyUnarmed.DA_EnemyUnarmed")));
	Fighter->CombatAnimClass = TSoftClassPtr<UAnimInstance>(FSoftObjectPath(TEXT("/Game/Combat/ABP_Combat.ABP_Combat_C")));
	Fighter->DodgeMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Combat/AM_Dodge.AM_Dodge")));
	Fighter->DefenseMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(TEXT("/Game/Combat/AM_DefensePose.AM_DefensePose")));
	Combat = CreateDefaultSubobject<UCCLCombatComponent>(TEXT("Combat"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));

	if (MannyMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MannyMesh.Object);
	}

	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -96.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	static ConstructorHelpers::FClassFinder<UAnimInstance> Anim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));

	if (Anim.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(Anim.Class);
	}
}

// 부모 인터페이스 함수

void ACCLEnemyCharacter::BeginPlay()
{
	// A definition chosen in the editor applies here; a spawner applies it before FinishSpawning.
	if (Definition && !bDefinitionApplied)
	{
		ApplyDefinition(Definition);
	}

	Super::BeginPlay();
	SpawnTransform = GetActorTransform();
	AbilitySystem->InitAbilityActorInfo(this, this);
	Fighter->Initialize(AbilitySystem);
	SelectAttackPattern();
	HealthChanged = AbilitySystem->GetGameplayAttributeValueChangeDelegate(UCCLHealthSet::GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
	FindComponentByClass<UCCLAgentComponent>()->RestoreHealthFromRecord();
}

void ACCLEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RespawnTimer);
	AbilitySystem->GetGameplayAttributeValueChangeDelegate(UCCLHealthSet::GetHealthAttribute()).Remove(HealthChanged);
	Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent* ACCLEnemyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}

void ACCLEnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACCLEnemyCharacter, bDead);
	DOREPLIFETIME(ACCLEnemyCharacter, DisplayName);
	DOREPLIFETIME(ACCLEnemyCharacter, Definition);
	DOREPLIFETIME(ACCLEnemyCharacter, PatternLabel);
}

// 내 클래스 함수

void ACCLEnemyCharacter::ApplyDefinition(UCCLEnemyDefinition* InDefinition)
{
	Definition = InDefinition;
	bDefinitionApplied = 1;
	PatternCursor = INDEX_NONE;
	if (!Definition)
	{
		return;
	}

	DisplayName = Definition->DisplayName.ToString();
	DetectionRadius = Definition->DetectionRadius;
	LeashRadius = Definition->LeashRadius;
	Fighter->InitialHealth = Definition->InitialHealth;
	GetCharacterMovement()->MaxWalkSpeed = Definition->WalkSpeed;
}

void ACCLEnemyCharacter::SelectAttackPattern()
{
	if (!HasAuthority() || IsDead() || AbilitySystem->HasMatchingGameplayTag(CCLTags::State_Busy) || !Definition || Definition->Patterns.IsEmpty())
	{
		return;
	}

	// Continue after the last pattern so that unlocked patterns alternate in list order.
	const auto& Patterns = Definition->Patterns;
	const float MaxHealth = Health->GetMaxHealth();
	const float Fraction = MaxHealth > 0.f ? Health->GetHealth() / MaxHealth : 0.f;
	for (int32 Step = 1; Step <= Patterns.Num(); ++Step)
	{
		const int32 Index = (PatternCursor + Step + Patterns.Num()) % Patterns.Num();
		if (Fraction <= Patterns[Index].MaxHealthFraction)
		{
			PatternCursor = Index;
			Fighter->AttackOverride = Patterns[Index].Attack;
			PatternLabel = Patterns[Index].Label.ToString();
			return;
		}
	}
}

void ACCLEnemyCharacter::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (!HasAuthority() || bDead || Data.NewValue > 0.f)
	{
		return;
	}

	bDead = 1;
	Fighter->EndLife();

	if (auto* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();

		if (AI->GetBrainComponent())
		{
			AI->GetBrainComponent()->StopLogic(TEXT("Dead"));
		}
	}

	OnRep_Dead();
	ForceNetUpdate();
	OnDefeated.Broadcast(this);

	if (bRespawnEnabled)
	{
		GetWorldTimerManager().SetTimer(RespawnTimer, this, &ThisClass::Respawn, 5.f);
	}
}

void ACCLEnemyCharacter::Respawn()
{
	auto* Replacement = GetWorld()->SpawnActorDeferred<ACCLEnemyCharacter>(GetClass(), SpawnTransform, GetOwner(), nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
	if (!Replacement)
	{
		GetWorldTimerManager().SetTimer(RespawnTimer, this, &ThisClass::Respawn, 0.5f);
		return;
	}

	CopyConfigurationTo(*Replacement);
	Replacement->FinishSpawning(SpawnTransform);
	if (!IsValid(Replacement))
	{
		GetWorldTimerManager().SetTimer(RespawnTimer, this, &ThisClass::Respawn, 0.5f);
		return;
	}

	// The record now belongs to the replacement; this body must not mark it inactive while leaving.
	if (auto* Agent = FindComponentByClass<UCCLAgentComponent>())
	{
		Agent->AgentId.Invalidate();
	}

	Destroy();
}

void ACCLEnemyCharacter::CopyConfigurationTo(ACCLEnemyCharacter& Replacement) const
{
	// The replacement keeps the encounter's type, instance overrides, listeners and Agent identity.
	Replacement.Definition = Definition;
	Replacement.bDefinitionApplied = 1;
	Replacement.bRespawnEnabled = bRespawnEnabled;
	Replacement.DisplayName = DisplayName;
	Replacement.DetectionRadius = DetectionRadius;
	Replacement.LeashRadius = LeashRadius;
	Replacement.OnDefeated = OnDefeated;
	Replacement.Fighter->InitialHealth = Fighter->InitialHealth;
	Replacement.GetCharacterMovement()->MaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	Replacement.AIControllerClass = AIControllerClass;

	const auto* Agent = FindComponentByClass<UCCLAgentComponent>();
	auto* ReplacementAgent = Replacement.FindComponentByClass<UCCLAgentComponent>();
	if (Agent && ReplacementAgent)
	{
		ReplacementAgent->AgentId = Agent->AgentId;
		ReplacementAgent->DefaultTraits = Agent->DefaultTraits;
		ReplacementAgent->ClearInjury();
	}
}

void ACCLEnemyCharacter::OnRep_Dead()
{
	if (!bDead)
	{
		return;
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 90.f));
}
