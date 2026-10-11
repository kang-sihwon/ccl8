#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Engine/DataAsset.h"
#include "KSIMapSystem.generated.h"

UENUM()
enum class EKSIMapKind : uint8 { Player, NPC, Enemy, Objective };

UCLASS(ClassGroup = (KSI), meta = (BlueprintSpawnableComponent))
class KSIMAP_API UKSIMapMarkerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	EKSIMapKind Kind = EKSIMapKind::NPC;
	UPROPERTY(EditAnywhere)
	float RevealDistance = 1800;
	UPROPERTY(EditAnywhere)
	uint8 bRequireLineOfSight = 1;
	UPROPERTY(EditAnywhere)
	FText Label;
	bool IsRevealedTo(const APlayerController* Observer) const;
};

USTRUCT()
struct KSIMAP_API FKSIMapMarkerView
{
	GENERATED_BODY()

	FVector Position = FVector::ZeroVector;
	float Heading = 0;
	EKSIMapKind Kind = EKSIMapKind::NPC;
	FString Label;
};

USTRUCT()
struct KSIMAP_API FKSIMapTerrain
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FVector2D Min = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere)
	FVector2D Max = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere)
	FLinearColor Color = FLinearColor(0.1f, 0.15f, 0.15f);
};

UCLASS()
class KSIMAP_API UKSIMapRegionDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FVector2D Min = FVector2D(-2000, -1600);
	UPROPERTY(EditAnywhere)
	FVector2D Max = FVector2D(4000, 1600);
	UPROPERTY(EditAnywhere)
	TArray<FKSIMapTerrain> Terrain;
};

// Observer-specific display data only. No economy, beliefs or private life state is exposed.
UCLASS()
class KSIMAP_API UKSIMapSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	void SetRegion(UKSIMapRegionDefinition* Definition);
	void Refresh(APlayerController* Observer);
	const TArray<FKSIMapMarkerView>& GetMarkers() const { return Markers; }
	const TArray<FKSIMapTerrain>& GetTerrain() const { return Terrain; }
	FBox2D GetBounds() const { return Bounds; }
	static FVector2D Project(FVector World, const FBox2D& Bounds, FVector2D Size);
private:
	UPROPERTY()
	TObjectPtr<UKSIMapRegionDefinition> Region;
	TWeakObjectPtr<UWorld> CachedWorld;
	TArray<FKSIMapMarkerView> Markers;
	TArray<FKSIMapTerrain> Terrain;
	FBox2D Bounds = FBox2D(FVector2D(-2000, -1600), FVector2D(4000, 1600));
};
