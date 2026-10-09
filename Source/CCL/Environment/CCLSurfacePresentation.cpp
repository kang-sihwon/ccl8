#include "CCLSurfacePresentation.h"

#include "CCLSurfaceReplication.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Crc.h"
#include "UObject/ConstructorHelpers.h"

ACCLSurfacePresentation::ACCLSurfacePresentation()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Beds = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Beds"));
	Water = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Water"));
	Ice = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Ice"));
	Snow = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Snow"));
	Mud = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Mud"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	for (auto* Component : {Beds.Get(), Water.Get(), Ice.Get(), Mud.Get(), Snow.Get()})
	{
		Component->SetupAttachment(RootComponent);
		Component->SetStaticMesh(Cube.Object);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCanEverAffectNavigation(false);
		Component->SetCastShadow(false);
	}
	Beds->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Beds->SetCollisionProfileName(TEXT("BlockAll"));
}

void ACCLSurfacePresentation::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const auto* Model = UCCLSurfaceReplication::View(GetWorld());
	if (!Model)
	{
		return;
	}
	TArray<double> BedGeometry;
	for (const auto& Pair : Model->GetRegions())
	{
		const auto& G = Pair.Value;
		if (!G.TerrainId.IsValid())
		{
			BedGeometry.Append({G.OriginMeters.X, G.OriginMeters.Y, G.OriginMeters.Z, G.SpacingMeters, double(G.Size.X), double(G.Size.Y)});
			for (const auto& C : G.Cells)
			{
				BedGeometry.Add(C.BedMeters);
			}
		}
	}
	const uint32 Hash = FCrc::MemCrc32(BedGeometry.GetData(), BedGeometry.Num() * sizeof(double));
	const bool bRebuildBeds = !bBedsBuilt || Hash != BedGeometryHash;
	if (bRebuildBeds)
	{
		Beds->ClearInstances();
		bBedsBuilt = 1;
		BedGeometryHash = Hash;
	}
	for (auto* Component : {Water.Get(), Ice.Get(), Mud.Get(), Snow.Get()})
	{
		Component->ClearInstances();
	}
	if (GetWorld()->GetNetMode() != NM_DedicatedServer && (!Water->GetMaterial(0) || !Water->GetMaterial(0)->GetPathName().Contains(TEXT("M_SurfaceWater"))))
	{
		const TPair<UInstancedStaticMeshComponent*, const TCHAR*> Materials[] = {
			{Beds, TEXT("/Game/Environment/Materials/M_SurfaceBed.M_SurfaceBed")},
			{Water, TEXT("/Game/Environment/Materials/M_SurfaceWater.M_SurfaceWater")},
			{Ice, TEXT("/Game/Environment/Materials/M_SurfaceIce.M_SurfaceIce")},
			{Mud, TEXT("/Game/Environment/Materials/M_SurfaceMud.M_SurfaceMud")},
			{Snow, TEXT("/Game/Environment/Materials/M_SurfaceSnow.M_SurfaceSnow")}};
		for (const auto& Pair : Materials)
		{
			if (auto* Material = LoadObject<UMaterialInterface>(nullptr, Pair.Value))
			{
				Pair.Key->SetMaterial(0, Material);
			}
		}
	}
	for (const auto& Pair : Model->GetRegions())
	{
		const auto& G = Pair.Value;
		if (!UCCLSurfaceReplication::IsGeometryReady(GetWorld(), *Model, G))
		{
			continue;
		}
		for (int32 I = 0; I < G.Cells.Num(); ++I)
		{
			const auto& C = G.Cells[I];
			const double Area = FMath::Square(G.SpacingMeters);
			const double IceDepth = C.IceCubicMeters / Area;
			FVector Location = (G.OriginMeters + FVector((I % G.Size.X + 0.5) * G.SpacingMeters,
				(I / G.Size.X + 0.5) * G.SpacingMeters, 0.)) * 100.;
			auto Add = [&](UInstancedStaticMeshComponent* Component, double Z, double Depth)
			{
				Location.Z = Z * 100.;
				Component->AddInstance(FTransform(FQuat::Identity, Location, FVector(G.SpacingMeters, G.SpacingMeters, Depth)), true);
			};
			if (!G.TerrainId.IsValid() && bRebuildBeds)
			{
				Add(Beds, C.BedMeters - 0.1, 0.2);
			}
			if (GetWorld()->GetNetMode() == NM_DedicatedServer)
			{
				continue;
			}
			if (C.SnowVolumeCubicMeters > 0.)
			{
				double Scale = 1.;
				if (GetWorld()->GetNetMode() == NM_Client)
				{
					if (const auto* PC = GetWorld()->GetFirstPlayerController())
					{
						if (const auto* Rep = PC->FindComponentByClass<UCCLSurfaceReplication>())
						{
							Scale = Rep->PredictedSnowScale(Location / 100.);
						}
					}
				}
				const double Depth = G.SnowDepthMeters(I) * Scale;
				Add(Snow, G.WaterLevelMeters(I) + Depth * 0.5, Depth);
			}
			if (G.Mud(I) > 0.1)
			{
				Add(Mud, C.BedMeters + 0.008, 0.012);
			}
			if (IceDepth > 0.001)
			{
				Add(Ice, C.BedMeters + IceDepth - 0.004, 0.015);
			}
			if (C.WaterCubicMeters / Area > 0.002)
			{
				Add(Water, G.WaterLevelMeters(I), 0.015);
			}
		}
	}
}
