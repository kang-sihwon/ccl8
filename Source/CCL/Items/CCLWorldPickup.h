#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CCLWorldPickup.generated.h"

class UCCLInventoryComponent;
class UCCLItemDefinition;
class UStaticMeshComponent;

UCLASS()
class CCL_API ACCLWorldPickup : public AActor
{
	GENERATED_BODY()

public:
	ACCLWorldPickup();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	bool TryCollect(UCCLInventoryComponent* Inventory, AActor* Collector);

public:
	UPROPERTY(EditAnywhere, Replicated, Category = "Pickup")
	TObjectPtr<UCCLItemDefinition> Definition;

	UPROPERTY(EditAnywhere, Replicated, Category = "Pickup", meta = (ClampMin = "1"))
	int32 Quantity = 1;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	uint8 bCollected = 0;
};
