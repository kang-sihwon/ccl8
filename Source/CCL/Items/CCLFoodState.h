#pragma once

#include "CoreMinimal.h"
#include "CCLFoodState.generated.h"

USTRUCT(BlueprintType)
struct CCL_API FCCLFoodPolicy
{
	GENERATED_BODY()

	bool operator==(const FCCLFoodPolicy& Other) const;

	UPROPERTY(EditAnywhere)
	double ShelfLifeSeconds = 86400.;

	UPROPERTY(EditAnywhere)
	double MoldDelaySeconds = 43200.;

	UPROPERTY(EditAnywhere)
	double MoldGrowthSeconds = 43200.;
};

USTRUCT(BlueprintType)
struct CCL_API FCCLFoodStorage
{
	GENERATED_BODY()

	bool operator==(const FCCLFoodStorage& Other) const;

	UPROPERTY(EditAnywhere)
	double TemperatureCelsius = 20.;

	UPROPERTY(EditAnywhere)
	double RelativeHumidity = 0.5;

	UPROPERTY(EditAnywhere)
	double SpoilageMultiplier = 1.;

	UPROPERTY(EditAnywhere)
	double MoldExposure = 1.;
};

USTRUCT()
struct CCL_API FCCLFoodLot
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid OriginId;

	UPROPERTY()
	int32 Quantity = 0;

	UPROPERTY()
	double CreatedWorldSeconds = 0.;

	UPROPERTY()
	double SpoilageSeconds = 0.;

	UPROPERTY()
	double MoldSeconds = 0.;
};

USTRUCT()
struct CCL_API FCCLFoodState
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid WorldId;

	UPROPERTY()
	double UpdatedWorldSeconds = 0.;

	UPROPERTY()
	FCCLFoodPolicy Policy;

	UPROPERTY()
	FCCLFoodStorage Storage;

	UPROPERTY()
	TArray<FCCLFoodLot> Lots;

	bool IsPresent() const { return !Lots.IsEmpty(); }
};

struct CCL_API FCCLFoodQuality
{
	double Freshness = 1.;
	double Mold = 0.;
	double OldestAgeSeconds = 0.;
	int32 SpoiledQuantity = 0;
	int32 MoldyQuantity = 0;
};

namespace CCLFood
{
	constexpr int32 MaxLots = 256;
	bool ValidatePolicy(const FCCLFoodPolicy& Policy);
	bool ValidateStorage(const FCCLFoodStorage& Storage);
	bool Validate(const FCCLFoodState& State, int32 Quantity, FString& Error);
	bool Create(FGuid WorldId, FGuid OriginId, int32 Quantity, double WorldSeconds,
		const FCCLFoodPolicy& Policy, const FCCLFoodStorage& Storage, FCCLFoodState& Out, FString& Error);
	bool Advance(FCCLFoodState& State, double WorldSeconds, FString& Error);
	bool ChangeStorage(FCCLFoodState& State, const FCCLFoodStorage& Storage, double WorldSeconds, FString& Error);
	bool Merge(FCCLFoodState& Destination, const FCCLFoodState& Source, double WorldSeconds, FString& Error);
	bool Split(FCCLFoodState& Source, int32 Quantity, double WorldSeconds, FCCLFoodState& Removed, FString& Error);
	bool Evaluate(const FCCLFoodState& State, double WorldSeconds, FCCLFoodQuality& Quality, FString& Error);
	bool Encode(const FCCLFoodState& State, FString& Encoded, FString& Error);
	bool Decode(const FString& Encoded, FCCLFoodState& State, FString& Error);
	int32 Quantity(const FCCLFoodState& State);
}
