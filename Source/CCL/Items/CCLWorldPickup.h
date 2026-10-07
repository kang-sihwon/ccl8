#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CCLWorldPickup.generated.h"

class UCCLInventoryComponent;
class UCCLItemDefinition;
class UStaticMeshComponent;
class UStaticMesh;

UCLASS()
class CCL_API ACCLWorldPickup : public AActor
{
	GENERATED_BODY()

public:
	ACCLWorldPickup();
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	bool TryCollect(UCCLInventoryComponent* Inventory, AActor* Collector);

public:
	UPROPERTY(EditAnywhere, ReplicatedUsing = RefreshVisual, Category = "Pickup")
	TObjectPtr<UCCLItemDefinition> Definition;

	UPROPERTY(EditAnywhere, Replicated, Category = "Pickup", meta = (ClampMin = "1"))
	int32 Quantity = 1;

private:
	UFUNCTION()
	void RefreshVisual();

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> FallbackMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	uint8 bCollected = 0;
};
