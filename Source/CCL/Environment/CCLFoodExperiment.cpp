#include "CCLFoodExperiment.h"
#include "Items/CCLInventoryFood.h"

#include "CCLExperimentDefinition.h"
#include "CCLWorldEnvironmentConfig.h"
#include "CCLExperimentDirector.h"
#include "CCLWorldSimulationSubsystem.h"
#include "Items/CCLInventoryComponent.h"
#include "Items/CCLInventorySnapshot.h"
#include "Items/CCLWorldPickup.h"
#include "Items/CCLItemDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
FCCLFoodStorage Storage(int32 Index)
{
	FCCLFoodStorage Value;
	Value.TemperatureCelsius = Index == 1 ? -5. : 20.;
	Value.RelativeHumidity = 1.;
	Value.SpoilageMultiplier = Index == 2 ? 0.2 : 1.;
	Value.MoldExposure = Index == 2 ? 0. : 1.;
	return Value;
}
} // namespace

ACCLFoodExperiment::ACCLFoodExperiment()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(5.);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.2f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Bread(TEXT("/Game/TestResources/Fishermans_Cabin/Meshes/Bread/SM_Bread_01.SM_Bread_01"));
	for (int32 Index = 0; Index < 3; ++Index)
	{
		auto* Inventory = CreateDefaultSubobject<UCCLInventoryComponent>(*FString::Printf(TEXT("Storage%d"), Index));
		Inventory->Capacity = 4;
		Inventories.Add(Inventory);
		const float X = (1 - Index) * 260.f;
		auto* Shelf = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Shelf%d"), Index));
		Shelf->SetupAttachment(RootComponent);
		Shelf->SetStaticMesh(Cube.Object);
		Shelf->SetRelativeLocation(FVector(X, 0, 35));
		Shelf->SetRelativeScale3D(FVector(1.8, 1.1, 0.2));
		Shelf->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Shelf->SetCanEverAffectNavigation(false);
		auto* Sample = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Sample%d"), Index));
		Sample->SetupAttachment(RootComponent);
		Sample->SetStaticMesh(Sphere.Object);
		Sample->SetRelativeLocation(FVector(X, 0, 65));
		Sample->SetRelativeScale3D(FVector(0.8, 0.6, 0.4));
		if (Bread.Succeeded())
		{
			const auto Bounds = Bread.Object->GetBounds();
			const FVector Scale(40. / FMath::Max(Bounds.BoxExtent.X, 1.), 30. / FMath::Max(Bounds.BoxExtent.Y, 1.), 20. / FMath::Max(Bounds.BoxExtent.Z, 1.));
			Sample->SetStaticMesh(Bread.Object);
			Sample->SetRelativeLocation(FVector(X, 0, 65) - Bounds.Origin * Scale);
			Sample->SetRelativeScale3D(Scale);
		}
		Sample->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Sample->SetCanEverAffectNavigation(false);
		Samples.Add(Sample);
		auto* Label = CreateDefaultSubobject<UTextRenderComponent>(*FString::Printf(TEXT("Label%d"), Index));
		Label->SetupAttachment(RootComponent);
		Label->SetRelativeLocation(FVector(X, -180, 35));
		Label->SetRelativeRotation(FRotator(0, -90, 0));
		Label->SetWorldSize(28);
		Label->SetHorizontalAlignment(EHTA_Center);
		Labels.Add(Label);
	}
}

void ACCLFoodExperiment::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() != NM_DedicatedServer)
	{
		auto* Font = LoadObject<UFont>(nullptr, TEXT("/Game/Environment/Fonts/F_FoodLabel.F_FoodLabel"));
		auto* FoodMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/TestResources/Diagnostics/M_FoodProgress.M_FoodProgress"));
		for (auto& Sample : Samples)
		{
			auto* Material = UMaterialInstanceDynamic::Create(FoodMaterial, this);
			SampleMaterials.Add(Material);
			if (Material) { Sample->SetMaterial(0, Material); }
		}
		for (auto& Label : Labels)
		{
			Label->SetFont(Font);
			Label->SetTextMaterial(
				LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Materials/M_FoodLabel.M_FoodLabel")));
			Label->SetTextRenderColor(FColor::White);
		}
	}
	UpdatePresentation();
}

