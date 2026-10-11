#pragma once

#include "CoreMinimal.h"
#include "KSISurfaceQuery.generated.h"

// Values are query-mask bits; geometry providers may return several surfaces at one XY.
UENUM(BlueprintType)
enum class EKSISurfaceKind : uint8
{
	None = 0 UMETA(Hidden),
	Terrain = 1,
	Structure = 2,
	Water = 4,
	Snow = 8
};

USTRUCT(BlueprintType)
struct KSICORE_API FKSISurfaceTransmission
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double Sun = 0.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double Precipitation = 0.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double Wind = 0.;
};

struct KSICORE_API FKSISurfaceQuery
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

struct KSICORE_API FKSISurfaceSample
{
	FGuid SurfaceId;
	FName BodyId;
	uint64 Revision = 0;
	FGuid Epoch;
	uint64 SourceRevision = 0;
	FGuid SourceEpoch;
	FVector3d PositionMeters = FVector3d::ZeroVector;
	FVector3d Normal = FVector3d::ZAxisVector;
	double DistanceMeters = 0.;
	FName MaterialId;
	EKSISurfaceKind Kind = EKSISurfaceKind::Terrain;
	double WaterDepthMeters = 0.;
	double SnowDepthMeters = 0.;
	FKSISurfaceTransmission Transmission;
};

// Finite planar fixture; terrain providers use the same query contract.
USTRUCT(BlueprintType)
struct KSICORE_API FKSISurfacePatch
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
	EKSISurfaceKind Kind = EKSISurfaceKind::Structure;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double WaterDepthMeters = 0.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double SnowDepthMeters = 0.;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FKSISurfaceTransmission Transmission;
};

USTRUCT(BlueprintType)
struct KSICORE_API FKSISurfaceOpening
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

struct KSICORE_API FKSIShelterQuery
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

struct KSICORE_API FKSIShelterSample
{
	uint64 Revision = 0;
	FGuid Epoch;
	FKSISurfaceTransmission Transmission;
};

class KSICORE_API IKSISurfaceProvider
{
public:
	virtual ~IKSISurfaceProvider() = default;

	// Nearest first, stable ID tie break. Any failure leaves OutSamples unchanged.
	virtual bool QuerySurfaces(const FKSISurfaceQuery& Query, TArray<FKSISurfaceSample>& OutSamples, FString& Error) const = 0;

	// Volume occupancy; providers without this contract reject rather than assume clear air.
	virtual bool QuerySolidPoint(FName BodyId, const FVector3d& PositionMeters, bool& bOutSolid, FString& Error) const;

	virtual bool QueryOpeningFraction(const FKSISurfaceOpening& Opening, double& OutFraction, FString& Error) const;

	virtual uint64 GetRevision() const = 0;

	virtual FGuid GetEpoch() const = 0;
};

class KSICORE_API FKSISurfaceScene final : public IKSISurfaceProvider
{
public:
	virtual bool QuerySurfaces(const FKSISurfaceQuery& Query, TArray<FKSISurfaceSample>& OutSamples, FString& Error) const override;

	virtual bool QuerySolidPoint(FName BodyId, const FVector3d& PositionMeters, bool& bOutSolid, FString& Error) const override;

	virtual bool QueryOpeningFraction(const FKSISurfaceOpening& Opening, double& OutFraction, FString& Error) const override;

	virtual uint64 GetRevision() const override { return Revision; }

	virtual FGuid GetEpoch() const override { return Epoch; }

public:
	bool Replace(const TArray<FKSISurfacePatch>& CandidatePatches, const TArray<FKSISurfaceOpening>& CandidateOpenings,
		uint64 NewRevision, FString& Error);

	void SetGeometryProvider(FGuid Id, TSharedPtr<const IKSISurfaceProvider> Provider);
	void CopyGeometryProviders(const FKSISurfaceScene& Other);

	double GetEffectiveOpeningAreaM2(const FGuid& SpaceId) const;

	const TArray<FKSISurfacePatch>& GetPatches() const { return Patches; }

	const TArray<FKSISurfaceOpening>& GetOpenings() const { return Openings; }

private:
	TMap<FGuid, TSharedPtr<const IKSISurfaceProvider>> GeometryProviders;
	TArray<FKSISurfacePatch> Patches;
	TArray<FKSISurfaceOpening> Openings;
	TMap<FGuid, TArray<int32>> OpeningsBySurface;
	uint64 Revision = 0;
	FGuid Epoch = FGuid::NewGuid();
};

class KSICORE_API FKSIShelterEvaluator
{
public:
	static bool Evaluate(const IKSISurfaceProvider& Provider, const FKSIShelterQuery& Query,
		FKSIShelterSample& OutSample, FString& Error);
};
