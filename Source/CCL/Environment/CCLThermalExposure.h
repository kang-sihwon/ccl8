#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Subsystems/WorldSubsystem.h"
#include "CCLThermalExposure.generated.h"

class FCCLSurfaceSimulation;
class ICCLSurfaceProvider;
class UCCLAbilitySystemComponent;
struct FKSIWorldStep;

USTRUCT(BlueprintType)
struct CCL_API FCCLThermalDamagePolicy
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	double ThresholdCelsius = 60.;

	UPROPERTY(EditAnywhere)
	double ContactDamagePerDegreeSecond = .05;

	UPROPERTY(EditAnywhere)
	double AirDamagePerDegreeSecond = .1;

	UPROPERTY(EditAnywhere)
	double MaximumDamagePerSecond = 100.;

	UPROPERTY(EditAnywhere)
	double ContactHeightMeters = 2.;

	bool Validate() const;
};

namespace CCLThermalExposure
{
bool Sample(const FCCLSurfaceSimulation& Surface, const FVector& PositionMeters, const FCCLThermalDamagePolicy& Policy,
			const ICCLSurfaceProvider* Shelter, FName BodyId, double& DamagePerSecond, FString& Error);
}

UCLASS(ClassGroup = (Environment), meta = (BlueprintSpawnableComponent))
class CCL_API UCCLThermalReceiverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

public:
	UPROPERTY(EditAnywhere)
	FCCLThermalDamagePolicy Policy;

	UPROPERTY(EditAnywhere)
	uint8 bEnabled = 1;
};

struct FCCLPendingThermalDamage
{
	TWeakObjectPtr<UCCLThermalReceiverComponent> Receiver;
	TWeakObjectPtr<UCCLAbilitySystemComponent> AbilitySystem;
	float Damage = 0.f;
};

UCLASS()
class CCL_API UCCLThermalExposureSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

public:
	void Register(UCCLThermalReceiverComponent* Receiver);
	void Unregister(UCCLThermalReceiverComponent* Receiver);
	bool Prepare(const FKSIWorldStep& Step, const FCCLSurfaceSimulation& Previous, FString& Error);
	void Commit();
	void Cancel();

private:
	TArray<TWeakObjectPtr<UCCLThermalReceiverComponent>> Receivers;
	TArray<FCCLPendingThermalDamage> Pending;
	FGuid PendingEpoch;
	uint64 PendingStep = 0;
};