void ACCLFoodExperiment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACCLFoodExperiment, View);
}

UKSIItemDefinition* ACCLFoodExperiment::SampleDefinition() const
{
	return LoadObject<UKSIItemDefinition>(nullptr, TEXT("/Game/Environment/Experiments/DA_FoodSample.DA_FoodSample"));
}

bool ACCLFoodExperiment::MakeInitial(FGuid WorldId, double WorldSeconds, TArray<FCCLInventorySnapshot>& Records,
									 FString& Error) const
{
	auto* Definition = SampleDefinition();
	const auto* Perishable = Definition ? Definition->FindFragment<FCCLItemFragment_Perishable>() : nullptr;
	if (!Perishable)
	{
		Error = TEXT("실험용 음식 정의를 불러올 수 없다.");
		return false;
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		auto& Record = Records.AddDefaulted_GetRef();
		Record.ContainerId = ContainerId(Index);
		Record.Capacity = 4;
		Record.Environment.Conditions = Storage(Index);
		const auto* Config = ACCLWorldEnvironmentConfig::Find(GetWorld());
		if (Config && Config->LaboratoryZone == 7 && Index != Config->LaboratoryVariant) { continue; }
		auto& Item = Record.Items.AddDefaulted_GetRef();
		Item.Id = FGuid(7071, 7074, 1, Index + 1);
		Item.Definition = Definition->GetPathName();
		Item.Quantity = Config && Config->LaboratoryZone == 7 ? Config->SampleCount() : 4;
		Item.Slot = 0;
		FCCLFoodState Food;
		if (!CCLFood::Create(WorldId, FGuid(7071, 7075, 1, Index + 1), Item.Quantity, WorldSeconds, Perishable->Policy,
							 Storage(Index), Food, Error) ||
			!CCLFood::Encode(Food, Item.Food, Error))
		{
			return false;
		}
	}
	return true;
}

bool ACCLFoodExperiment::Initialize(bool bSeed, FCCLWorldSnapshot& Baseline, FString& Error)
{
	if (!HasAuthority())
	{
		return false;
	}
	TArray<FCCLInventorySnapshot> Initial;
	if (!MakeInitial(Baseline.Identity.WorldId, Baseline.Clock.WorldSeconds, Initial, Error))
	{
		return false;
	}
	if (!bInitialized)
	{
		auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
		TArray<FCCLInventorySnapshot> Current;
		if (bSeed &&
			!MakeInitial(Runtime->GetIdentity().WorldId, Runtime->GetClock().GetWorldSeconds(), Current, Error))
		{
			return false;
		}
		for (int32 Index = 0; Index < 3; ++Index)
		{
			if (!Inventories[Index]->BindPersistence(ContainerId(Index), Error))
			{
				return false;
			}
			if (bSeed && !FCCLInventoryFood::ChangeFoodEnvironment(*Inventories[Index], Current[Index].Environment, Error)) { return false; }
			if (bSeed && !Current[Index].Items.IsEmpty())
			{
				FKSIInventoryEntry Entry;
				Entry.Id = Current[Index].Items[0].Id;
				Entry.Definition = SampleDefinition();
				Entry.Quantity = Current[Index].Items[0].Quantity;
				Entry.Slot = 0;
				if (!CCLFood::Decode(Current[Index].Items[0].Food, Entry.GetState<FCCLItemState>().Food, Error) ||
					!Inventories[Index]->Restore({Entry}))
				{
					return false;
				}
			}
		}
		bInitialized = 1;
	}
	TArray<FCCLInventorySnapshot> Stored;
	if (!CCLInventorySnapshot::ReadWorld(Baseline, Stored, Error))
	{
		return false;
	}
	Stored.RemoveAll(
		[](const auto& R)
		{
			return R.ContainerId == ContainerId(0) || R.ContainerId == ContainerId(1) ||
				   R.ContainerId == ContainerId(2);
		});
	Stored.Append(Initial);
	if (!CCLInventorySnapshot::WriteWorld(Baseline, Stored, Error))
	{
		return false;
	}
	UpdateView();
	return true;
}

