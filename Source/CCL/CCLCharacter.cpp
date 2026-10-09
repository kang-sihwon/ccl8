#include "CCLCharacter.h"
#include "Environment/CCLSnowMovementComponent.h"
#include "Environment/CCLSnowAnimInstance.h"

#include "Map/CCLMapSystem.h"

#include "CCLPlayerState.h"
#include "Items/CCLLoadoutComponent.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystem/CCLEffects.h"
#include "Combat/CCLFighterComponent.h"
#include "Items/CCLAttachmentProfile.h"
#include "Combat/CCLCombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

// 생성자

ACCLCharacter::ACCLCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCCLSnowMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	auto* MapMarker = CreateDefaultSubobject<UCCLMapMarkerComponent>(TEXT("MapMarker"));
	MapMarker->Kind = ECCLMapKind::Player;

	bReplicates = true;
	Fighter = CreateDefaultSubobject<UCCLFighterComponent>(TEXT("Fighter"));
	Fighter->AttachmentProfile = TSoftObjectPtr<UCCLAttachmentProfile>(FSoftObjectPath(TEXT("/Game/Progression/DA_HumanoidAttachments.DA_HumanoidAttachments")));
	Combat = CreateDefaultSubobject<UCCLCombatComponent>(TEXT("Combat"));
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.25f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 420.f;
	CameraBoom->SocketOffset = FVector(0.f, 40.f, 65.f);
	CameraBoom->bUsePawnControlRotation = true;
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(RootComponent);
	Body->SetHiddenInGame(true);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetRelativeScale3D(FVector(0.65f, 0.65f, 1.8f));
	FacingMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FacingMarker"));
	FacingMarker->SetupAttachment(Body);
	FacingMarker->SetHiddenInGame(true);
	FacingMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FacingMarker->SetRelativeLocation(FVector(65.f, 0.f, 20.f));
	FacingMarker->SetRelativeScale3D(FVector(0.7f, 0.25f, 0.15f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));

	if (Cube.Succeeded())
	{
		Body->SetStaticMesh(Cube.Object);
		FacingMarker->SetStaticMesh(Cube.Object);
	}

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Manny(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));

	if (Manny.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(Manny.Object);
		GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -96.f));
		GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> Anim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));

	if (Anim.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(Anim.Class);
	}
}

// 부모 인터페이스 함수

void ACCLCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (GetWorld()->GetMapName().Contains(TEXT("Environment")))
	{
		if (auto* SnowClass = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Environment/Animation/ABP_SnowPostProcess.ABP_SnowPostProcess_C")))
		{
			GetMesh()->SetOverridePostProcessAnimBP(SnowClass);
		}
	}
}

void ACCLCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeAbilitySystem();
}

void ACCLCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitializeAbilitySystem();
}

void ACCLCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BoundASC.IsValid())
	{
		BoundASC->GetGameplayAttributeValueChangeDelegate(UCCLHealthSet::GetHealthAttribute()).Remove(HealthChanged);
	}

	Super::EndPlay(EndPlayReason);
}

UAbilitySystemComponent* ACCLCharacter::GetAbilitySystemComponent() const
{
	const auto* State = GetPlayerState<ACCLPlayerState>();
	return State ? State->GetAbilitySystemComponent() : nullptr;
}

float ACCLCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || IsDead() || !FMath::IsFinite(DamageAmount) || DamageAmount <= 0.f)
	{
		return 0.f;
	}

	auto* ASC = Cast<UCCLAbilitySystemComponent>(GetAbilitySystemComponent());

	if (!ASC)
	{
		return 0.f;
	}

	const float Before = ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute());
	const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (AppliedDamage > 0.f)
	{
		ASC->ApplyEffect(UCCLHealthChangeEffect::StaticClass(), -AppliedDamage);
	}

	return FMath::Max(0.f, Before - ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()));
}

void ACCLCharacter::FellOutOfWorld(const UDamageType& DamageType)
{
	Die();
}

void ACCLCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACCLCharacter, bDead);
}

// 내 클래스 함수

void ACCLCharacter::Die()
{
	if (!HasAuthority() || IsDead())
	{
		return;
	}

	bDead = 1;
	Fighter->EndLife();

	if (auto* ASC = Cast<UCCLAbilitySystemComponent>(GetAbilitySystemComponent()); ASC && ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) > 0.f)
	{
		ASC->ApplyEffect(UCCLHealthChangeEffect::StaticClass(), -ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()));
	}

	OnRep_Dead();
	ForceNetUpdate();
	UE_LOG(LogTemp, Display, TEXT("CCL Death Pawn=%s Controller=%s"), *GetName(), *GetNameSafe(Controller));
}

void ACCLCharacter::OnRep_Dead()
{
	if (IsDead())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Body->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
		GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 90.f));
	}
}

void ACCLCharacter::InitializeAbilitySystem()
{
	auto* ASC = Cast<UCCLAbilitySystemComponent>(GetAbilitySystemComponent());

	if (!ASC || BoundASC.Get() == ASC)
	{
		return;
	}

	if (BoundASC.IsValid())
	{
		BoundASC->GetGameplayAttributeValueChangeDelegate(UCCLHealthSet::GetHealthAttribute()).Remove(HealthChanged);
	}

	ASC->InitAbilityActorInfo(GetPlayerState(), this);
	Fighter->Initialize(ASC);
	if (auto* State = GetPlayerState<ACCLPlayerState>())
	{
		State->GetLoadout()->SyncAvatar(true);
	}
	BoundASC = ASC;
	HealthChanged = ASC->GetGameplayAttributeValueChangeDelegate(UCCLHealthSet::GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
}

void ACCLCharacter::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (HasAuthority() && Data.NewValue <= 0.f && BoundASC.IsValid() && BoundASC->GetAvatarActor() == this)
	{
		Die();
	}
}
