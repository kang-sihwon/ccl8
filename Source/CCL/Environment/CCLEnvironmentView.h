#pragma once

#include "CoreMinimal.h"
#include "CCLCelestialSystem.h"
#include "CCLSurfaceQuery.h"
#include "CCLEnvironmentView.generated.h"

USTRUCT(BlueprintType)
struct CCL_API FCCLEnvironmentProbe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ProbeId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector PositionMeters = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector ToPrecipitationSource = FVector::UpVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector ToWindSource = FVector::ForwardVector;
};

USTRUCT()
struct FCCLEnvironmentProbeView
{
	GENERATED_BODY()

	UPROPERTY()
	FName ProbeId;

	UPROPERTY()
	FVector PositionMeters = FVector::ZeroVector;

	UPROPERTY()
	FVector ToSun = FVector::UpVector;

	UPROPERTY()
	FVector ToPrecipitationSource = FVector::UpVector;

	UPROPERTY()
	FVector ToWindSource = FVector::ForwardVector;

	UPROPERTY()
	FCCLSurfaceTransmission Transmission;
};

USTRUCT()
struct FCCLCelestialSourceView
{
	GENERATED_BODY()

	UPROPERTY()
	FName BodyId;

	UPROPERTY()
	FVector LocalDirection = FVector::UpVector;

	UPROPERTY()
	double SolarHours = 0.;

	UPROPERTY()
	double ElevationDegrees = 0.;

	UPROPERTY()
	double NormalIrradiance = 0.;

	UPROPERTY()
	double HorizontalIrradiance = 0.;

	UPROPERTY()
	uint8 bOcculted = 0;
};

USTRUCT()
struct FCCLCelestialBodyView
{
	GENERATED_BODY()

	UPROPERTY()
	FName BodyId;

	UPROPERTY()
	FVector LocalDirection = FVector::UpVector;

	UPROPERTY()
	double AngularRadiusDegrees = 0.;

	UPROPERTY()
	double IlluminatedFraction = 0.;
};

// Diagnostic/presentation interest set, published with its exact committed world time.
USTRUCT()
struct FCCLEnvironmentView
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 bValid = 0;

	UPROPERTY()
	FGuid DefinitionId;

	UPROPERTY()
	int32 DefinitionVersion = 0;

	UPROPERTY()
	int32 Seed = 0;

	UPROPERTY()
	uint64 InputRevision = 0;

	UPROPERTY()
	uint64 SurfaceRevision = 0;

	UPROPERTY()
	FGuid SurfaceEpoch;

	UPROPERTY()
	FCCLCelestialObserver Observer;

	UPROPERTY()
	double ObliquityDegrees = 0.;

	// The diagnostic view evaluates this definition at the same committed WorldSeconds.
	UPROPERTY()
	double CelestialEpochSeconds = 0.;

	UPROPERTY()
	TArray<FCCLCelestialBodyDefinition> CelestialBodies;

	UPROPERTY()
	FName DominantStarId;

	UPROPERTY()
	TArray<FCCLCelestialSourceView> Stars;

	UPROPERTY()
	TArray<FCCLCelestialBodyView> SkyBodies;

	UPROPERTY()
	TArray<FCCLEnvironmentProbeView> Probes;

	UPROPERTY()
	TArray<FCCLSurfaceOpening> Openings;

	UPROPERTY()
	TArray<FCCLSurfacePatch> Surfaces;
};
