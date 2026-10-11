#include "CCLEcologyExperiment.h"

#include "CCLExperimentDefinition.h"
#include "CCLEcologyRepresentationSubsystem.h"
#include "CCLWorldSimulationSubsystem.h"
#include "CCLWorldSnapshot.h"
#include "CCLTerrainNavigation.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ACCLEcologyExperiment::ACCLEcologyExperiment()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(4.);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = .25f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	for (int32 N = 0; N < 4; ++N)
	{
		const FVector Center = (HabitatCenter(N) - UCCLExperimentDefinition::ZoneCenter(6) / 100.) * 100.;
		for (int32 Edge = 0; Edge < 4; ++Edge)
		{
			auto* Mark = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Pen%dEdge%d"), N, Edge));
			Mark->SetupAttachment(RootComponent);
			Mark->SetStaticMesh(Cube.Object);
			Mark->SetRelativeLocation(Center + FVector(Edge < 2 ? (Edge ? 280. : -280.) : 0.,
													   Edge >= 2 ? (Edge == 3 ? 280. : -280.) : 0., 3.));
			Mark->SetRelativeScale3D(Edge < 2 ? FVector(.04, 5.6, .04) : FVector(5.6, .04, .04));
			Mark->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Mark->SetCanEverAffectNavigation(false);
		}
		auto* Food = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Food%d"), N));
		Food->SetupAttachment(RootComponent);
		Food->SetStaticMesh(Sphere.Object);
		Food->SetRelativeLocation(Center + FVector(160., 0., N == 1 ? 100. : N == 2 ? 250. : 35.));
		Food->SetRelativeScale3D(FVector(.25));
		Food->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Food->SetCanEverAffectNavigation(false);
		FoodMarkers.Add(Food);
		auto* Label = CreateDefaultSubobject<UTextRenderComponent>(*FString::Printf(TEXT("Label%d"), N));
		Label->SetupAttachment(RootComponent);
		Label->SetRelativeLocation(Center + FVector(0., -305., 45.));
		Label->SetRelativeRotation(FRotator(0., -90., 0.));
		Label->SetWorldSize(24.);
		Label->SetHorizontalAlignment(EHTA_Center);
		Labels.Add(Label);
	}
}

void ACCLEcologyExperiment::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		auto* Bounds = GetWorld()->SpawnActor<ACCLTerrainNavigationBounds>();
		const FVector Center = UCCLExperimentDefinition::ZoneCenter(6);
		Bounds->Configure(FBox(Center + FVector(-800., -800., -200.), Center + FVector(800., 800., 600.)));
		if (auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			Nav->GetDefaultNavDataInstance(FNavigationSystem::Create);
		}
	}
	if (GetNetMode() != NM_DedicatedServer)
	{
		auto* Font = LoadObject<UFont>(nullptr, TEXT("/Game/Environment/Fonts/F_EcologyLabel.F_EcologyLabel"));
		auto* Material =
			LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Materials/M_FoodLabel.M_FoodLabel"));
		for (auto& Label : Labels)
		{
			Label->SetFont(Font);
			Label->SetTextMaterial(Material);
			Label->SetTextRenderColor(FColor::White);
		}
	}
	UpdatePresentation();
}

void ACCLEcologyExperiment::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority() && bInitialized)
	{
		Reconcile();
		UpdateView();
	}
}

void ACCLEcologyExperiment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACCLEcologyExperiment, View);
}

FVector ACCLEcologyExperiment::HabitatCenter(int32 N)
{
	return UCCLExperimentDefinition::ZoneCenter(6) / 100. + FVector(N % 2 ? 3.5 : -3.5, N / 2 ? 3.5 : -3.5, 0.);
}

FString ACCLEcologyExperiment::MediumName(int32 N)
{
	const TCHAR* Names[] = {TEXT("지상 생물"), TEXT("물고기"), TEXT("새"), TEXT("곤충")};
	return N >= 0 && N < 4 ? Names[N] : TEXT("알 수 없음");
}