bool ACCLFoodExperiment::Execute(ECCLExperimentAction Action, FString& Error)
{
	if (!HasAuthority() || !bInitialized)
	{
		Error = TEXT("서버의 음식 실험 준비를 기다려 줘.");
		return false;
	}
	const auto* Config = ACCLWorldEnvironmentConfig::Find(GetWorld());
	const int32 SelectedStorage = Config && Config->LaboratoryZone == 7 ? Config->LaboratoryVariant : 0;
	auto* Open = Inventories[SelectedStorage].Get();
	auto* Cold = Inventories[1].Get();
	if (Action == ECCLExperimentAction::FoodDry || Action == ECCLExperimentAction::FoodWarmHumid || Action == ECCLExperimentAction::FoodCool)
	{
		FCCLFoodEnvironmentBinding Binding;
		Binding.Conditions = Storage(SelectedStorage);
		Binding.Conditions.TemperatureCelsius = Action == ECCLExperimentAction::FoodCool ? -5. : Action == ECCLExperimentAction::FoodWarmHumid ? 35. : 20.;
		Binding.Conditions.RelativeHumidity = Action == ECCLExperimentAction::FoodDry ? 0.2 : 1.;
		if (!FCCLInventoryFood::ChangeFoodEnvironment(*Open, Binding, Error)) { return false; }
	}
	else if (Action == ECCLExperimentAction::FoodWeather || Action == ECCLExperimentAction::FoodIndoor)
	{
		const int32 Index = Action == ECCLExperimentAction::FoodWeather ? 0 : 2;
		auto* Inventory = Inventories[Index].Get();
		FCCLFoodEnvironmentBinding Binding;
		Binding.Conditions = Storage(Index);
		if (!Inventory->GetFoodEnvironment().IsDynamic())
		{
			Binding.Source = Index == 0 ? ECCLFoodEnvironment::WeatherRegion : ECCLFoodEnvironment::AirSpace;
			Binding.SourceId =
				Index == 0 ? ACCLExperimentDirector::WeatherRegionId() : ACCLExperimentDirector::FireSpaceId();
		}
		if (!FCCLInventoryFood::ChangeFoodEnvironment(*Inventory, Binding, Error))
		{
			return false;
		}
	}
	else if (Action == ECCLExperimentAction::FoodDrop)
	{
		if (FindGroundSample() || Open->GetEntries().IsEmpty() ||
			!ACCLWorldPickup::DropFood(Open, Open->GetEntries()[0].Id, 1,
									   FTransform(GetGroundPosition()), Error))
		{
			Error = TEXT("바닥 음식을 먼저 회수하거나 노출 보관함에 음식을 넣어 줘.");
			return false;
		}
	}
	else if (Action == ECCLExperimentAction::FoodCollect)
	{
		auto* Pickup = FindGroundSample();
		if (!Pickup || !Pickup->TryCollect(Open, this))
		{
			Error = TEXT("회수할 음식이 없거나 경로·보관 공간이 부족하다.");
			return false;
		}
	}
	else if (Action == ECCLExperimentAction::FoodAdd)
	{
		const auto Conditions = Open->GetEntries().IsEmpty() ? Open->GetFoodEnvironment().Conditions : Open->GetEntries()[0].GetState<FCCLItemState>().Food.Storage;
		const FGuid Added = Open->Add(SampleDefinition(), 2);
		if (!Added.IsValid() ||
			(!Open->GetFoodEnvironment().IsDynamic() && !FCCLInventoryFood::ChangeFoodStorage(*Open, Added, Conditions)))
		{
			Error = TEXT("새 음식을 넣을 공간이 없다.");
			return false;
		}
	}
	else if (Action == ECCLExperimentAction::FoodMoveCold)
	{
		const auto TargetStorage = Cold->GetEntries().IsEmpty() ? Storage(1) : Cold->GetEntries()[0].GetState<FCCLItemState>().Food.Storage;
		if (Open->GetEntries().IsEmpty() || !FCCLInventoryFood::TransferFood(*Open, Open->GetEntries()[0].Id, 1, Cold))
		{
			Error = TEXT("노출 보관함에 음식이 없거나 냉각 보관함이 가득 찼다.");
			return false;
		}
		for (const auto& Entry : TArray<FKSIInventoryEntry>(Cold->GetEntries()))
		{
			if (!FCCLInventoryFood::ChangeFoodStorage(*Cold, Entry.Id, TargetStorage))
			{
				return false;
			}
		}
	}
	else if (Action == ECCLExperimentAction::FoodThaw)
	{
		if (Cold->GetEntries().IsEmpty())
		{
			Error = TEXT("냉각 보관함이 비어 있다.");
			return false;
		}
		const bool bThaw = Cold->GetEntries()[0].GetState<FCCLItemState>().Food.Storage.TemperatureCelsius <= 0.;
		auto Binding = Cold->GetFoodEnvironment();
		Binding.Conditions.TemperatureCelsius = bThaw ? 20. : -5.;
		if (!FCCLInventoryFood::ChangeFoodEnvironment(*Cold, Binding, Error)) { return false; }
	}
	else
	{
		return false;
	}
	UpdateView();
	UpdatePresentation();
	ForceNetUpdate();
	return true;
}

