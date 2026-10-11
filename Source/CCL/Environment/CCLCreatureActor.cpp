#include "CCLCreatureActor.h"

#include "CCLEcologyRepresentationSubsystem.h"
#include "Components/StaticMeshComponent.h"
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
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	for (int32 Index = 0; Index < 3; ++Index)
	{
		auto* Shape = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Shape%d"), Index));
		Shape->SetupAttachment(SceneRoot);
		Shape->SetStaticMesh(Index == 0 ? Sphere.Object : Cube.Object);
		Shape->SetMobility(EComponentMobility::Movable);
		Shape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Shapes.Add(Shape);
	}

	OnRep_Medium();
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

	const FVector BodyScale[] = {FVector(.8, .35, .5), FVector(.7, .18, .3), FVector(.45, .2, .25), FVector(.12, .08, .06)};
	const FVector AppendageScale[] = {FVector(.12, .2, .5), FVector(.25, .03, .25), FVector(.25, .65, .035), FVector(.02, .15, .02)};
	const uint8 Index = FMath::Min<uint8>(Medium, 3);
	Shapes[0]->SetRelativeScale3D(BodyScale[Index]);
	for (int32 ShapeIndex = 1; ShapeIndex < 3; ++ShapeIndex)
	{
		const double Side = ShapeIndex == 1 ? -1. : 1.;
		Shapes[ShapeIndex]->SetRelativeScale3D(AppendageScale[Index]);
		Shapes[ShapeIndex]->SetRelativeLocation(FVector(Index == 1 ? -35. : 0., Side * (Index == 2 ? 35. : 12.), Index == 0 ? -30. : 0.));
	}
}
