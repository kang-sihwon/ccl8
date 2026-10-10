#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CCLEcologySimulation.h"
#include "CCLCreatureActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class CCL_API ACCLCreatureActor : public AActor
{
	GENERATED_BODY()

public:
	ACCLCreatureActor();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	void Bind(const FCCLEcologyLease& Lease, ECCLEcologyMovement Movement);
	ECCLEcologyMovement GetMovementMedium() const { return ECCLEcologyMovement(Medium); }
	FGuid GetIndividualId() const { return IndividualId; }
	const FCCLEcologyLease& GetLease() const { return Lease; }

private:
	UFUNCTION()
	void OnRep_Medium();

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UStaticMeshComponent>> Shapes;

	UPROPERTY(Replicated)
	FGuid IndividualId;

	UPROPERTY(ReplicatedUsing=OnRep_Medium)
	uint8 Medium = 0;

	FCCLEcologyLease Lease;
};