FCCLSurfaceGrid ACCLEcologyExperiment::Pond()
{
	FCCLSurfaceGrid G;
	G.RegionId = Id(1);
	G.OriginMeters = HabitatCenter(1) - FVector(2.8, 2.8, 0.);
	G.Size = FIntPoint(8, 8);
	G.SpacingMeters = .7;
	G.Cells.SetNum(64);
	G.Forcing.FlowConductance = 0.;
	G.Forcing.InfiltrationMetersPerWorldSecond = 0.;
	G.Forcing.DrainageMetersPerWorldSecond = 0.;
	G.Forcing.PhaseMetersPerDegreeWorldSecond = 0.;
	for (auto& C : G.Cells)
	{
		C.BedMeters = .15;
		C.CeilingMeters = 4.;
		C.WaterCubicMeters = 1.8 * .49;
	}
	G.Ledger.InitialCubicMeters = G.TotalCubicMeters();
	return G;
}

bool ACCLEcologyExperiment::MakeInitial(FGuid WorldId, double Time, FCCLEcologySimulation& Out, FString& Error)
{
	FCCLEcologySimulation E;
	if (!E.Initialize(WorldId, Time, Error))
	{
		return false;
	}
	for (uint32 N = 0; N < 4; ++N)
	{
		const FVector Center = HabitatCenter(N);
		const double Height = N == 1 ? 1. : N == 2 ? 2.5 : .35;
		FCCLEcologySpecies S;
		S.Id = Id(10 + N);
		S.Movement = ECCLEcologyMovement(N);
		S.AggregateDietMask = 1;
		FCCLEcologyHabitat H;
		H.Id = Id(20 + N);
		H.MinimumMeters = Center - FVector(2.8, 2.8, 0.);
		H.MaximumMeters = Center + FVector(2.8, 2.8, 4.);
		H.MovementMask = 1 << N;
		FCCLEcologyPopulation P;
		P.Id = Id(30 + N);
		P.SpeciesId = S.Id;
		P.HabitatId = H.Id;
		P.Cohorts.Add(Id(40 + N), {Id(40 + N), Time, 20});
		P.Cohorts[Id(40 + N)].Nutrition.Energy = .35;
		FCCLEcologyGroup G;
		G.Id = Id(50 + N);
		G.HabitatId = H.Id;
		G.AnchorMeters = Center + FVector(0., 0., Height);
		G.RadiusMeters = 2.;
		FCCLEcologyForage F;
		F.Id = Id(60 + N);
		F.HabitatId = H.Id;
		F.PositionMeters = Center + FVector(1.6, 0., Height);
		F.InitialUnits = F.AvailableUnits = F.CapacityUnits = 100.;
		F.RegrowthPerSecond = .03;
		if (!E.AddSpecies(S, Error) || !E.AddHabitat(H, Error) || !E.AddPopulation(P, Error) || !E.AddGroup(G, Error) ||
			!E.AddForage(F, Error))
		{
			return false;
		}
		for (uint32 Member = 0; Member < 3; ++Member)
		{
			const FGuid Individual = IndividualId(N, Member);
			FCCLEcologyLease Lease;
			FCCLEcologyBehavior B;
			B.Id = Individual;
			B.GroupId = G.Id;
			B.bUseWorldSpace = 1;
			B.Energy = .35;
			B.MetabolismPerSecond = .002;
			if (!E.Materialize(Id(200 + N * 3 + Member), P.Id, Id(40 + N), Individual, true, Error) ||
				!E.Acquire(Individual, ECCLEcologyRepresentation::Actor, Id(900), Lease, Error) ||
				!E.UpdatePosition(Lease, Center + FVector(-1.6, (double(Member) - 1.) * .8, Height), Error) ||
				!E.Suspend(Lease, Error) || !E.AddBehavior(B, Error))
			{
				return false;
			}
		}
	}
	Out = MoveTemp(E);
	return true;
}

