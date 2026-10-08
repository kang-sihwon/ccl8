#include "CCLProjectile.h"

#include "CCLCombatDefinition.h"
#include "CCLHitRule.h"
#include "CCLFighterComponent.h"
#include "AbilitySystem/CCLOffenseSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ACCLProjectile::ACCLProjectile()
{
	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(60.f);
	InitialLifeSpan = 8.f;
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(3.f);
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	RootComponent = Collision;
	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Collision);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetRelativeScale3D(FVector(0.18f, 0.05f, 0.05f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Visual->SetStaticMesh(Mesh.Object);
	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->SetUpdatedComponent(Collision);
	Movement->bRotationFollowsVelocity = true;
	Movement->bForceSubStepping = true;
	Movement->MaxSimulationTimeStep = 1.f / 120.f;
	Movement->MaxSimulationIterations = 16;
	Collision->OnComponentHit.AddDynamic(this, &ThisClass::Impact);
}

void ACCLProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner())
	{
		Collision->IgnoreActorWhenMoving(GetOwner(), true);
	}

	if (!HasAuthority())
	{
		Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Movement->Deactivate();
	}
}

void ACCLProjectile::Launch(UAbilitySystemComponent* ASC, const UCCLCombatDefinition* Definition,
	FVector Velocity, float Gravity)
{
	if (!HasAuthority() || !ASC || !Definition || Velocity.ContainsNaN() || !FMath::IsFinite(Gravity))
	{
		Destroy();
		return;
	}

	SourceASC = ASC;
	ShotDefinition = DuplicateObject<UCCLCombatDefinition>(Definition, this);
	CapturedEffect = ASC->MakeOutgoingSpec(Definition->DamageEffect, 1.f, ASC->MakeEffectContext());
	if (const auto* Fighter = GetOwner() ? GetOwner()->FindComponentByClass<UCCLFighterComponent>() : nullptr)
	{
		CapturedTeam = Fighter->Team;
	}
	if (ASC->HasAttributeSetForAttribute(UCCLOffenseSet::GetAttackBonusAttribute()))
	{
		ShotDefinition->Damage += ASC->GetNumericAttribute(UCCLOffenseSet::GetAttackBonusAttribute());
	}

	Movement->ProjectileGravityScale = Gravity;
	Movement->Velocity = Velocity;
	Movement->MaxSpeed = 0;
}

void ACCLProjectile::Impact(UPrimitiveComponent* HitComponent, AActor* Other,
	UPrimitiveComponent* OtherComponent, FVector Impulse, const FHitResult& Hit)
{
	if (!HasAuthority() || bResolved || Other == GetOwner())
	{
		return;
	}

	bResolved = 1;
	auto* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Other);
	if (CapturedEffect.IsValid() && ShotDefinition && TargetASC && IsValid(Other))
	{
		FCCLHitContext Context{IsValid(GetOwner()) ? GetOwner() : this, Other,
			IsValid(SourceASC) ? SourceASC.Get() : nullptr, TargetASC, ShotDefinition, Hit, 0};
		Context.bDetachedShot = 1;
		Context.CapturedEffect = CapturedEffect;
		Context.CapturedTeam = CapturedTeam;
		Context.IncomingDirection = (Hit.TraceEnd - Hit.TraceStart).GetSafeNormal();
		CCLHit::Apply(Context);
	}

	Destroy();
}
