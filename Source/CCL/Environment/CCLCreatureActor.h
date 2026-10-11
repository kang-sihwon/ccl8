#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KSIEcologySimulation.h"
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
	void Bind(const FKSIEcologyLease& Lease, EKSIEcologyMovement Movement);
	EKSIEcologyMovement GetMovementMedium() const { return EKSIEcologyMovement(Medium); }
	FGuid GetIndividualId() const { return IndividualId; }
	const FKSIEcologyLease& GetLease() const { return Lease; }

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

	FKSIEcologyLease Lease;
};
