#pragma once

#include "CoreMinimal.h"
#include "CCLSurfaceQuery.generated.h"

// Values are query-mask bits; geometry providers may return several surfaces at one XY.
UENUM(BlueprintType)
enum class ECCLSurfaceKind : uint8
{
	None = 0 UMETA(Hidden),
	Terrain = 1,
	Structure = 2,
	Water = 4,
	Snow = 8
};

USTRUCT(BlueprintType)
struct CCL_API FCCLSurfaceTransmission
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double Sun = 0.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double Precipitation = 0.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double Wind = 0.;
};

struct CCL_API FCCLSurfaceQuery
{
	FName BodyId;
	FVector3d OriginMeters = FVector3d::ZeroVector;
	FVector3d Direction = FVector3d::ZAxisVector;
	double MinimumDistanceMeters = 1.e-5;
	double MaximumDistanceMeters = 10000.;
	// Zero accepts the provider's current revision.
	uint64 RequiredRevision = 0;
	FGuid RequiredEpoch;
	uint8 KindMask = 15;
	int32 MaximumResults = 256;
};

struct CCL_API FCCLSurfaceSample
{
	FGuid SurfaceId;
	FName BodyId;
	uint64 Revision = 0;
	FGuid Epoch;
	FVector3d PositionMeters = FVector3d::ZeroVector;
	FVector3d Normal = FVector3d::ZAxisVector;
	double DistanceMeters = 0.;
	FName MaterialId;
	ECCLSurfaceKind Kind = ECCLSurfaceKind::Terrain;
	double WaterDepthMeters = 0.;
	double SnowDepthMeters = 0.;
	FCCLSurfaceTransmission Transmission;
};

// Finite planar fixture. Permanent voxel terrain implements the same query contract in stage 3.
USTRUCT(BlueprintType)
struct CCL_API FCCLSurfacePatch
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid SurfaceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName BodyId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector CenterMeters = FVector3d::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Normal = FVector3d::ZAxisVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector TangentU = FVector3d::XAxisVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector TangentV = FVector3d::YAxisVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D HalfExtentsMeters = FVector2d(1., 1.);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName MaterialId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECCLSurfaceKind Kind = ECCLSurfaceKind::Structure;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double WaterDepthMeters = 0.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double SnowDepthMeters = 0.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCCLSurfaceTransmission Transmission;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLSurfaceOpening
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid OpeningId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid SurfaceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D CenterUV = FVector2d::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D HalfExtentsMeters = FVector2d(0.5, 1.);
	// Opens from the low-U edge. This is an actual aperture, not material transparency.

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double OpenFraction = 0.;
	// An invalid space GUID denotes outdoors; at least one endpoint must be indoors.

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid SpaceA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid SpaceB;
};

struct CCL_API FCCLShelterQuery
{
	FName BodyId;
	FVector3d PositionMeters = FVector3d::ZeroVector;
	FVector3d ToSun = FVector3d::ZAxisVector;
	FVector3d ToPrecipitationSource = FVector3d::ZAxisVector;
	FVector3d ToWindSource = FVector3d::XAxisVector;
	double MaximumDistanceMeters = 10000.;
	// Zero = point sample; positive = center and four regional XY footprint samples.
	double ProbeRadiusMeters = 0.;
	uint64 RequiredRevision = 0;
	FGuid RequiredEpoch;
};

struct CCL_API FCCLShelterSample
{
	uint64 Revision = 0;
	FGuid Epoch;
	FCCLSurfaceTransmission Transmission;
};

class CCL_API ICCLSurfaceProvider
{
public:
	virtual ~ICCLSurfaceProvider() = default;

	// Nearest first, stable ID tie break. Any failure leaves OutSamples unchanged.
	virtual bool QuerySurfaces(const FCCLSurfaceQuery& Query, TArray<FCCLSurfaceSample>& OutSamples, FString& Error) const = 0;

	virtual uint64 GetRevision() const = 0;

	virtual FGuid GetEpoch() const = 0;
};

class CCL_API FCCLSurfaceScene final : public ICCLSurfaceProvider
{
public:
	virtual bool QuerySurfaces(const FCCLSurfaceQuery& Query, TArray<FCCLSurfaceSample>& OutSamples, FString& Error) const override;

	virtual uint64 GetRevision() const override { return Revision; }

	virtual FGuid GetEpoch() const override { return Epoch; }

public:
	bool Replace(const TArray<FCCLSurfacePatch>& CandidatePatches, const TArray<FCCLSurfaceOpening>& CandidateOpenings,
		uint64 NewRevision, FString& Error);

	double GetEffectiveOpeningAreaM2(const FGuid& SpaceId) const;

	const TArray<FCCLSurfacePatch>& GetPatches() const { return Patches; }

	const TArray<FCCLSurfaceOpening>& GetOpenings() const { return Openings; }

private:
	TArray<FCCLSurfacePatch> Patches;
	TArray<FCCLSurfaceOpening> Openings;
	TMap<FGuid, TArray<int32>> OpeningsBySurface;
	uint64 Revision = 0;
	FGuid Epoch = FGuid::NewGuid();
};

class CCL_API FCCLShelterEvaluator
{
public:
	static bool Evaluate(const ICCLSurfaceProvider& Provider, const FCCLShelterQuery& Query,
		FCCLShelterSample& OutSample, FString& Error);
};