void ACCLFoodExperiment::UpdateView()
{
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	if (!bInitialized || !Runtime || !Runtime->IsRunning())
	{
		return;
	}
	FCCLFoodExperimentView Candidate;
	Candidate.WorldId = Runtime->GetIdentity().WorldId;
	Candidate.Epoch = Runtime->GetEpoch();
	Candidate.WorldSeconds = Runtime->GetClock().GetWorldSeconds();
	for (int32 Index = 0; Index < 3; ++Index)
	{
		auto& Row = Candidate.Rows.AddDefaulted_GetRef();
		const auto& Entries = Inventories[Index]->GetEntries();
		const auto& Binding = Inventories[Index]->GetFoodEnvironment();
		Row.EnvironmentSource = uint8(Binding.Source);
		const auto Conditions = Binding.IsDynamic() ? Binding.Conditions
								: Entries.IsEmpty() ? Binding.Conditions
													: Entries[0].GetState<FCCLItemState>().Food.Storage;
		Row.Temperature = Conditions.TemperatureCelsius;
		Row.Humidity = Conditions.RelativeHumidity;
		Row.SpoilageMultiplier = Conditions.SpoilageMultiplier;
		Row.MoldExposure = Conditions.MoldExposure;
		double FreshSum = 0., MoldSum = 0.;
		for (const auto& Entry : Entries)
		{
			FCCLFoodQuality Quality;
			if (!FCCLInventoryFood::GetFoodQuality(*Inventories[Index], Entry.Id, Quality))
			{
				return;
			}
			Row.Quantity += Entry.Quantity;
			Row.Lots += Entry.GetState<FCCLItemState>().Food.Lots.Num();
			Row.Spoiled += Quality.SpoiledQuantity;
			Row.Moldy += Quality.MoldyQuantity;
			Row.AgeSeconds = FMath::Max(Row.AgeSeconds, Quality.OldestAgeSeconds);
			FreshSum += Quality.Freshness * Entry.Quantity;
			MoldSum += Quality.Mold * Entry.Quantity;
		}
		Row.Freshness = Row.Quantity ? FreshSum / Row.Quantity : 1.;
		Row.Mold = Row.Quantity ? MoldSum / Row.Quantity : 0.;
	}
	if (const auto* Ground = FindGroundSample())
	{
		Candidate.GroundQuantity = Ground->Quantity;
	}
	View = MoveTemp(Candidate);
}

