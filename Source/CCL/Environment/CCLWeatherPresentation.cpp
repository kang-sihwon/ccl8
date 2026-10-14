#include "CCLWeatherPresentation.h"

#include "CCLSurfaceReplication.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Components/VolumetricCloudComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"

ACCLWeatherPresentation::ACCLWeatherPresentation()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Clouds = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("Clouds"));
	Clouds->SetupAttachment(RootComponent);
	Clouds->SetVisibility(false);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.2f;
}

float ACCLWeatherPresentation::GetCloudFraction(const UWorld* World)
{
	const auto* Surface = UCCLSurfaceReplication::View(World);
	const auto* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!Surface || !PC)
	{
		return 0.f;
	}

	FVector Camera;
	FRotator Rotation;
	PC->GetPlayerViewPoint(Camera, Rotation);
	double Nearest = TNumericLimits<double>::Max();
	float Result = 0.f;
	for (const auto& Pair : Surface->GetRegions())
	{
		const auto& G = Pair.Value;
		const double Distance = FVector::DistSquared2D(Camera / 100., G.OriginMeters);
		if (G.bDynamicShelter && Distance < Nearest)
		{
			Nearest = Distance;
			Result = float(FMath::Clamp(G.Weather.CloudMeters / 0.003, 0., 1.));
		}
	}
	return Result;
}

void ACCLWeatherPresentation::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetNetMode() == NM_DedicatedServer || !RainSystem || !SnowSystem)
	{
		return;
	}
	if (CloudMaterial && !CloudInstance)
	{
		CloudInstance = UMaterialInstanceDynamic::Create(CloudMaterial, this);
		Clouds->SetMaterial(CloudInstance);
	}
	if (CloudInstance)
	{
		const float Fraction = GetCloudFraction(GetWorld());
		CloudInstance->SetScalarParameterValue(TEXT("Cloud_GlobalCoverage"), Fraction);
		Clouds->SetVisibility(Fraction > 0.001f);
	}
	const auto* Surface = UCCLSurfaceReplication::View(GetWorld());
	const auto* PC = GetWorld()->GetFirstPlayerController();
	if (!Surface || !PC)
	{
		return;
	}
	FVector Camera;
	FRotator Rotation;
	PC->GetPlayerViewPoint(Camera, Rotation);
	int32 At = 0;
	for (const auto& Pair : Surface->GetRegions())
	{
		const auto& G = Pair.Value;
		if (FVector::DistSquared2D(Camera / 100., G.OriginMeters) > FMath::Square(40.))
		{
			continue;
		}
		const FKSIAirSpace* Indoor = nullptr;
		for (const auto& PairRoom : Surface->GetAirSimulation().GetSpaces())
		{
			if (PairRoom.Value.ClimateRegionId == G.RegionId)
			{
				Indoor = &PairRoom.Value;
				break;
			}
		}
		const int32 Tile = FMath::Max(1, FMath::FloorToInt(2. / G.SpacingMeters));
		const int32 Margin = Indoor ? FMath::CeilToInt(6. / G.SpacingMeters) : 0;
		// Exterior tiles make roof shelter visible; they do not add simulated water.
		for (int32 Y = -Margin; Y < G.Size.Y + Margin; Y += Tile)
		{
			for (int32 X = -Margin; X < G.Size.X + Margin; X += Tile)
			{
				while (Rain.Num() <= At)
				{
					auto Make = [&](UNiagaraSystem* Asset)
					{
						auto* C = NewObject<UNiagaraComponent>(this, NAME_None, RF_Transient);
						C->SetupAttachment(RootComponent);
						C->SetAutoActivate(false);
						C->SetAsset(Asset);
						C->RegisterComponent();
						return C;
					};
					Rain.Add(Make(RainSystem));
					Snow.Add(Make(SnowSystem));
				}
				const int32 Index = FMath::Clamp(X, 0, G.Size.X - 1) + FMath::Clamp(Y, 0, G.Size.Y - 1) * G.Size.X;
				const auto& Cell = G.Cells[Index];
				const FVector Position((G.OriginMeters.X + (X + Tile * .5) * G.SpacingMeters) * 100.,
					(G.OriginMeters.Y + (Y + Tile * .5) * G.SpacingMeters) * 100., (Cell.BedMeters + 2.8) * 100.);
				const double Exposure = Indoor ? (Indoor->Contains(Position / 100.) ? 0. : 1.) : Cell.RainExposure;
				auto Update = [&](UNiagaraComponent* C, double Rate, bool bSnow)
				{
					const float Spawn = float(FMath::Clamp(Rate * 3600000. * 3. * Exposure, 0., 240.));
					C->SetWorldLocation(Position);
					C->SetVariableFloat(TEXT("User.SpawnRate"), Spawn);
					C->SetVariableVec3(TEXT("User.Velocity"), FVector(G.Weather.WindMetersPerSecond * 25., 0., bSnow ? -100. : -800.));
					if (Spawn > 0.01f && !C->IsActive())
					{
						C->Activate(true);
					}

					if (Spawn <= 0.01f && C->IsActive())
					{
						C->DeactivateImmediate();
					}
				};
				Update(Rain[At], G.Forcing.RainMetersPerWorldSecond, false);
				Update(Snow[At], G.Forcing.SnowMetersPerWorldSecond, true);
				++At;
			}
		}
	}
	for (int32 I = At; I < Rain.Num(); ++I)
	{
		Rain[I]->DeactivateImmediate();
		Snow[I]->DeactivateImmediate();
	}
}

