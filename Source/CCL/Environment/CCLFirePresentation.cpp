#include "CCLFirePresentation.h"

#include "CCLSurfaceReplication.h"
#include "CCLWeatherPresentation.h"
#include "CCLWorldEnvironmentState.h"
#include "EngineUtils.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

ACCLFirePresentation::ACCLFirePresentation()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Fuel = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Fuel"));
	Char = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Char"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	for (auto* Mesh : {Fuel.Get(), Char.Get()})
	{
		Mesh->SetupAttachment(RootComponent);
		Mesh->SetStaticMesh(Cube.Object);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
	}

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.2f;
}

void ACCLFirePresentation::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetNetMode() == NM_DedicatedServer || !FlameSystem || !SmokeSystem)
	{
		return;
	}

	const auto* Model = UCCLSurfaceReplication::View(GetWorld());
	const auto* PC = GetWorld()->GetFirstPlayerController();
	if (!Model || !PC)
	{
		return;
	}

	FVector Camera;
	FRotator Rotation;
	PC->GetPlayerViewPoint(Camera, Rotation);
	Fuel->SetMaterial(0, FuelMaterial);
	Char->SetMaterial(0, CharMaterial);
	Fuel->ClearInstances();
	Char->ClearInstances();
	int32 Emitter = 0;
	int32 LightIndex = 0;
	TArray<FGuid> Ids;
	Model->GetRegions().GetKeys(Ids);
	Ids.Sort();
	for (FGuid Id : Ids)
	{
		const auto& G = Model->GetRegions()[Id];
		if (G.Fire.Cells.IsEmpty() || FVector::DistSquared2D(Camera / 100., G.OriginMeters) > FMath::Square(45.))
		{
			continue;
		}

		int32 BurningCells = 0;
		for (const auto& Cell : G.Fire.Cells)
		{
			BurningCells += Cell.bBurning ? 1 : 0;
		}

		if (BurningCells > 0)
		{
			while (Lights.Num() <= LightIndex)
			{
				auto* Light = NewObject<UPointLightComponent>(this, NAME_None, RF_Transient);
				Light->SetupAttachment(RootComponent);
				Light->SetMobility(EComponentMobility::Movable);
				Light->SetAttenuationRadius(800.f);
				Light->SetLightColor(FLinearColor(1.f, 0.34f, 0.06f));
				Light->RegisterComponent();
				Lights.Add(Light);
			}

			auto* Light = Lights[LightIndex++].Get();
			Light->SetWorldLocation(FVector((G.OriginMeters.X + G.Size.X * G.SpacingMeters * 0.5) * 100.,
				(G.OriginMeters.Y + G.Size.Y * G.SpacingMeters * 0.5) * 100., G.Cells[0].BedMeters * 100. + 110.));
			Light->SetIntensity(FMath::Min(40000.f, BurningCells * 8000.f));
			Light->SetVisibility(true);
		}

		for (int32 I = 0; I < G.Fire.Cells.Num(); ++I)
		{
			const auto& C = G.Fire.Cells[I];
			const FVector Position = FKSISurfaceThermalContext::CellPosition(G, I) * 100.;
			const double Initial = C.FuelKg + C.BurnedKg;
			const float Remain = Initial > 0. ? float(C.FuelKg / Initial) : 0.f;
			if (Remain > 0. && G.Vegetation.Cells.IsEmpty())
			{
				Fuel->AddInstance(FTransform(FRotator(0., (I % 2) * 90., 0.), Position + FVector(0., 0., 8. * Remain), FVector(0.7, 0.45, 0.16 * Remain)), true);
			}

			if (C.BurnedKg > 0. && G.Vegetation.Cells.IsEmpty())
			{
				Char->AddInstance(FTransform(FRotator::ZeroRotator, Position + FVector(0., 0., 1.), FVector(0.85, 0.7, 0.02)), true);
			}

			if (C.bBurning && Emitter < 128)
			{
				UpdateEmitter(Emitter++, FlameSystem, Position + FVector(0., 0., 12.), 35.f, FVector(0., 0., 90.));
			}
			if (C.bBurning && Emitter < 128 && !Model->GetAirSimulation().FindSpaceAt(Position / 100.))
			{
				UpdateEmitter(Emitter++, SmokeSystem, Position + FVector(0., 0., 45.), C.bBurning ? 8.f : 0.f, FVector(15., 0., 65.));
			}
		}
	}

	for (int32 I = LightIndex; I < Lights.Num(); ++I)
	{
		Lights[I]->SetVisibility(false);
	}

	Model->GetAirSimulation().GetSpaces().GetKeys(Ids);
	Ids.Sort();
	for (FGuid Id : Ids)
	{
		const auto& Room = Model->GetAirSimulation().GetSpaces()[Id];
		const FVector Center = (Room.MinimumMeters + Room.MaximumMeters) * 50.;
		if (Emitter >= 128 || FVector::DistSquared(Camera, Center) > FMath::Square(4500.))
		{
			continue;
		}

		const float Rate = float(FMath::Clamp(Room.Contents.SmokeKg / Room.VolumeCubicMeters * 500000., 0., 100.));
		UpdateEmitter(Emitter++, SmokeSystem, Center, Rate, FVector(0., 0., 12.));
		for (TActorIterator<ACCLWorldEnvironmentState> State(GetWorld()); State; ++State)
		{
			const auto& View = State->GetTime().Environment;
			for (const auto& Opening : View.Openings)
			{
				if (Emitter >= 128 || Opening.OpenFraction <= 0. ||
					!((Opening.SpaceA == Id && !Opening.SpaceB.IsValid()) || (Opening.SpaceB == Id && !Opening.SpaceA.IsValid())))
				{
					continue;
				}
				const auto* Patch = View.Surfaces.FindByPredicate([&](const auto& P) { return P.SurfaceId == Opening.SurfaceId; });
				if (!Patch)
				{
					continue;
				}
				const auto* Region = Model->FindRegion(Room.ClimateRegionId);
				const double Wind = Region ? Region->Weather.WindMetersPerSecond : 0.;
				const FVector Mouth = Patch->CenterMeters + Patch->TangentU * Opening.CenterUV.X + Patch->TangentV * Opening.CenterUV.Y;
				const FVector Outward = FVector::DotProduct(Patch->Normal, Mouth * 100. - Center) >= 0. ? Patch->Normal : -Patch->Normal;
				UpdateEmitter(Emitter++, SmokeSystem, Mouth * 100. + Outward * 20.,
					Rate * Opening.OpenFraction, Outward * (25. + Wind * 15.) + FVector(0., 0., 20.));
			}
			break;
		}
	}

	for (int32 I = Emitter; I < Emitters.Num(); ++I)
	{
		Emitters[I]->DeactivateImmediate();
	}
}

void ACCLFirePresentation::UpdateEmitter(int32 Index, UNiagaraSystem* Asset, const FVector& Position, float Rate, const FVector& Velocity)
{
	while (Emitters.Num() <= Index)
	{
		auto* Component = NewObject<UNiagaraComponent>(this, NAME_None, RF_Transient);
		Component->SetupAttachment(RootComponent);
		Component->SetAutoActivate(false);
		Component->RegisterComponent();
		Emitters.Add(Component);
	}

	auto* Component = Emitters[Index].Get();
	if (Component->GetAsset() != Asset)
	{
		Component->DeactivateImmediate();
		Component->SetAsset(Asset);
	}

	Component->SetWorldLocation(Position);
	Component->SetVariableFloat(TEXT("User.SpawnRate"), Rate);
	Component->SetVariableVec3(TEXT("User.Velocity"), Velocity);
	if (Rate > 0.01f && !Component->IsActive())
	{
		Component->Activate(true);
	}
	else if (Rate <= 0.01f && Component->IsActive())
	{
		Component->Deactivate();
	}
}
