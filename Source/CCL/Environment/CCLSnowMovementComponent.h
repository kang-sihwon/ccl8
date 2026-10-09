#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/CharacterMovementReplication.h"
#include "CCLSnowMovementComponent.generated.h"

class UInstancedStaticMeshComponent;

struct FCCLSnowNetworkMoveData : public FCharacterNetworkMoveData
{
	uint64 SurfaceSerial = 0;
	uint64 ContactSequence = 0;
	virtual void ClientFillNetworkMoveData(const FSavedMove_Character& Move, ENetworkMoveType MoveType) override;
	virtual bool Serialize(UCharacterMovementComponent& Movement, FArchive& Ar, UPackageMap* Map, ENetworkMoveType MoveType) override;
};

struct FCCLSnowMoveDataContainer : public FCharacterNetworkMoveDataContainer
{
	FCCLSnowMoveDataContainer();
	FCCLSnowNetworkMoveData Moves[3];
};

UCLASS()
class CCL_API UCCLSnowMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UCCLSnowMovementComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Props) const override;
	virtual float GetMaxSpeed() const override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual void PerformMovement(float DeltaSeconds) override;
	virtual void MoveAutonomous(float TimeStamp, float DeltaTime, uint8 Flags, const FVector& Accel) override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

	void PrepareSnowMove(const FVector& Direction = FVector::ZeroVector);
	void RestoreSnowMove(uint64 Serial, uint64 Sequence, float Depth);
	float GetSnowDepth() const { return SnowDepthMeters; }
	uint64 GetSurfaceSerial() const { return SurfaceSerial; }
	uint64 GetContactSequence() const { return ActiveSequence; }
	FGuid GetContactSource() const { return ContactSource; }
	uint32 GetSnowCorrections() const { return SnowCorrections; }
	uint32 GetSnowReplays() const { return SnowReplays; }

private:
	UPROPERTY(Replicated)
	FGuid ContactSource;
	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> Powder;
	FVector PowderOrigin = FVector::ZeroVector;
	double PowderTime = 0.;
	FCCLSnowMoveDataContainer MoveData;
	uint64 SurfaceSerial = 0;
	uint64 NextSequence = 0;
	uint64 ActiveSequence = 0;
	float SnowDepthMeters = 0.f;
	FVector LastContactLocation = FVector::ZeroVector;
	uint32 SnowCorrections = 0;
	uint32 SnowReplays = 0;
	uint8 bPreparedMove = 0;
	uint8 bHasContactLocation = 0;
};