void ACCLFoodExperiment::UpdatePresentation()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const bool bReady = View.Rows.IsValidIndex(Index);
		Samples[Index]->SetVisibility(bReady && View.Rows[Index].Quantity > 0);
		const auto* Config = ACCLWorldEnvironmentConfig::Find(GetWorld());
		const bool bVisible = !Config || Config->LaboratoryZone < 0 || Config->LaboratoryVariant == Index || (bReady && View.Rows[Index].Quantity > 0);
		Labels[Index]->SetVisibility(bReady && bVisible);
		for (auto* Shape : TInlineComponentArray<UStaticMeshComponent*>(this))
		{
			if (Shape->GetName() == FString::Printf(TEXT("Shelf%d"), Index)) { Shape->SetVisibility(bVisible); }
		}
		if (!bReady)
		{
			continue;
		}
		const auto& Row = View.Rows[Index];
		if (SampleMaterials.IsValidIndex(Index) && SampleMaterials[Index])
		{
			const auto Color = FMath::Lerp(FLinearColor(0.25f, 0.2f, 0.13f), FLinearColor::White, float(Row.Freshness));
			SampleMaterials[Index]->SetVectorParameterValue(TEXT("FoodColor"), Color);
			SampleMaterials[Index]->SetScalarParameterValue(TEXT("Mold"), float(Row.Mold));
		}
		Labels[Index]->SetText(
			FText::FromString(FString::Printf(TEXT("%s · %d개 · %.0f초\n%.0f°C / 습도 %.0f%%\n신선 %.0f%% · 곰팡이 %.0f%%"), *StorageName(Index),
				Row.Quantity, Row.AgeSeconds, Row.Temperature, Row.Humidity * 100., Row.Freshness * 100., Row.Mold * 100.)));
	}
}

void ACCLFoodExperiment::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority())
	{
		UpdateView();
	}
	UpdatePresentation();
}

UCCLInventoryComponent* ACCLFoodExperiment::GetInventory(int32 Index) const
{
	return Inventories.IsValidIndex(Index) ? Inventories[Index].Get() : nullptr;
}

FString ACCLFoodExperiment::StorageName(int32 Index)
{
	return Index == 0 ? TEXT("노출") : Index == 1 ? TEXT("냉각") : TEXT("밀폐");
}

int32 ACCLFoodExperiment::GetRenderedCount() const
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return 0;
	}
	int32 Count = 0;
	for (const auto& Sample : Samples)
	{
		Count += Sample->IsVisible() ? 1 : 0;
	}
	return Count;
}

ACCLFoodExperiment* ACCLFoodExperiment::Find(const UWorld* World)
{
	for (TActorIterator<ACCLFoodExperiment> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void ACCLFoodExperiment::OnRep_View()
{
	UpdatePresentation();
}

ACCLWorldPickup* ACCLFoodExperiment::FindGroundSample() const
{
	ACCLWorldPickup* Result = nullptr;
	for (TActorIterator<ACCLWorldPickup> It(GetWorld()); It; ++It)
	{
		if (!It->IsActorBeingDestroyed() && It->Definition == SampleDefinition() &&
			FVector::DistSquared(It->GetActorLocation(), GetActorLocation()) <= FMath::Square(225.))
		{
			if (!Result || It->PersistentId < Result->PersistentId)
			{
				Result = *It;
			}
		}
	}
	return Result;
}


void ACCLFoodExperiment::GetActorEyesViewPoint(FVector& Location, FRotator& Rotation) const
{
	Location = GetActorLocation() + FVector(0., 0., 80.);
	Rotation = GetActorRotation();
}
