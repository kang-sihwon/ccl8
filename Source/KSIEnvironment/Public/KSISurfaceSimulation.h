#pragma once

#include "CoreMinimal.h"
#include "KSIWorldTime.h"
#include "KSIWeatherModel.h"
#include "KSISurfaceFire.h"
#include "KSISurfaceVegetation.h"

class FKSICelestialSystem;
class IKSISurfaceProvider;
struct FKSICelestialObserver;
struct FKSISurfaceOpening;

struct KSIENVIRONMENT_API FKSISurfaceForcing
{
	double RainMetersPerWorldSecond = 0.;
	double SnowMetersPerWorldSecond = 0.;
	double EvaporationMetersPerWorldSecond = 0.;
	double InfiltrationMetersPerWorldSecond = 0.0000005;
	double DrainageMetersPerWorldSecond = 0.00000005;
	double SoilCapacityMeters = 0.1;
	double TemperatureCelsius = 15.;
	double PhaseMetersPerDegreeWorldSecond = 0.000001;
	double FlowConductance = 0.5;
	double BoundaryLevelMeters = 0.;
	double BoundaryConductance = 0.;
};

struct KSIENVIRONMENT_API FKSISurfaceCell
{
	double BedMeters = 0.;
	double CeilingMeters = 32.;
	double WaterCubicMeters = 0.;
	double IceCubicMeters = 0.;
	double SoilCubicMeters = 0.;
	double SnowCubicMeters = 0.;
	double SnowVolumeCubicMeters = 0.;
	double RainExposure = 1.;
	double SunExposure = 1.;
	double WindExposure = 1.;
	uint8 bOpenBoundary = 0;
};

struct KSIENVIRONMENT_API FKSIWaterLedger
{
	double InitialCubicMeters = 0.;
	double RainCubicMeters = 0.;
	double SnowCubicMeters = 0.;
	double BoundaryInCubicMeters = 0.;
	double BoundaryOutCubicMeters = 0.;
	double EvaporatedCubicMeters = 0.;
};

struct KSIENVIRONMENT_API FKSISurfaceGrid
{
	FGuid RegionId;
	FGuid TerrainId;
	FVector OriginMeters = FVector::ZeroVector;
	FIntPoint Size = FIntPoint(1, 1);
	double SpacingMeters = 0.5;
	uint64 Revision = 1;
	uint64 TerrainRevision = 0;
	FKSISurfaceForcing Forcing;
	FKSIClimate Climate;
	FKSIWeatherState Weather;
	FKSIWaterLedger Ledger;
	FKSISurfaceFireLayer Fire;
	FKSISurfaceVegetationLayer Vegetation;
	double AirHeatToSurfaceJoules = 0.;
	double SurfaceHeatToAirJoules = 0.;
	uint8 bDynamicShelter = 0;
	TArray<FKSISurfaceCell> Cells;

	double TotalCubicMeters() const;
	double BalanceErrorCubicMeters() const;
	double WaterLevelMeters(int32 Index) const;
	double Wetness(int32 Index) const;
	double Mud(int32 Index) const;
	double SnowDepthMeters(int32 Index) const;
};

struct KSIENVIRONMENT_API FKSISnowSample
{
	FGuid RegionId;
	uint64 Revision = 0;
	double BedMeters = 0.;
	double DepthMeters = 0.;
	double DensityRatio = 0.1;
};

// Value state only. The world owner advances a candidate before publishing time and life.
class KSIENVIRONMENT_API FKSISurfaceSimulation
{
public:
	bool Initialize(FGuid WorldId, double GameSeconds, double WorldSeconds, uint64 StepId, FString& Error);
	bool AddRegion(FKSISurfaceGrid Region, FString& Error);
	bool ChangeForcing(FGuid RegionId, const FKSISurfaceForcing& Forcing, FString& Error);
	bool AddAirSpace(const FKSIAirSpace& Space, FString& Error);
	bool Ignite(FGuid RegionId, int32 CellIndex, double HeatJoules, FString& Error);
	bool ChangeClimate(FGuid RegionId, const FKSIClimate& Climate, FString& Error);
	bool ChangeWind(FGuid RegionId, double MetersPerSecond, FString& Error);
	bool Advance(const FKSIWorldStep& Step, FString& Error, const FKSICelestialSystem* Celestial = nullptr,
		const FKSICelestialObserver* Observer = nullptr, const IKSISurfaceProvider* Shelter = nullptr,
		const TArray<FKSISurfaceOpening>* Openings = nullptr);
	bool RebaseTerrain(FGuid RegionId, const TArray<double>& BedsMeters, uint64 TerrainRevision, FString& Error, bool bRestoring = false);
	bool ApplySnowContact(FGuid RegionId, FGuid SourceId, uint64 Sequence, const FVector& PositionMeters,
		double RadiusMeters, FString& Error);
	bool SampleSnow(const FVector& PositionMeters, FKSISnowSample& Sample) const;
	uint64 LastSnowContact(FGuid SourceId) const { return SnowContacts.FindRef(SourceId); }
	bool Capture(TArray<uint8>& Bytes, FString& Error) const;
	bool Restore(const TArray<uint8>& Bytes, FString& Error);

	bool Validate(FString& Error) const;
	bool ValidateAirContext(const TArray<FKSISurfaceOpening>& Openings, FString& Error) const;
	static bool ValidateRegion(const FKSISurfaceGrid& Region, FString& Error);

	const FKSIAirSimulation& GetAirSimulation() const { return Air; }
	const FKSISurfaceGrid* FindRegion(FGuid Id) const { return Regions.Find(Id); }
	const TMap<FGuid, FKSISurfaceGrid>& GetRegions() const { return Regions; }
	FGuid GetWorldId() const { return WorldId; }
	double GetGameSeconds() const { return GameSeconds; }
	double GetWorldSeconds() const { return WorldSeconds; }
	uint64 GetStepId() const { return StepId; }
	bool IsInitialized() const { return WorldId.IsValid(); }

	static constexpr int32 MaximumBytes = 8 * 1024 * 1024;
	static constexpr int32 MaximumCells = 65536;
	static constexpr int32 MaximumRegions = 16;
	static constexpr int32 MaximumSubsteps = 4096;

private:
	void BuildAirExteriors(TMap<FGuid, FKSIAirExterior>& Exteriors) const;
	bool AdvanceSlice(double GameDelta, double WorldFrom, double WorldDelta, FString& Error,
		const FKSICelestialSystem* Celestial, const FKSICelestialObserver* Observer,
		const IKSISurfaceProvider* Shelter, const TArray<FKSISurfaceOpening>* Openings);
	bool Serialize(FArchive& Archive);

private:
	FKSIAirSimulation Air;
	TMap<FGuid, FKSISurfaceGrid> Regions;
	TMap<FGuid, uint64> SnowContacts;
	FGuid WorldId;
	double GameSeconds = 0.;
	double WorldSeconds = 0.;
	uint64 StepId = 0;
};
