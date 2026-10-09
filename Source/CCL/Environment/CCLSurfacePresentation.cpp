#include "CCLSurfacePresentation.h"

#include "CCLSurfaceReplication.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/DynamicMeshComponent.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "DynamicMesh/MeshNormals.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Crc.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	enum class ESurfaceLayer { Water, Ice, Mud, Snow };

	// Shared corners remove cell seams; each cell center retains its sampled physical height.
	// This is presentation only: no smoothing is written back into the conservative model.
	void AppendSurface(UE::Geometry::FDynamicMesh3& Mesh, const FCCLSurfaceGrid& G, ESurfaceLayer Layer,
		const UCCLSurfaceReplication* Prediction, const FTransform& ActorTransform)
	{
		const int32 NX = G.Size.X, NY = G.Size.Y;
		const double Area = FMath::Square(G.SpacingMeters);
		TArray<double> Tops;
		TArray<uint8> Wet;
		Tops.SetNum(G.Cells.Num());
		Wet.SetNumZeroed(G.Cells.Num());
		for (int32 I = 0; I < G.Cells.Num(); ++I)
		{
			const auto& C = G.Cells[I];
			switch (Layer)
			{
			case ESurfaceLayer::Water:
				Wet[I] = C.WaterCubicMeters / Area > 0.002;
				Tops[I] = G.WaterLevelMeters(I) + 0.003;
				break;
			case ESurfaceLayer::Ice:
				Wet[I] = C.IceCubicMeters / Area > 0.001;
				Tops[I] = C.BedMeters + C.IceCubicMeters / Area + 0.002;
				break;
			case ESurfaceLayer::Mud:
				Wet[I] = G.Mud(I) > 0.1;
				Tops[I] = C.BedMeters + 0.012;
				break;
			case ESurfaceLayer::Snow:
			{
				FVector Position = G.OriginMeters + FVector((I % NX + 0.5) * G.SpacingMeters, (I / NX + 0.5) * G.SpacingMeters, 0.);
				Position.Z = G.WaterLevelMeters(I);
				const double Depth = G.SnowDepthMeters(I) * (Prediction ? Prediction->PredictedSnowScale(Position) : 1.);
				Wet[I] = Depth > 0.001;
				Tops[I] = G.WaterLevelMeters(I) + Depth;
				break;
			}
			}
		}

		TArray<int32> Corners;
		TArray<double> Beds;
		Corners.Init(INDEX_NONE, (NX + 1) * (NY + 1));
		Beds.SetNumZeroed(Corners.Num());
		auto Position = [&](double X, double Y, double Z)
		{
			return ActorTransform.InverseTransformPosition(FVector((G.OriginMeters.X + X * G.SpacingMeters) * 100.,
				(G.OriginMeters.Y + Y * G.SpacingMeters) * 100., Z * 100.));
		};
		for (int32 Y = 0; Y <= NY; ++Y)
		{
			for (int32 X = 0; X <= NX; ++X)
			{
				double Top = 0., Bed = 0.;
				int32 Count = 0, BedCount = 0, Active = 0;
				for (int32 DY = -1; DY <= 0; ++DY)
				{
					for (int32 DX = -1; DX <= 0; ++DX)
					{
						if (X + DX < 0 || X + DX >= NX || Y + DY < 0 || Y + DY >= NY)
						{
							continue;
						}
						const int32 I = X + DX + (Y + DY) * NX;
						Bed += G.Cells[I].BedMeters;
						++BedCount;
						Active += Wet[I];
						if (Wet[I] || Layer == ESurfaceLayer::Snow)
						{
							Top += Tops[I];
							++Count;
						}
					}
				}
				if (Active > 0 && Count > 0)
				{
					const int32 Corner = X + Y * (NX + 1);
					Corners[Corner] = Mesh.AppendVertex(Position(X, Y, Top / Count));
					Beds[Corner] = Bed / BedCount;
				}
			}
		}
		for (int32 Y = 0; Y < NY; ++Y)
		{
			for (int32 X = 0; X < NX; ++X)
			{
				const int32 I = X + Y * NX;
				if (!Wet[I])
				{
					continue;
				}
				const int32 Center = Mesh.AppendVertex(Position(X + 0.5, Y + 0.5, Tops[I]));
				const int32 CI[] = {X + Y * (NX + 1), X + 1 + Y * (NX + 1), X + 1 + (Y + 1) * (NX + 1), X + (Y + 1) * (NX + 1)};
				const FIntPoint Neighbors[] = {{X, Y - 1}, {X + 1, Y}, {X, Y + 1}, {X - 1, Y}};
				for (int32 Edge = 0; Edge < 4; ++Edge)
				{
					const int32 A = Corners[CI[Edge]], B = Corners[CI[(Edge + 1) % 4]];
					// GeometryCore computes normals for Unreal clockwise front faces.
					Mesh.AppendTriangle(Center, B, A);
					const auto N = Neighbors[Edge];
					if (N.X >= 0 && N.X < NX && N.Y >= 0 && N.Y < NY && Wet[N.X + N.Y * NX])
					{
						continue;
					}
					// Close exposed patch edges down to the bed; no floating tile sides or added collision.
					FVector3d PA = Mesh.GetVertex(A), PB = Mesh.GetVertex(B);
					PA = ActorTransform.InverseTransformPosition(FVector(ActorTransform.TransformPosition(PA).X,
						ActorTransform.TransformPosition(PA).Y, Beds[CI[Edge]] * 100.));
					PB = ActorTransform.InverseTransformPosition(FVector(ActorTransform.TransformPosition(PB).X,
						ActorTransform.TransformPosition(PB).Y, Beds[CI[(Edge + 1) % 4]] * 100.));
					const int32 BottomA = Mesh.AppendVertex(PA), BottomB = Mesh.AppendVertex(PB);
					Mesh.AppendTriangle(A, BottomB, BottomA);
					Mesh.AppendTriangle(A, B, BottomB);
				}
			}
		}
	}
}

