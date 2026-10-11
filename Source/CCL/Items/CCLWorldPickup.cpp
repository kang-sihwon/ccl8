#include "CCLWorldPickup.h"
#include "Items/CCLInventoryFood.h"

#include "CCLInventoryComponent.h"
#include "CCLInventoryPersistenceSubsystem.h"
#include "CCLItemDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Environment/CCLWorldSimulationSubsystem.h"

ACCLWorldPickup::ACCLWorldPickup()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = .2f;
	FoodInventory = CreateDefaultSubobject<UCCLInventoryComponent>(TEXT("FoodInventory"));
	FoodInventory->Capacity = 1;
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
	DOREPLIFETIME(ACCLWorldPickup, PersistentId);
	DOREPLIFETIME(ACCLWorldPickup, Definition);
	DOREPLIFETIME(ACCLWorldPickup, Quantity);
	DOREPLIFETIME(ACCLWorldPickup, FoodView);
}

void ACCLWorldPickup::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(HasAuthority());
	RefreshVisual();
	if (HasAuthority())
	{
		UpdateFoodView();
	}
}

void ACCLWorldPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshVisual();
}

void ACCLWorldPickup::RefreshVisual()
{
	if (!FallbackMesh)
	{
		FallbackMesh = Mesh->GetStaticMesh();
	}

	const auto* Visual = Definition ? Definition->FindFragment<FCCLItemFragment_Visual>() : nullptr;
	auto* DefaultMesh = Definition && Definition->FindFragment<FCCLItemFragment_Perishable>()
		? LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")) : FallbackMesh.Get();
	Mesh->SetStaticMesh(Visual && Visual->DroppedMesh ? Visual->DroppedMesh.Get() : DefaultMesh);
}

bool ACCLWorldPickup::TryCollect(UCCLInventoryComponent* Inventory, AActor* Collector)
{
	const auto* Store = GetWorld()->GetSubsystem<UCCLInventoryPersistenceSubsystem>();
	if (!HasAuthority() || bCollected || (Store && Store->IsPreparing()) || !Inventory || !Inventory->GetOwner() ||
		!Inventory->GetOwner()->HasAuthority() || Inventory->IsTransactionOpen() || !IsValid(Collector) ||
		Collector->GetWorld() != GetWorld() || Inventory->GetWorld() != GetWorld() ||
		FVector::DistSquared(Collector->GetActorLocation(), GetActorLocation()) > FMath::Square(225.f))
	{
		return false;
	}
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CCLPickup), false, Collector);
	Params.AddIgnoredActor(this);
	FVector SightOrigin;
	FRotator SightRotation;
	Collector->GetActorEyesViewPoint(SightOrigin, SightRotation);
	if (GetWorld()->LineTraceSingleByChannel(Hit, SightOrigin, GetActorLocation(), ECC_Visibility,
											 Params))
	{
		return false;
	}
	const bool bFood = Definition && Definition->FindFragment<FCCLItemFragment_Perishable>();
	if ((bFood && !EnsureFood()) || (!bFood && bFoodInitialized))
	{
		return false;
	}
	bCollected = 1;
	const auto* Entry = bFood ? FoodInventory->FindSlot(0) : nullptr;
	const bool bAdded = bFood ? FCCLInventoryFood::TransferFood(*FoodInventory, Entry->Id, Entry->Quantity, Inventory)
							  : Inventory->Add(Definition, Quantity).IsValid();
	if (!bAdded)
	{
		bCollected = 0;
		return false;
	}
	Destroy();
	return true;
}

void ACCLWorldPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority() && !bCollected)
	{
		UpdateFoodView();
	}
}

bool ACCLWorldPickup::InitializeFood(UKSIItemDefinition* Item, const FCCLFoodState& Food, FGuid ItemId, FString& Error)
{
	const auto* Runtime = GetWorld() ? GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>() : nullptr;
	auto* Store = GetWorld() ? GetWorld()->GetSubsystem<UCCLInventoryPersistenceSubsystem>() : nullptr;
	if (!Store || Store->IsPreparing() || !HasAuthority() || bCollected || bFoodInitialized ||
		!FoodInventory->GetEntries().IsEmpty() || !Runtime || !Runtime->IsRunning() || !Item || !ItemId.IsValid() ||
		!Item->FindFragment<FCCLItemFragment_Perishable>() || Food.WorldId != Runtime->GetIdentity().WorldId ||
		Food.UpdatedWorldSeconds > Runtime->GetClock().GetWorldSeconds())
	{
		Error = TEXT("Pickup food initialization is unavailable or belongs to another world.");
		return false;
	}
	FKSIInventoryEntry Entry;
	Entry.Id = ItemId;
	Entry.Definition = Item;
	Entry.Quantity = CCLFood::Quantity(Food);
	Entry.Slot = 0;
	Entry.GetState<FCCLItemState>().Food = Food;
	if (!Food.IsPresent() || !FoodInventory->Restore({Entry}))
	{
		Error = TEXT("Invalid pickup food history.");
		return false;
	}
	if (!Store->BindPickup(*this, Error))
	{
		verify(FoodInventory->Restore({}));
		return false;
	}
	if (bCollected)
	{
		return false;
	}
	RefreshVisual();
	UpdateFoodView();
	ForceNetUpdate();
	return true;
}