bool ACCLEcologyExperiment::Initialize(bool bSeed, FCCLWorldSnapshot& Baseline, FString& Error)
{
	if (!HasAuthority())
	{
		return false;
	}
	FCCLEcologySimulation Initial;
	if (!MakeInitial(Baseline.Identity.WorldId, Baseline.Clock.WorldSeconds, Initial, Error) ||
		!Initial.Capture(Baseline.Ecology, Error))
	{
		return false;
	}
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	if (!bInitialized && bSeed)
	{
		FCCLEcologySimulation Current;
		if (!MakeInitial(Runtime->GetIdentity().WorldId, Runtime->GetClock().GetWorldSeconds(), Current, Error) ||
			!Runtime->SeedEcology(Current, Error))
		{
			return false;
		}
	}
	bInitialized = 1;
	Reconcile();
	UpdateView();
	return true;
}

void ACCLEcologyExperiment::Reconcile()
{
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	auto* Reps = GetWorld()->GetSubsystem<UCCLEcologyRepresentationSubsystem>();
	if (!Runtime || !Runtime->IsRunning() || !Reps)
	{
		return;
	}
	if (SeenEpoch != Runtime->GetEpoch())
	{
		SeenEpoch = Runtime->GetEpoch();
		Desired = ECCLEcologyRepresentation::Actor;
	}
	const auto& E = Runtime->GetEcologySimulation();
	FString Error;
	for (uint32 N = 0; N < 4; ++N)
	{
		for (uint32 M = 0; M < 3; ++M)
		{
			const auto IdValue = IndividualId(N, M);
			const auto* I = E.FindIndividual(IdValue);
			if (!I || I->State != ECCLEcologyLifeState::Alive)
			{
				continue;
			}
			const auto* Entry = Reps->Find(IdValue);
			if ((Desired == ECCLEcologyRepresentation::Dormant && !Entry) ||
				(Entry && E.IsLeaseCurrent(Entry->Lease) && Entry->Lease.Representation == Desired))
			{
				continue;
			}
			if (!Reps->SetRepresentation(IdValue, Desired, Error))
			{
				return;
			}
		}
	}
}

bool ACCLEcologyExperiment::Execute(ECCLExperimentAction Action, FString& Error)
{
	if (!HasAuthority() || !bInitialized)
	{
		Error = TEXT("서버의 생태 실험 준비를 기다려 줘.");
		return false;
	}
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	if (Action >= ECCLExperimentAction::EcologyFeedGround && Action <= ECCLExperimentAction::EcologyFeedInsect)
	{
		const int32 N = int32(Action) - int32(ECCLExperimentAction::EcologyFeedGround);
		const auto* Base = Runtime->GetEcologySimulation().GetForage().Find(Id(60 + N));
		if (!Base)
		{
			Error = TEXT("이전 저장에 생태 구역이 없다. 실험 초기화로 기본 배치를 만들 수 있다.");
			return false;
		}
		FCCLEcologyForage F;
		F.Id = FGuid::NewGuid();
		F.HabitatId = Base->HabitatId;
		F.PositionMeters = Base->PositionMeters;
		if (!Runtime->AddEcologyForage(F, Error))
		{
			return false;
		}
	}
	else if (Action == ECCLExperimentAction::EcologyActors)
	{
		Desired = ECCLEcologyRepresentation::Actor;
	}
	else if (Action == ECCLExperimentAction::EcologyMass)
	{
		Desired = ECCLEcologyRepresentation::Mass;
	}
	else if (Action == ECCLExperimentAction::EcologyHidden)
	{
		Desired = ECCLEcologyRepresentation::Dormant;
	}
	else
	{
		Error = TEXT("지원하지 않는 생태 조작이다.");
		return false;
	}
	Reconcile();
	UpdateView();
	ForceNetUpdate();
	return true;
}

