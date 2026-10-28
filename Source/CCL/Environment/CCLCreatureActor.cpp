#include "CCLCreatureActor.h"

#include "CCLEcologyRepresentationSubsystem.h"
#include "CCLWorldEnvironmentConfig.h"
#include "CCLWorldEnvironmentState.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimationAsset.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ACCLCreatureActor::ACCLCreatureActor()
{
	bReplicates = true;
	SetReplicateMovement(true);
	SetNetCullDistanceSquared(8000. * 8000.);
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(SceneRoot);
	VisualRoot->SetAbsolute(false, true, false);
	CreatureMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CreatureMesh"));
	CreatureMesh->SetupAttachment(VisualRoot);
	CreatureMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CreatureMesh->SetCanEverAffectNavigation(false);
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	for (int32 Index = 0; Index < 3; ++Index)
	{
		auto* Shape = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Shape%d"), Index));
		Shape->SetupAttachment(VisualRoot);
		Shape->SetStaticMesh(Index == 0 ? Sphere.Object : Cube.Object);
		Shape->SetMobility(EComponentMobility::Movable);
		Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Shapes.Add(Shape);
	}

	OnRep_Medium();
}

void ACCLCreatureActor::BeginPlay()
{
	Super::BeginPlay();
	OnRep_Medium();
	for (TActorIterator<ACCLWorldEnvironmentState> It(GetWorld()); It; ++It)
	{
		WorldTime = *It;
		break;
	}
}

void ACCLCreatureActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const FVector Target = GetActorLocation();
	if (!bVisualInitialized || FVector::DistSquared(VisualPosition, Target) > FMath::Square(500.))
	{
		VisualPosition = Target;
		SegmentStart = Target;
		LastTarget = Target;
		VisualRotation = GetActorRotation();
		bVisualInitialized = 1;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (!Target.Equals(LastTarget, .01))
	{
		SegmentDuration = LastUpdateSeconds >= 0. ? FMath::Clamp(Now - LastUpdateSeconds, .016, 1.) : 1.;
		LastUpdateSeconds = Now;
		SegmentStart = VisualPosition;
		LastTarget = Target;
		SegmentElapsed = 0.;
	}
	SegmentElapsed += DeltaSeconds;
	const FVector Previous = VisualPosition;
	VisualPosition = FMath::Lerp(SegmentStart, LastTarget, FMath::Clamp(SegmentElapsed / SegmentDuration, 0., 1.));
	const FVector Velocity = (VisualPosition - Previous) / FMath::Max(double(DeltaSeconds), .0001);
	FRotator Facing = VisualRotation;
	if (Velocity.SizeSquared() > 1.)
	{
		Facing = Velocity.Rotation();
		if (Medium == 0 || Medium == 3)
		{
			Facing.Pitch = 0.;
		}
		if (Medium == 2)
		{
			Facing.Roll = FMath::Clamp(FMath::FindDeltaAngleDegrees(VisualRotation.Yaw, Facing.Yaw) * .5, -25., 25.);
		}
	}
	VisualRotation = FMath::RInterpTo(VisualRotation, Facing, DeltaSeconds, Medium == 2 ? 3.f : 5.f);
	VisualRoot->SetWorldLocationAndRotation(VisualPosition, VisualRotation);
	if (Visual.Mesh)
	{
		// The replicated clock can arrive after this actor's BeginPlay.
		if (!WorldTime.IsValid())
		{
			for (TActorIterator<ACCLWorldEnvironmentState> It(GetWorld()); It; ++It)
			{
				WorldTime = *It;
				break;
			}
		}
		const bool bPaused = WorldTime.IsValid() && WorldTime->GetTime().TimeScale == 0.;
		const bool bMoving = Velocity.Size() > 3. || Medium == 1 || Medium == 2;
		auto* Animation = bMoving ? Visual.Move.Get() : Visual.Idle.Get();
		if (!Animation)
		{
			Animation = Visual.Move;
		}
		if (Animation && CurrentAnimation != Animation)
		{
			CreatureMesh->PlayAnimation(Animation, true);
			CurrentAnimation = Animation;
		}
		CreatureMesh->SetPlayRate(bPaused ? 0. : bMoving ? FMath::Clamp(Velocity.Size() / FMath::Max(Visual.GaitSpeedCmPerSecond, 1.), .4, 2.5) : 1.);
		return;
	}
	AnimationSeconds += DeltaSeconds;
	for (int32 Index = 1; Index < Shapes.Num(); ++Index)
	{
		const double Side = Index == 1 ? -1. : 1.;
		const double Phase = AnimationSeconds * (Medium == 3 ? 32. : Medium == 2 ? 12. : 8.);
		const double Swing = FMath::Sin(Phase) * (Medium >= 2 ? 35. : 15.);
		Shapes[Index]->SetRelativeRotation(Medium >= 2 ? FRotator(0., 0., Side * Swing) :
			Medium == 1 ? FRotator(0., Swing, 0.) : FRotator(Side * Swing, 0., 0.));
	}
}

void ACCLCreatureActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority() && Lease.IndividualId.IsValid() && GetWorld())
	{
		if (auto* Representations = GetWorld()->GetSubsystem<UCCLEcologyRepresentationSubsystem>())
		{
			Representations->OnActorEnded(Lease);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ACCLCreatureActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACCLCreatureActor, IndividualId);
	DOREPLIFETIME(ACCLCreatureActor, Medium);
}

void ACCLCreatureActor::Bind(const FKSIEcologyLease& NewLease, EKSIEcologyMovement Movement)
{
	if (!HasAuthority())
	{
		return;
	}

	Lease = NewLease;
	IndividualId = NewLease.IndividualId;
	Medium = uint8(Movement);
	OnRep_Medium();
	ForceNetUpdate();
}

void ACCLCreatureActor::OnRep_Medium()
{
	if (Shapes.Num() != 3)
	{
		return;
	}
	if (const auto* Config = GetWorld() ? ACCLWorldEnvironmentConfig::Find(GetWorld()) : nullptr; Config && Config->CreatureVisuals.IsValidIndex(Medium))
	{
		Visual = Config->CreatureVisuals[Medium];
		CreatureMesh->SetSkeletalMesh(Visual.Mesh);
		CreatureMesh->SetRelativeTransform(Visual.Transform);
		for (const auto& Shape : Shapes)
		{
			Shape->SetVisibility(!Visual.Mesh);
		}
	}

	const FVector BodyScale[] = {FVector(.8, .35, .5), FVector(.7, .18, .3), FVector(.45, .2, .25), FVector(.12, .08, .06)};
	const FVector AppendageScale[] = {FVector(.12, .2, .5), FVector(.25, .03, .25), FVector(.25, .65, .035), FVector(.02, .15, .02)};
	const uint8 Index = FMath::Min<uint8>(Medium, 3);
	auto* DisplayMaterial = LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/TestResources/Diagnostics/M_Creature%d.M_Creature%d"), Index, Index));
	for (const auto& Shape : Shapes)
	{
		if (DisplayMaterial)
		{
			Shape->SetMaterial(0, DisplayMaterial);
		}
	}
	const double Magnification = Index == 3 ? 3. : 1.;
	Shapes[0]->SetRelativeScale3D(BodyScale[Index] * Magnification);
	for (int32 ShapeIndex = 1; ShapeIndex < 3; ++ShapeIndex)
	{
		const double Side = ShapeIndex == 1 ? -1. : 1.;
		Shapes[ShapeIndex]->SetRelativeScale3D(AppendageScale[Index] * Magnification);
		Shapes[ShapeIndex]->SetRelativeLocation(FVector(Index == 1 ? -35. : 0., Side * (Index == 2 ? 35. : 12.), Index == 0 ? -10. : 0.));
	}
}
