#include "CCLVegetationPresentation.h"

#include "CCLExperimentDirector.h"
#include "CCLSurfaceReplication.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Font.h"
#include "Misc/CoreMisc.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Crc.h"
#include "UObject/ConstructorHelpers.h"

ACCLVegetationPresentation::ACCLVegetationPresentation()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bAllowTickOnDedicatedServer = false;
	PrimaryActorTick.TickInterval = 0.25f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Shapes[] = {Sphere.Object, Cone.Object, Cylinder.Object, Cube.Object};
	for (int32 State = 0; State < 4; ++State)
	{
		for (int32 Shape = 0; Shape < 4; ++Shape)
		{
			auto* Batch = CreateDefaultSubobject<UInstancedStaticMeshComponent>(*FString::Printf(TEXT("Plant%d_%d"), State, Shape));
			Batch->SetupAttachment(RootComponent);
			Batch->SetStaticMesh(Shapes[Shape]);
			Batch->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Batch->SetCanEverAffectNavigation(false);
			Batches.Add(Batch);
		}
	}

	UFont* LabelFont = nullptr;
	if (!IsRunningDedicatedServer())
	{
		static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Game/Environment/Fonts/F_VegetationLabel.F_VegetationLabel"));
		LabelFont = Font.Object;
	}
	const TCHAR* Names[] = {TEXT("나무"), TEXT("풀"), TEXT("꽃"), TEXT("관목"), TEXT("이끼"), TEXT("양치"), TEXT("조류"), TEXT("수생"), TEXT("버섯")};
	for (int32 I = 0; I < 9; ++I)
	{
		auto* Label = CreateDefaultSubobject<UTextRenderComponent>(*FString::Printf(TEXT("Species%d"), I));
		Label->SetupAttachment(RootComponent);
		if (LabelFont)
		{
			Label->SetFont(LabelFont);
		}
		Label->SetWorldSize(36.);
		Label->SetHorizontalAlignment(EHTA_Center);
		Label->SetText(FText::FromString(Names[I]));
		Label->SetTextRenderColor(FColor::White);
		Labels.Add(Label);
	}
}

void ACCLVegetationPresentation::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() == NM_DedicatedServer)
	{
		SetActorTickEnabled(false);
		return;
	}
	Clear();
}

void ACCLVegetationPresentation::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const auto* Model = UCCLSurfaceReplication::View(GetWorld());
	if (!Model)
	{
		if (bHasState)
		{
			Clear();
		}

		return;
	}

	TArray<const FCCLSurfaceGrid*> Grids;
	TArray<double> Values;
	for (const auto& Pair : Model->GetRegions())
	{
		const auto& G = Pair.Value;
		if (G.Vegetation.Cells.IsEmpty() || !UCCLSurfaceReplication::IsGeometryReady(GetWorld(), *Model, G))
		{
			continue;
		}

		Grids.Add(&G);
		Values.Append({double(G.RegionId.A), double(G.RegionId.B), double(G.RegionId.C), double(G.RegionId.D),
			G.OriginMeters.X, G.OriginMeters.Y, G.SpacingMeters, double(G.Size.X), double(G.Size.Y)});
		for (int32 I = 0; I < G.Vegetation.Cells.Num(); ++I)
		{
			const auto& C = G.Vegetation.Cells[I];
			if (G.Vegetation.Species.IsValidIndex(C.SpeciesIndex))
			{
				const auto& S = G.Vegetation.Species[C.SpeciesIndex];
				Values.Append({double(S.Kind), S.MaximumBiomassKg});
				if (S.Kind == ECCLVegetationKind::Algae)
				{
					Values.Add(G.WaterLevelMeters(I));
				}
			}

			Values.Append({double(C.SpeciesIndex), double(C.State), G.Cells[I].BedMeters,
				FMath::FloorToDouble(C.LivingBiomassKg * 1000.), FMath::FloorToDouble(C.DeadBiomassKg * 1000.),
				FMath::FloorToDouble(C.BurnedBiomassKg * 1000.)});
		}
	}

	const uint32 Hash = FCrc::MemCrc32(Values.GetData(), Values.Num() * sizeof(double));
	if (bHasState && LastHash == Hash)
	{
		return;
	}

	Clear();
	for (const auto* G : Grids)
	{
		Append(*G);
	}

	LastHash = Hash;
	bHasState = 1;
}

void ACCLVegetationPresentation::RefreshPreview()
{
	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		return;
	}

	FCCLSurfaceGrid Grid;
	FString Error;
	Clear();
	if (ACCLExperimentDirector::VegetationGrid(0., Grid, Error))
	{
		Append(Grid);
	}
}

