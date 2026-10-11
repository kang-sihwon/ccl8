#pragma once

#include "CoreMinimal.h"

struct FKSISurfaceOpening;

struct KSIENVIRONMENT_API FKSIAirContents
{
	double HeatJoules = 0.;
	double VaporKg = 0.;
	double SmokeKg = 0.;

	FKSIAirContents operator+(const FKSIAirContents& Other) const;
	FKSIAirContents operator-(const FKSIAirContents& Other) const;
	FKSIAirContents operator*(double Scale) const;
	void Serialize(FArchive& Archive);
};

struct KSIENVIRONMENT_API FKSIAirSpace
{
	FGuid SpaceId;
	FGuid ClimateRegionId;
	FVector MinimumMeters = FVector::ZeroVector;
	FVector MaximumMeters = FVector(1.);
	double VolumeCubicMeters = 1.;
	FKSIAirContents Contents;
	FKSIAirContents Initial;
	FKSIAirContents Added;
	FKSIAirContents Removed;

	bool Initialize(double TemperatureCelsius, double VaporKgPerCubicMeter, FString& Error);
	double TemperatureCelsius() const;
	bool Contains(const FVector& PositionMeters) const;
	bool Validate(FString& Error) const;
	void Serialize(FArchive& Archive);
};

struct KSIENVIRONMENT_API FKSIAirExterior
{
	double TemperatureCelsius = 20.;
	double VaporKgPerCubicMeter = 0.008;
	double WindMetersPerSecond = 0.;
};

// Space averages. Openings come from committed geometry; exterior weather is an explicit input.
class KSIENVIRONMENT_API FKSIAirSimulation
{
public:
	bool AddSpace(const FKSIAirSpace& Space, FString& Error);
	bool AddSource(FGuid SpaceId, double HeatJoules, double VaporKg, double SmokeKg, FString& Error);
	bool Advance(double GameSeconds, const TArray<FKSISurfaceOpening>& Openings,
		const TMap<FGuid, FKSIAirExterior>& Exteriors, FString& Error);
	bool Validate(FString& Error) const;
	bool Serialize(FArchive& Archive);

	const FKSIAirSpace* FindSpace(FGuid Id) const { return Spaces.Find(Id); }
	const FKSIAirSpace* FindSpaceAt(const FVector& PositionMeters) const;
	const TMap<FGuid, FKSIAirSpace>& GetSpaces() const { return Spaces; }
	FKSIAirContents Total() const;
	const FKSIAirContents& GetExteriorImported() const { return ExteriorImported; }
	const FKSIAirContents& GetExteriorExported() const { return ExteriorExported; }

	static constexpr int32 MaximumSpaces = 64;
	static constexpr int32 MaximumOpenings = 256;
	static constexpr double HeatCapacityPerCubicMeter = 1200.;

private:
	TMap<FGuid, FKSIAirSpace> Spaces;
	FKSIAirContents SourcesAdded;
	FKSIAirContents SourcesRemoved;
	FKSIAirContents ExteriorImported;
	FKSIAirContents ExteriorExported;
};
