#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KSIEcologySimulation.h"
#include "CCLCreatureActor.generated.h"

class UStaticMeshComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UAnimationAsset;
class ACCLWorldEnvironmentState;

USTRUCT(BlueprintType)
struct FCCLCreatureVisual
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimationAsset> Move;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimationAsset> Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform Transform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double GaitSpeedCmPerSecond = 100.;
};

UCLASS()
class CCL_API ACCLCreatureActor : public AActor
{
	GENERATED_BODY()

public:
	ACCLCreatureActor();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
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
	TObjectPtr<USceneComponent> VisualRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> CreatureMesh;

	UPROPERTY()
	FCCLCreatureVisual Visual;

	UPROPERTY()
	TObjectPtr<UAnimationAsset> CurrentAnimation;

	UPROPERTY()
	TWeakObjectPtr<ACCLWorldEnvironmentState> WorldTime;

	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UStaticMeshComponent>> Shapes;

	UPROPERTY(Replicated)
	FGuid IndividualId;

	UPROPERTY(ReplicatedUsing=OnRep_Medium)
	uint8 Medium = 0;

	FKSIEcologyLease Lease;
	FVector VisualPosition = FVector::ZeroVector;
	FVector SegmentStart = FVector::ZeroVector;
	FVector LastTarget = FVector::ZeroVector;
	FRotator VisualRotation = FRotator::ZeroRotator;
	double SegmentElapsed = 0.;
	double SegmentDuration = 1.;
	double LastUpdateSeconds = -1.;
	double AnimationSeconds = 0.;
	uint8 bVisualInitialized = 0;
};