void ACCLVegetationPresentation::Append(const FCCLSurfaceGrid& G)
{
	if (G.RegionId == ACCLExperimentDirector::VegetationRegionId() && G.Size.X == 9)
	{
		for (int32 I = 0; I < Labels.Num(); ++I)
		{
			if (!Labels[I])
			{
				continue;
			}

			Labels[I]->SetWorldLocation(FVector((G.OriginMeters.X + (I + 0.5) * G.SpacingMeters) * 100.,
				G.OriginMeters.Y * 100. - 35., G.Cells[I].BedMeters * 100. + 55.));
			Labels[I]->SetWorldRotation(FRotator(35., -90., 0.));
			Labels[I]->SetVisibility(true);
		}
	}

	for (int32 Index = 0; Index < Batches.Num(); ++Index)
	{
		if (StateMaterials.IsValidIndex(Index / 4) && StateMaterials[Index / 4])
		{
			Batches[Index]->SetMaterial(0, StateMaterials[Index / 4]);
		}
	}

	for (int32 I = 0; I < G.Vegetation.Cells.Num(); ++I)
	{
		const auto& C = G.Vegetation.Cells[I];
		if (!G.Vegetation.Species.IsValidIndex(C.SpeciesIndex))
		{
			continue;
		}

		const auto& Species = G.Vegetation.Species[C.SpeciesIndex];
		const double Biomass = C.LivingBiomassKg + C.DeadBiomassKg;
		if (Biomass < 0.0001 && C.BurnedBiomassKg < 0.0001)
		{
			continue;
		}

		++RenderedPatchCount;
		FVector Base((G.OriginMeters.X + (I % G.Size.X + 0.5) * G.SpacingMeters) * 100.,
			(G.OriginMeters.Y + (I / G.Size.X + 0.5) * G.SpacingMeters) * 100., G.Cells[I].BedMeters * 100. + 2.);
		if (Species.Kind == ECCLVegetationKind::Algae)
		{
			Base.Z = G.WaterLevelMeters(I) * 100. + 2.;
		}

		const bool bDead = C.State == ECCLVegetationState::Dead;
		const int32 Palette = bDead ? (C.BurnedBiomassKg > 0.001 ? 2 : 1) : C.State == ECCLVegetationState::Dormant ? 1 : 0;
		const double Size = FMath::Clamp(FMath::Sqrt(Biomass / Species.MaximumBiomassKg), 0.12, 1.);
		auto Add = [&](int32 Shape, FVector Offset, FVector Scale, FRotator Rotation = FRotator::ZeroRotator, int32 Color = INDEX_NONE)
		{
			if (bDead)
			{
				Offset.Z *= 0.2;
				Scale.Z *= 0.2;
			}

			Batches[(Color == INDEX_NONE ? Palette : Color) * 4 + Shape]->AddInstance(
				FTransform(Rotation, Base + Offset * Size, Scale * Size), true);
		};
		switch (Species.Kind)
		{
		case ECCLVegetationKind::Tree:
			Add(2, FVector(0., 0., 65.), FVector(.16, .16, 1.3));
			for (int32 K = 0; K < 3; ++K)
			{
				Add(1, FVector(0., 0., 95. + K * 30.), FVector(1. - K * .2, 1. - K * .2, 1.));
			}

			break;
		case ECCLVegetationKind::Grass:
		case ECCLVegetationKind::AquaticPlant:
			for (int32 K = 0; K < 7; ++K)
			{
				Add(3, FVector((K % 3 - 1) * 15., (K / 3 - 1) * 15., 25.), FVector(.04, .07, .5), FRotator(K * 7. - 20., K * 51., 0.));
			}

			break;
		case ECCLVegetationKind::Flower:
			Add(2, FVector(0., 0., 25.), FVector(.035, .035, .5));
			for (int32 K = 0; K < 5; ++K)
			{
				Add(0, FVector(FMath::Cos(K * UE_DOUBLE_TWO_PI / 5.) * 12., FMath::Sin(K * UE_DOUBLE_TWO_PI / 5.) * 12., 52.), FVector(.2, .2, .08), FRotator::ZeroRotator, Palette != 0 ? Palette : 3);
			}

			break;
		case ECCLVegetationKind::Shrub:
			for (int32 K = 0; K < 4; ++K)
			{
				Add(0, FVector((K % 2 - .5) * 32., (K / 2 - .5) * 32., 24.), FVector(.6, .6, .6));
			}

			break;
		case ECCLVegetationKind::Moss:
		case ECCLVegetationKind::Algae:
			for (int32 K = 0; K < 5; ++K)
			{
				Add(0, FVector((K % 3 - 1) * 20., (K / 3 - .5) * 22., 4.), FVector(.4, .4, .06));
			}

			break;
		case ECCLVegetationKind::Fern:
			for (int32 K = 0; K < 8; ++K)
			{
				Add(3, FVector(0., 0., 8. + K * 2.), FVector(.55 - K * .045, .07, .03), FRotator(15., K * 45., 0.));
			}

			break;
		case ECCLVegetationKind::Fungus:
			Add(2, FVector(0., 0., 15.), FVector(.1, .1, .3));
			Add(0, FVector(0., 0., 31.), FVector(.55, .55, .2), FRotator::ZeroRotator, Palette != 0 ? Palette : 3);
			break;
		}

		if (C.BurnedBiomassKg > 0.001)
		{
			Batches[2 * 4]->AddInstance(FTransform(FRotator::ZeroRotator, Base, FVector(.7, .7, .02)), true);
		}
	}
}

void ACCLVegetationPresentation::Clear()
{
	for (const auto& Batch : Batches)
	{
		if (Batch)
		{
			Batch->ClearInstances();
		}
	}

	for (const auto& Label : Labels)
	{
		if (Label)
		{
			Label->SetVisibility(false);
		}
	}

	RenderedPatchCount = 0;
	bHasState = 0;
}
