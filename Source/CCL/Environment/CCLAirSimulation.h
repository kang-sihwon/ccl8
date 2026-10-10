#pragma once

#include "CoreMinimal.h"

struct FCCLSurfaceOpening;

struct FCCLAirContents
{
	double HeatJoules = 0.;
	double VaporKg = 0.;
	double SmokeKg = 0.;

	FCCLAirContents operator+(const FCCLAirContents& Other) const;
	FCCLAirContents operator-(const FCCLAirContents& Other) const;
	FCCLAirContents operator*(double Scale) const;
	void Serialize(FArchive& Archive);
};

struct FCCLAirSpace
{
	FGuid SpaceId;
	FGuid ClimateRegionId;
	FVector MinimumMeters = FVector::ZeroVector;
	FVector MaximumMeters = FVector(1.);
	double VolumeCubicMeters = 1.;
	FCCLAirContents Contents;
	FCCLAirContents Initial;
	FCCLAirContents Added;
	FCCLAirContents Removed;

	bool Initialize(double TemperatureCelsius, double VaporKgPerCubicMeter, FString& Error);
	double TemperatureCelsius() const;
	bool Contains(const FVector& PositionMeters) const;
	bool Validate(FString& Error) const;
	void Serialize(FArchive& Archive);
};

struct FCCLAirExterior
{
	double TemperatureCelsius = 20.;
	double VaporKgPerCubicMeter = 0.008;
	double WindMetersPerSecond = 0.;
};

// Space averages. Openings come from committed geometry; exterior weather is an explicit input.
class CCL_API FCCLAirSimulation
{
public:
	bool AddSpace(const FCCLAirSpace& Space, FString& Error);
	bool AddSource(FGuid SpaceId, double HeatJoules, double VaporKg, double SmokeKg, FString& Error);
	bool Advance(double GameSeconds, const TArray<FCCLSurfaceOpening>& Openings,
		const TMap<FGuid, FCCLAirExterior>& Exteriors, FString& Error);
	bool Validate(FString& Error) const;
	bool Serialize(FArchive& Archive);

	const FCCLAirSpace* FindSpace(FGuid Id) const { return Spaces.Find(Id); }
	const FCCLAirSpace* FindSpaceAt(const FVector& PositionMeters) const;
	const TMap<FGuid, FCCLAirSpace>& GetSpaces() const { return Spaces; }
	FCCLAirContents Total() const;
	const FCCLAirContents& GetExteriorImported() const { return ExteriorImported; }
	const FCCLAirContents& GetExteriorExported() const { return ExteriorExported; }

	static constexpr int32 MaximumSpaces = 64;
	static constexpr int32 MaximumOpenings = 256;
	static constexpr double HeatCapacityPerCubicMeter = 1200.;

private:
	TMap<FGuid, FCCLAirSpace> Spaces;
	FCCLAirContents SourcesAdded;
	FCCLAirContents SourcesRemoved;
	FCCLAirContents ExteriorImported;
	FCCLAirContents ExteriorExported;
};
