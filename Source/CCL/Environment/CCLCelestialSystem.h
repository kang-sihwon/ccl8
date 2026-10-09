#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CCLCelestialSystem.generated.h"

UENUM(BlueprintType)
enum class ECCLCelestialKind : uint8
{
	Star,
	Planet,
	Moon
};

// Orbits are parent-relative translations in a shared inertial frame, in km and seconds.
USTRUCT(BlueprintType)
struct CCL_API FCCLCelestialBodyDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName BodyId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ParentBodyId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECCLCelestialKind Kind = ECCLCelestialKind::Planet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double RadiusKm = 1.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double LuminosityWatts = 0.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double SemiMajorAxisKm = 0.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double Eccentricity = 0.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double OrbitalPeriodSeconds = 86400.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double InclinationDegrees = 0.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double AscendingNodeDegrees = 0.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double PeriapsisDegrees = 0.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double MeanAnomalyDegrees = 0.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double ObliquityDegrees = 0.;

	// Signed sidereal period; negative means retrograde rotation. Not a solar day.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double SpinPeriodSeconds = 86400.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double SpinPhaseDegrees = 0.;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLCelestialDefinitionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid DefinitionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Version = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Seed = 42;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double EpochWorldSeconds = 0.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCCLCelestialBodyDefinition> Bodies;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLCelestialObserver
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName BodyId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double LatitudeDegrees = 0.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double LongitudeDegrees = 0.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double AltitudeMeters = 0.;
};

struct CCL_API FCCLCelestialBodyState
{
	FName BodyId;
	FVector3d PositionKm = FVector3d::ZeroVector;
	FVector3d EquatorX = FVector3d::XAxisVector;
	FVector3d EquatorY = FVector3d::YAxisVector;
	FVector3d SpinAxis = FVector3d::ZAxisVector;
	double SpinRadians = 0.;
};

struct CCL_API FCCLStellarObservation
{
	FName BodyId;
	// Local X = north, Y = east, Z = up. Points towards the source.
	FVector3d LocalDirection = FVector3d::ZAxisVector;
	double DistanceKm = 0.;
	double ElevationDegrees = 0.;
	double AzimuthDegrees = 0.;
	double DeclinationDegrees = 0.;
	double SolarHours = 12.;
	// Extraterrestrial values; atmosphere/cloud attenuation belongs to weather.
	double NormalIrradianceWattsPerM2 = 0.;
	double HorizontalIrradianceWattsPerM2 = 0.;
	uint8 bOcculted = 0;
};

struct CCL_API FCCLCelestialSkyBody
{
	FName BodyId;
	FVector3d LocalDirection = FVector3d::ZAxisVector;
	double AngularRadiusDegrees = 0.;
	double IlluminatedFraction = 0.;
};

struct CCL_API FCCLCelestialObservation
{
	double WorldSeconds = 0.;
	FVector3d ObserverPositionKm = FVector3d::ZeroVector;
	TArray<FCCLStellarObservation> Stars;
	TArray<FCCLCelestialSkyBody> SkyBodies;
	FName DominantStarId;
	double TotalHorizontalIrradianceWattsPerM2 = 0.;
};

UCLASS(BlueprintType)
class CCL_API UCCLCelestialDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, CallInEditor)
	void ConfigureDefault(int32 Seed);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCCLCelestialDefinitionData Definition;
};

// Immutable after validated replacement. No Tick or independent clock; outputs publish atomically.
class CCL_API FCCLCelestialSystem
{
public:
	bool Initialize(const FCCLCelestialDefinitionData& Candidate, FString& Error);

	bool Evaluate(double WorldSeconds, TArray<FCCLCelestialBodyState>& OutStates, FString& Error) const;

	bool Observe(double WorldSeconds, const FCCLCelestialObserver& Observer,
		FCCLCelestialObservation& OutObservation, FString& Error) const;

	static FCCLCelestialDefinitionData MakeDefaultDefinition(int32 Seed);

	const FCCLCelestialDefinitionData& GetDefinition() const { return Definition; }

private:
	FCCLCelestialDefinitionData Definition;
	TArray<int32> ParentIndices;
};