bool ACCLEcologyExperiment::CheckState(FString& Error) const
{
	const auto& E = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>()->GetEcologySimulation();
	if (!E.Validate(Error))
	{
		return false;
	}
	for (uint32 N = 0; N < 4; ++N)
	{
		if (!E.GetPopulations().Contains(Id(30 + N)))
		{
			Error = TEXT("생태 개체군이 없다. 실험 초기화 후 다시 확인해 줘.");
			return false;
		}
		int64 Reserved = 0;
		for (const auto& Pair : E.GetIndividuals())
		{
			Reserved += Pair.Value.PopulationId == Id(30 + N) && Pair.Value.State == ECCLEcologyLifeState::Alive;
		}
		if (E.LivingCount(Id(30 + N)) != E.AggregateCount(Id(30 + N)) + Reserved)
		{
			Error = TEXT("생태 수량 보존 불일치");
			return false;
		}
	}
	return true;
}

void ACCLEcologyExperiment::UpdateView()
{
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	auto* Reps = GetWorld()->GetSubsystem<UCCLEcologyRepresentationSubsystem>();
	const auto& E = Runtime->GetEcologySimulation();
	View.WorldId = E.GetWorldId();
	View.Epoch = Runtime->GetEpoch();
	View.WorldSeconds = E.GetWorldSeconds();
	View.Rows.SetNum(4);
	for (uint32 N = 0; N < 4; ++N)
	{
		auto& Row = View.Rows[N];
		Row = {};
		Row.Living = E.LivingCount(Id(30 + N));
		Row.Aggregate = E.AggregateCount(Id(30 + N));
		if (const auto* Population = E.GetPopulations().Find(Id(30 + N)))
		{
			Row.AggregateConsumed = Population->AggregateConsumedUnits;
			for (const auto& Pair : Population->Cohorts)
			{
				Row.AggregateEnergy += Pair.Value.Nutrition.Energy * double(Pair.Value.Count);
			}
			if (Row.Aggregate > 0)
			{
				Row.AggregateEnergy /= double(Row.Aggregate);
			}
		}
		for (const auto& Pair : E.GetIndividuals())
		{
			const auto& I = Pair.Value;
			if (I.PopulationId != Id(30 + N) || I.State != ECCLEcologyLifeState::Alive)
			{
				continue;
			}
			++Row.Reserved;
			if (const auto* Entry = Reps->Find(I.Id))
			{
				Row.Actors += Entry->Lease.Representation == ECCLEcologyRepresentation::Actor;
				Row.Mass += Entry->Lease.Representation == ECCLEcologyRepresentation::Mass;
			}
			if (const auto* B = E.GetBehaviors().Find(I.Id))
			{
				Row.Energy += B->Energy;
				Row.Consumed += B->ConsumedUnits;
				Row.Blocked += B->Activity == ECCLEcologyActivity::Blocked;
			}
		}
		if (Row.Reserved)
		{
			Row.Energy /= Row.Reserved;
		}
		for (const auto& Pair : E.GetForage())
		{
			if (Pair.Value.HabitatId == Id(20 + N))
			{
				Row.Food += Pair.Value.AvailableUnits;
			}
		}
	}
	UpdatePresentation();
}

void ACCLEcologyExperiment::UpdatePresentation()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	for (int32 N = 0; N < 4; ++N)
	{
		const auto* Row = View.Rows.IsValidIndex(N) ? &View.Rows[N] : nullptr;
		Labels[N]->SetText(FText::FromString(FString::Printf(TEXT("%s  %lld마리\n먹이 %.1f · 개체 %.0f%% · 집계 %.0f%%"),
															 *MediumName(N), Row ? Row->Living : 0,
															 Row ? Row->Food : 0., Row ? Row->Energy * 100. : 0., Row ? Row->AggregateEnergy * 100. : 0.)));
		FoodMarkers[N]->SetVisibility(Row && Row->Food > 0.);
	}
}

void ACCLEcologyExperiment::OnRep_View()
{
	UpdatePresentation();
}

ACCLEcologyExperiment* ACCLEcologyExperiment::Find(const UWorld* World)
{
	for (TActorIterator<ACCLEcologyExperiment> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}
