#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Engine/DataAsset.h"
#include "CCLMapSystem.generated.h"

UENUM()
enum class ECCLMapKind : uint8 { Player, NPC, Enemy, Objective };

UCLASS(ClassGroup = (CCL), meta = (BlueprintSpawnableComponent))
class CCL_API UCCLMapMarkerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	ECCLMapKind Kind = ECCLMapKind::NPC;
	UPROPERTY(EditAnywhere)
	float RevealDistance = 1800;
	UPROPERTY(EditAnywhere)
	uint8 bRequireLineOfSight = 1;
	UPROPERTY(EditAnywhere)
	FText Label;
	bool IsRevealedTo(const APlayerController* Observer) const;
};

USTRUCT()
struct FCCLMapMarkerView
{
	GENERATED_BODY()

	FVector Position = FVector::ZeroVector;
	float Heading = 0;
	ECCLMapKind Kind = ECCLMapKind::NPC;
	FString Label;
};

USTRUCT()
struct FCCLMapTerrain
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
class CCL_API UCCLMapRegionDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FVector2D Min = FVector2D(-2000, -1600);
	UPROPERTY(EditAnywhere)
	FVector2D Max = FVector2D(4000, 1600);
	UPROPERTY(EditAnywhere)
	TArray<FCCLMapTerrain> Terrain;
};

// Observer-specific display data only. No economy, beliefs or private life state is exposed.
UCLASS()
class CCL_API UCCLMapSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	void SetRegion(UCCLMapRegionDefinition* Definition);
	void Refresh(APlayerController* Observer);
	const TArray<FCCLMapMarkerView>& GetMarkers() const { return Markers; }
	const TArray<FCCLMapTerrain>& GetTerrain() const { return Terrain; }
	FBox2D GetBounds() const { return Bounds; }
	static FVector2D Project(FVector World, const FBox2D& Bounds, FVector2D Size);
private:
	UPROPERTY()
	TObjectPtr<UCCLMapRegionDefinition> Region;
	TWeakObjectPtr<UWorld> CachedWorld;
	TArray<FCCLMapMarkerView> Markers;
	TArray<FCCLMapTerrain> Terrain;
	FBox2D Bounds = FBox2D(FVector2D(-2000, -1600), FVector2D(4000, 1600));
};
