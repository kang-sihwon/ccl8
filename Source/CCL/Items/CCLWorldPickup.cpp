#include "CCLWorldPickup.h"

#include "CCLInventoryComponent.h"
#include "CCLItemDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

ACCLWorldPickup::ACCLWorldPickup()
{
	bReplicates = true;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(0.4f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Game/Combat/SM_ArenaBlock.SM_ArenaBlock"));
	if (Cube.Succeeded())
	{
		Mesh->SetStaticMesh(Cube.Object);
	}
}

void ACCLWorldPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACCLWorldPickup, Definition);
	DOREPLIFETIME(ACCLWorldPickup, Quantity);
}

bool ACCLWorldPickup::TryCollect(UCCLInventoryComponent* Inventory, AActor* Collector)
{
	if (!HasAuthority() || bCollected || !Inventory || !Inventory->GetOwner()->HasAuthority() || !IsValid(Collector) ||
		Collector->GetWorld() != GetWorld() || Inventory->GetWorld() != GetWorld() ||
		FVector::DistSquared(Collector->GetActorLocation(), GetActorLocation()) > FMath::Square(225.f))
	{
		return false;
	}
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CCLPickup), false, Collector);
	Params.AddIgnoredActor(this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Collector->GetActorLocation(), GetActorLocation(), ECC_Visibility, Params))
	{
		return false;
	}
	bCollected = 1;
	if (!Inventory->Add(Definition, Quantity).IsValid())
	{
		bCollected = 0;
		return false;
	}
	Destroy();
	return true;
}
