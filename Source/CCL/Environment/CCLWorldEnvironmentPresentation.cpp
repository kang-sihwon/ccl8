#include "CCLWorldEnvironmentPresentation.h"

#include "CCLWorldEnvironmentConfig.h"
#include "CCLWorldEnvironmentState.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
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

		FVector CameraPosition = FVector::ZeroVector;
		FRotator CameraRotation;
		const auto* PC = GetWorld()->GetFirstPlayerController();
		if (PC)
		{
			PC->GetPlayerViewPoint(CameraPosition, CameraRotation);
		}
		for (const auto& Probe : View.Probes)
		{
			const FVector Position = Probe.PositionMeters * 100.;
			if (!PC || FVector::DistSquared(CameraPosition, Position) > FMath::Square(2500.))
			{
				continue;
			}
			DrawDebugSphere(GetWorld(), Position, 12.f, 12, FColor::White, false, 0.f, 0, 1.f);
			auto DrawTransmission = [&](const FVector& Direction, double Fraction, FColor Color, const FVector& Offset)
			{
				const FVector End = Position + Offset;
				const FVector Source = End + Direction.GetSafeNormal() * 220.;
				DrawDebugLine(GetWorld(), Source, End, FColor(45, 50, 55), false, 0.f, 0, 1.f);
				if (Fraction > 0.001)
				{
					DrawDebugDirectionalArrow(GetWorld(), Source, FMath::Lerp(Source, End, Fraction), 18.f, Color, false, 0.f, 0, 3.f);
				}
				else
				{
					DrawDebugPoint(GetWorld(), Source, 10.f, FColor::Red, false, 0.f);
				}
			};
			DrawTransmission(Probe.ToSun, Probe.Transmission.Sun, FColor::Yellow, FVector(0, -24, 0));
			DrawTransmission(Probe.ToPrecipitationSource, Probe.Transmission.Precipitation, FColor(70, 140, 255), FVector(0, 0, 0));
			DrawTransmission(Probe.ToWindSource, Probe.Transmission.Wind, FColor::Cyan, FVector(0, 24, 0));
			DrawDebugString(GetWorld(), Position + FVector(0, 0, 260), FString::Printf(TEXT("%s\nSun %.0f%% | Rain %.0f%% | Wind %.0f%%"),
				*Probe.ProbeId.ToString(), 100. * Probe.Transmission.Sun, 100. * Probe.Transmission.Precipitation,
				100. * Probe.Transmission.Wind), nullptr, FColor::White, 0.f, true, 1.f);
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
