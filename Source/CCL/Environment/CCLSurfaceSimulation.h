#pragma once

#include "CoreMinimal.h"
#include "CCLWorldClock.h"

struct FCCLSurfaceForcing
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

struct FCCLSurfaceCell
{
	double BedMeters = 0.;
	double CeilingMeters = 32.;
	double WaterCubicMeters = 0.;
	double IceCubicMeters = 0.;
	double SoilCubicMeters = 0.;
	double SnowCubicMeters = 0.;
	double SnowVolumeCubicMeters = 0.;
	double RainExposure = 1.;
	uint8 bOpenBoundary = 0;
};

struct FCCLWaterLedger
{
	double InitialCubicMeters = 0.;
	double RainCubicMeters = 0.;
	double SnowCubicMeters = 0.;
	double BoundaryInCubicMeters = 0.;
	double BoundaryOutCubicMeters = 0.;
	double EvaporatedCubicMeters = 0.;
};

struct FCCLSurfaceGrid
{
	FGuid RegionId;
	FGuid TerrainId;
	FVector OriginMeters = FVector::ZeroVector;
	FIntPoint Size = FIntPoint(1, 1);
	double SpacingMeters = 0.5;
	uint64 Revision = 1;
	uint64 TerrainRevision = 0;
	FCCLSurfaceForcing Forcing;
	FCCLWaterLedger Ledger;
	TArray<FCCLSurfaceCell> Cells;

	double TotalCubicMeters() const;
	double BalanceErrorCubicMeters() const;
	double WaterLevelMeters(int32 Index) const;
	double Wetness(int32 Index) const;
	double Mud(int32 Index) const;
	double SnowDepthMeters(int32 Index) const;
};

struct FCCLSnowSample
{
	FGuid RegionId;
	uint64 Revision = 0;
	double BedMeters = 0.;
	double DepthMeters = 0.;
	double DensityRatio = 0.1;
};

// Value state only. The world owner advances a candidate before publishing time and life.
class CCL_API FCCLSurfaceSimulation
{
public:
	bool Initialize(FGuid WorldId, double GameSeconds, double WorldSeconds, uint64 StepId, FString& Error);
	bool AddRegion(FCCLSurfaceGrid Region, FString& Error);
	bool ChangeForcing(FGuid RegionId, const FCCLSurfaceForcing& Forcing, FString& Error);
	bool Advance(const FCCLWorldStep& Step, FString& Error);
	bool RebaseTerrain(FGuid RegionId, const TArray<double>& BedsMeters, uint64 TerrainRevision, FString& Error, bool bRestoring = false);
	bool ApplySnowContact(FGuid RegionId, FGuid SourceId, uint64 Sequence, const FVector& PositionMeters,
		double RadiusMeters, FString& Error);
	bool SampleSnow(const FVector& PositionMeters, FCCLSnowSample& Sample) const;
	uint64 LastSnowContact(FGuid SourceId) const { return SnowContacts.FindRef(SourceId); }
	bool Capture(TArray<uint8>& Bytes, FString& Error) const;
	bool Restore(const TArray<uint8>& Bytes, FString& Error);

	bool Validate(FString& Error) const;
	static bool ValidateRegion(const FCCLSurfaceGrid& Region, FString& Error);

	const FCCLSurfaceGrid* FindRegion(FGuid Id) const { return Regions.Find(Id); }
	const TMap<FGuid, FCCLSurfaceGrid>& GetRegions() const { return Regions; }
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
	bool Serialize(FArchive& Archive);

private:
	TMap<FGuid, FCCLSurfaceGrid> Regions;
	TMap<FGuid, uint64> SnowContacts;
	FGuid WorldId;
	double GameSeconds = 0.;
	double WorldSeconds = 0.;
	uint64 StepId = 0;
};
