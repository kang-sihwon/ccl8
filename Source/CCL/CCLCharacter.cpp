#include "CCLCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

// 생성자

ACCLCharacter::ACCLCharacter()
{
	bReplicates = true;
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
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetRelativeScale3D(FVector(0.65f, 0.65f, 1.8f));
	FacingMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FacingMarker"));
	FacingMarker->SetupAttachment(Body);
	FacingMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FacingMarker->SetRelativeLocation(FVector(65.f, 0.f, 20.f));
	FacingMarker->SetRelativeScale3D(FVector(0.7f, 0.25f, 0.15f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		Body->SetStaticMesh(Cube.Object);
		FacingMarker->SetStaticMesh(Cube.Object);
	}
}

// 부모 인터페이스 함수

float ACCLCharacter::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || IsDead() || !FMath::IsFinite(DamageAmount) || DamageAmount <= 0.f)
	{
		return 0.f;
	}
	const float AppliedDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (AppliedDamage > 0.f)
	{
		Die();
	}
	return AppliedDamage;
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
	}
}
