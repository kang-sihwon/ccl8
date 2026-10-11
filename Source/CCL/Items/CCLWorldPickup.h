#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CCLFoodState.h"
#include "CCLWorldPickup.generated.h"

class UCCLInventoryComponent;
class UKSIItemDefinition;
class UCCLItemDefinition;
class UStaticMeshComponent;
class UStaticMesh;

USTRUCT()
struct FCCLPickupFoodView
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid Epoch;

	UPROPERTY()
	double WorldSeconds = 0.;

	UPROPERTY()
	double AgeSeconds = 0.;

	UPROPERTY()
	double Freshness = 1.;

	UPROPERTY()
	double Mold = 0.;

	UPROPERTY()
	uint8 bReady = 0;
};

UCLASS()
class CCL_API ACCLWorldPickup : public AActor
{
	GENERATED_BODY()

public:
	ACCLWorldPickup();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	bool TryCollect(UCCLInventoryComponent* Inventory, AActor* Collector);
	bool InitializeFood(UKSIItemDefinition* Item, const FCCLFoodState& Food, FGuid ItemId, FString& Error);
	static ACCLWorldPickup* DropFood(UCCLInventoryComponent* Source, FGuid ItemId, int32 Count,
									 const FTransform& Transform, FString& Error);
	const FCCLPickupFoodView& GetFoodView() const { return FoodView; }
	const UCCLInventoryComponent* GetFoodInventory() const { return FoodInventory; }

private:
	friend class UCCLInventoryPersistenceSubsystem;

	UFUNCTION()
	void RefreshVisual();

	bool EnsureFood();
	void UpdateFoodView();

public:
	UPROPERTY(EditInstanceOnly, Replicated, Category = "Pickup")
	FGuid PersistentId;

	UPROPERTY(EditAnywhere, ReplicatedUsing = RefreshVisual, Category = "Pickup")
	TObjectPtr<UKSIItemDefinition> Definition;

	UPROPERTY(EditAnywhere, Replicated, Category = "Pickup", meta = (ClampMin = "1"))
	int32 Quantity = 1;

private:
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> FallbackMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCCLInventoryComponent> FoodInventory;

	UPROPERTY(Replicated)
	FCCLPickupFoodView FoodView;

	uint8 bFoodInitialized = 0;
	uint8 bCollected = 0;
};