ACCLSurfacePresentation::ACCLSurfacePresentation()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Beds = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Beds"));
	Beds->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Beds->SetStaticMesh(Cube.Object);
	Beds->SetCollisionProfileName(TEXT("BlockAll"));
	Beds->SetCanEverAffectNavigation(false);
	Water = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("Water"));
	Ice = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("Ice"));
	Snow = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("Snow"));
	Mud = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("Mud"));
	for (auto* Component : {Water.Get(), Ice.Get(), Mud.Get(), Snow.Get()})
	{
		Component->SetupAttachment(RootComponent);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCanEverAffectNavigation(false);
		Component->SetCastShadow(Component == Snow.Get());
	}
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
		if (!G.TerrainId.IsValid() && UCCLSurfaceReplication::IsGeometryReady(GetWorld(), *Model, G))
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
	const bool bRendered = GetNetMode() != NM_DedicatedServer;
	if (bRendered && (!Water->GetMaterial(0) || !Water->GetMaterial(0)->GetPathName().Contains(TEXT("M_SurfaceWater"))))
	{
		const TPair<UMeshComponent*, const TCHAR*> Materials[] = {
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
	const UCCLSurfaceReplication* Prediction = nullptr;
	if (GetNetMode() == NM_Client)
	{
		if (const auto* PC = GetWorld()->GetFirstPlayerController())
		{
			Prediction = PC->FindComponentByClass<UCCLSurfaceReplication>();
		}
	}
	UE::Geometry::FDynamicMesh3 Meshes[4];
	for (const auto& Pair : Model->GetRegions())
	{
		const auto& G = Pair.Value;
		if (!UCCLSurfaceReplication::IsGeometryReady(GetWorld(), *Model, G))
		{
			continue;
		}
		if (!G.TerrainId.IsValid() && bRebuildBeds)
		{
			for (int32 I = 0; I < G.Cells.Num(); ++I)
			{
				const FVector Location((G.OriginMeters.X + (I % G.Size.X + 0.5) * G.SpacingMeters) * 100.,
					(G.OriginMeters.Y + (I / G.Size.X + 0.5) * G.SpacingMeters) * 100., (G.Cells[I].BedMeters - 0.1) * 100.);
				Beds->AddInstance(FTransform(FQuat::Identity, Location, FVector(G.SpacingMeters, G.SpacingMeters, 0.2)), true);
			}
		}
		if (bRendered)
		{
			for (int32 Layer = 0; Layer < 4; ++Layer)
			{
				AppendSurface(Meshes[Layer], G, ESurfaceLayer(Layer), Prediction, GetActorTransform());
			}
		}
	}
	if (bRendered)
	{
		UDynamicMeshComponent* Components[] = {Water, Ice, Mud, Snow};
		for (int32 Layer = 0; Layer < 4; ++Layer)
		{
			Meshes[Layer].EnableAttributes();
			UE::Geometry::FMeshNormals::InitializeOverlayToPerVertexNormals(Meshes[Layer].Attributes()->PrimaryNormals(), false);
			Components[Layer]->SetMesh(MoveTemp(Meshes[Layer]));
		}
	}
}
