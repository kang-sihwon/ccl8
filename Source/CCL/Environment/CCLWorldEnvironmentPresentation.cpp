#include "CCLWorldEnvironmentPresentation.h"

#include "CCLWorldEnvironmentConfig.h"
#include "CCLWorldEnvironmentState.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Math/RotationMatrix.h"

ACCLWorldEnvironmentPresentation::ACCLWorldEnvironmentPresentation()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ACCLWorldEnvironmentPresentation::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!GetWorld()->IsGameWorld())
	{
		if (Pieces.IsEmpty())
		{
			RefreshPreview();
		}

		return;
	}
	for (TActorIterator<ACCLWorldEnvironmentState> It(GetWorld()); It; ++It)
	{
		const FCCLEnvironmentView& View = It->GetTime().Environment;
		if (!View.bValid)
		{
			return;
		}

		if (DisplayedEpoch != View.SurfaceEpoch || DisplayedRevision != View.InputRevision)
		{
			RebuildSurfaces(View);
			DisplayedEpoch = View.SurfaceEpoch;
			DisplayedRevision = View.InputRevision;
		}

		if (GetNetMode() == NM_DedicatedServer)
		{
			return;
		}

		for (const auto& Star : View.Stars)
		{
			if (Star.BodyId == View.DominantStarId && Sun)
			{
				if (auto* Component = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
				{
					Sun->SetActorRotation((-Star.LocalDirection).Rotation());
					Component->SetAtmosphereSunLightIndex(0);
					Component->SetIntensity(Star.bOcculted || Star.ElevationDegrees <= 0. ? 0.f
						: ReferenceSunIntensity * float(Star.NormalIrradiance / 1361.));
				}
			}
		}

		for (const auto& Probe : View.Probes)
		{
			const FVector Position = Probe.PositionMeters * 100.;
			DrawDebugSphere(GetWorld(), Position, 16.f, 12,
				Probe.Transmission.Precipitation < 0.5 ? FColor::Cyan : FColor::Yellow, false, 0.f, 0, 2.f);
			DrawDebugLine(GetWorld(), Position, Position + FVector(0., 0., 150.), FColor::Cyan, false, 0.f, 0, 2.f);
		}

		return;
	}
}

void ACCLWorldEnvironmentPresentation::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (GetWorld() && !GetWorld()->IsGameWorld())
	{
		RefreshPreview();
	}
}

void ACCLWorldEnvironmentPresentation::EndPlay(EEndPlayReason::Type Reason)
{
	ClearSurfaces();
	Super::EndPlay(Reason);
}

void ACCLWorldEnvironmentPresentation::RefreshPreview()
{
	if (const auto* Config = ACCLWorldEnvironmentConfig::Find(GetWorld()))
	{
		FCCLEnvironmentView View;
		View.Surfaces = Config->Surfaces;
		View.Openings = Config->Openings;
		RebuildSurfaces(View);
	}
}

void ACCLWorldEnvironmentPresentation::RebuildSurfaces(const FCCLEnvironmentView& View)
{
	ClearSurfaces();
	for (const auto& Patch : View.Surfaces)
	{
		TArray<FBox2D> Rectangles;
		Rectangles.Add(FBox2D(-Patch.HalfExtentsMeters, Patch.HalfExtentsMeters));
		for (const auto& Opening : View.Openings)
		{
			if (Opening.SurfaceId != Patch.SurfaceId || Opening.OpenFraction <= 0.)
			{
				continue;
			}

			const FVector2D HoleMin = Opening.CenterUV - Opening.HalfExtentsMeters;
			const FVector2D HoleMax(HoleMin.X + 2. * Opening.HalfExtentsMeters.X * Opening.OpenFraction,
				Opening.CenterUV.Y + Opening.HalfExtentsMeters.Y);
			TArray<FBox2D> Remaining;
			for (const FBox2D& Rectangle : Rectangles)
			{
				const FVector2D CutMin(FMath::Max(Rectangle.Min.X, HoleMin.X), FMath::Max(Rectangle.Min.Y, HoleMin.Y));
				const FVector2D CutMax(FMath::Min(Rectangle.Max.X, HoleMax.X), FMath::Min(Rectangle.Max.Y, HoleMax.Y));
				if (CutMin.X >= CutMax.X || CutMin.Y >= CutMax.Y)
				{
					Remaining.Add(Rectangle);
					continue;
				}

				const FBox2D Outside[] = {
					FBox2D(Rectangle.Min, FVector2D(CutMin.X, Rectangle.Max.Y)),
					FBox2D(FVector2D(CutMax.X, Rectangle.Min.Y), Rectangle.Max),
					FBox2D(FVector2D(CutMin.X, Rectangle.Min.Y), FVector2D(CutMax.X, CutMin.Y)),
					FBox2D(FVector2D(CutMin.X, CutMax.Y), FVector2D(CutMax.X, Rectangle.Max.Y)) };
				for (const FBox2D& Piece : Outside)
				{
					if (Piece.Max.X - Piece.Min.X > 1.e-6 && Piece.Max.Y - Piece.Min.Y > 1.e-6)
					{
						Remaining.Add(Piece);
					}
				}
			}

			Rectangles = MoveTemp(Remaining);
		}

		for (const FBox2D& Rectangle : Rectangles)
		{
			AddPiece(Patch, Rectangle.Min, Rectangle.Max);
		}
	}
}

void ACCLWorldEnvironmentPresentation::ClearSurfaces()
{
	for (UStaticMeshComponent* Piece : Pieces)
	{
		if (Piece)
		{
			Piece->DestroyComponent();
		}
	}

	Pieces.Reset();
}

void ACCLWorldEnvironmentPresentation::AddPiece(const FCCLSurfacePatch& Patch, const FVector2D& Minimum, const FVector2D& Maximum)
{
	const bool bGlass = Patch.Transmission.Sun > 0.;
	auto* Mesh = bGlass ? GlassMesh.Get() : SolidMesh.Get();
	if (!Mesh)
	{
		return;
	}

	auto* Piece = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
	Piece->SetMobility(EComponentMobility::Movable);
	Piece->SetupAttachment(RootComponent);
	Piece->SetStaticMesh(Mesh);
	Piece->SetMaterial(0, bGlass ? GlassMaterial : SolidMaterial);
	Piece->SetCollisionProfileName(TEXT("BlockAll"));
	Piece->RegisterComponent();
	const FVector2D Center = (Minimum + Maximum) * 0.5;
	const FVector Position = (Patch.CenterMeters + Patch.TangentU * Center.X + Patch.TangentV * Center.Y) * 100.;
	const FQuat Rotation = FRotationMatrix::MakeFromXY(Patch.TangentU, Patch.TangentV).ToQuat();
	// Unit cube is 100 cm. The query surface runs through the center of this 5 cm display shell.
	Piece->SetWorldTransform(FTransform(Rotation, Position, FVector(Maximum.X - Minimum.X, Maximum.Y - Minimum.Y, 0.05)));
	Pieces.Add(Piece);
}
