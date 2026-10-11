#pragma once

#include "CoreMinimal.h"
#include "KSICelestialSystem.h"
#include "KSISurfaceQuery.h"
#include "KSIEnvironmentView.generated.h"

USTRUCT(BlueprintType)
struct KSIENVIRONMENT_API FKSIEnvironmentProbe
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
struct KSIENVIRONMENT_API FKSIEnvironmentProbeView
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
	FKSISurfaceTransmission Transmission;
};

USTRUCT()
struct KSIENVIRONMENT_API FKSICelestialSourceView
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
struct KSIENVIRONMENT_API FKSICelestialBodyView
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
struct KSIENVIRONMENT_API FKSIEnvironmentView
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
	FKSICelestialObserver Observer;

	UPROPERTY()
	double ObliquityDegrees = 0.;

	// The diagnostic view evaluates this definition at the same committed WorldSeconds.
	UPROPERTY()
	double CelestialEpochSeconds = 0.;

	UPROPERTY()
	TArray<FKSICelestialBodyDefinition> CelestialBodies;

	UPROPERTY()
	FName DominantStarId;

	UPROPERTY()
	TArray<FKSICelestialSourceView> Stars;

	UPROPERTY()
	TArray<FKSICelestialBodyView> SkyBodies;

	UPROPERTY()
	TArray<FKSIEnvironmentProbeView> Probes;

	UPROPERTY()
	TArray<FKSISurfaceOpening> Openings;

	UPROPERTY()
	TArray<FKSISurfacePatch> Surfaces;
};