bool ACCLWorldPickup::EnsureFood()
{
	if (!HasAuthority() || bCollected || !Definition)
	{
		return false;
	}
	if (bFoodInitialized)
	{
		const auto* Entry = FoodInventory->FindSlot(0);
		return Entry && Entry->Definition == Definition && Entry->Quantity == Quantity;
	}
	const auto* Fragment = Definition->FindFragment<FCCLItemFragment_Perishable>();
	const auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	if (!Fragment || !Runtime || !Runtime->IsRunning())
	{
		return false;
	}
	FCCLFoodState Food;
	FString Error;
	return CCLFood::Create(Runtime->GetIdentity().WorldId, FGuid::NewGuid(), Quantity,
						   Runtime->GetClock().GetWorldSeconds(), Fragment->Policy, {}, Food, Error) &&
		   InitializeFood(Definition, Food, FGuid::NewGuid(), Error);
}

void ACCLWorldPickup::UpdateFoodView()
{
	const auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	FCCLFoodQuality Quality;
	if (!Runtime || !Runtime->IsRunning() || !EnsureFood() ||
		!FCCLInventoryFood::GetFoodQuality(*FoodInventory, FoodInventory->FindSlot(0)->Id, Quality))
	{
		FoodView = {};
		return;
	}
	FoodView.Epoch = Runtime->GetEpoch();
	FoodView.WorldSeconds = Runtime->GetClock().GetWorldSeconds();
	FoodView.AgeSeconds = Quality.OldestAgeSeconds;
	FoodView.Freshness = Quality.Freshness;
	FoodView.Mold = Quality.Mold;
	FoodView.bReady = 1;
}

ACCLWorldPickup* ACCLWorldPickup::DropFood(UCCLInventoryComponent* Source, FGuid ItemId, int32 Count,
										   const FTransform& Transform, FString& Error)
{
	const auto* Entry = Source ? Source->Find(ItemId) : nullptr;
	if (!Source || !Source->GetOwner() || !Source->GetOwner()->HasAuthority() || Source->IsTransactionOpen() ||
		!Entry || !Entry->GetState<FCCLItemState>().Food.IsPresent() || Entry->Slot < 0 || Count < 1 || Count > Entry->Quantity ||
		Transform.ContainsNaN())
	{
		Error = TEXT("Invalid food drop request.");
		return nullptr;
	}
	auto* Store = Source->GetWorld()->GetSubsystem<UCCLInventoryPersistenceSubsystem>();
	if (!Store || Store->IsPreparing())
	{
		Error = TEXT("Cannot drop food while restoring the world.");
		return nullptr;
	}
	auto* Item = Entry->Definition.Get();
	auto* Pickup = Source->GetWorld()->SpawnActorDeferred<ACCLWorldPickup>(
		StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Pickup)
	{
		Error = TEXT("Food drop actor could not be spawned.");
		return nullptr;
	}
	Pickup->bCollected = 1;
	if (!Store->BindPickup(*Pickup, Error, true))
	{
		Pickup->Destroy();
		return nullptr;
	}
	FCCLFoodEnvironmentBinding Binding;
	const auto* Runtime = Source->GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	if (!Runtime ||
		!CCLFoodEnvironment::ResolveAt(Transform.GetLocation() / 100., Runtime->GetSurfaceSimulation(), Binding,
									   Error) ||
		!FCCLInventoryFood::ChangeFoodEnvironment(*Pickup->FoodInventory, Binding, Error))
	{
		Store->CancelEmptyPickup(*Pickup);
		Pickup->Destroy();
		return nullptr;
	}
	if (!FCCLInventoryFood::TransferFood(*Source, ItemId, Count, Pickup->FoodInventory))
	{
		Store->CancelEmptyPickup(*Pickup);
		Pickup->Destroy();
		Error = TEXT("Food drop transfer failed.");
		return nullptr;
	}
	const auto* Dropped = Pickup->FoodInventory->FindSlot(0);
	if (!Binding.IsDynamic())
	{
		verify(FCCLInventoryFood::ChangeFoodStorage(*Pickup->FoodInventory, Dropped->Id, Binding.Conditions));
	}
	Pickup->Definition = Item;
	Pickup->Quantity = Count;
	Pickup->bFoodInitialized = 1;
	Pickup->bCollected = 0;
	Pickup->FinishSpawning(Transform);
	Pickup->UpdateFoodView();
	return Pickup;
}
