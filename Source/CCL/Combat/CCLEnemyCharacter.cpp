#include "CCLEnemyCharacter.h"

#include "CCLEnemyAIController.h"
#include "CCLFighterComponent.h"
#include "CCLCombatComponent.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BrainComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"

ACCLEnemyCharacter::ACCLEnemyCharacter()
{
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

void ACCLEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	SpawnTransform = GetActorTransform();
	AbilitySystem->InitAbilityActorInfo(this, this);
	Fighter->Initialize(AbilitySystem);
	HealthChanged = AbilitySystem->GetGameplayAttributeValueChangeDelegate(UCCLHealthSet::GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
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
	GetWorldTimerManager().SetTimer(RespawnTimer, this, &ThisClass::Respawn, 5.f);
}

void ACCLEnemyCharacter::Respawn()
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

	if (GetWorld()->SpawnActor<ACCLEnemyCharacter>(GetClass(), SpawnTransform, Params))
	{
		Destroy();
	}
	else
	{
		GetWorldTimerManager().SetTimer(RespawnTimer, this, &ThisClass::Respawn, 0.5f);
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
