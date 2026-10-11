#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KSIEcologySimulation.h"
#include "KSISurfaceSimulation.h"
#include "CCLEcologyExperiment.generated.h"

class UTextRenderComponent;
class UStaticMeshComponent;
struct FCCLWorldSnapshot;
enum class ECCLExperimentAction : uint8;

USTRUCT()
struct FCCLEcologyExperimentRow
{
	GENERATED_BODY()

	UPROPERTY()
	int64 Living = 0;

	UPROPERTY()
	int64 Aggregate = 0;

	UPROPERTY()
	int32 Reserved = 0;

	UPROPERTY()
	int32 Actors = 0;

	UPROPERTY()
	int32 Mass = 0;

	UPROPERTY()
	int32 Blocked = 0;

	UPROPERTY()
	double Energy = 0.;

	UPROPERTY()
	double Consumed = 0.;

	UPROPERTY()
	double Food = 0.;

	UPROPERTY()
	double AggregateEnergy = 0.;

	UPROPERTY()
	double AggregateConsumed = 0.;

};

USTRUCT()
struct FCCLEcologyExperimentView
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid WorldId;

	UPROPERTY()
	FGuid Epoch;

	UPROPERTY()
	double WorldSeconds = 0.;

	UPROPERTY()
	TArray<FCCLEcologyExperimentRow> Rows;

};

UCLASS()
class CCL_API ACCLEcologyExperiment : public AActor
{
	GENERATED_BODY()
public:
	ACCLEcologyExperiment();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	bool Initialize(bool bSeed, FCCLWorldSnapshot& Baseline, FString& Error);
	bool Execute(ECCLExperimentAction Action, FString& Error);
	bool CheckState(FString& Error) const;
	const FCCLEcologyExperimentView& GetView() const { return View; }
	static ACCLEcologyExperiment* Find(const UWorld* World);
	static FGuid Id(uint32 N) { return FGuid(6011, 6012, 6013, N); }
	static FGuid IndividualId(uint32 Medium, uint32 Member) { return Id(100 + Medium * 3 + Member); }
	static FVector HabitatCenter(int32 Medium);
	static FString MediumName(int32 Medium);
	static FKSISurfaceGrid Pond();
	static bool MakeInitial(FGuid WorldId, double WorldSeconds, FKSIEcologySimulation& Out, FString& Error);

private:
	void Reconcile();
	void UpdateView();
	void UpdatePresentation();
	UFUNCTION()
	void OnRep_View();

private:
	UPROPERTY()
	TArray<TObjectPtr<UTextRenderComponent>> Labels;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> FoodMarkers;

	UPROPERTY(ReplicatedUsing = OnRep_View)
	FCCLEcologyExperimentView View;

	EKSIEcologyRepresentation Desired = EKSIEcologyRepresentation::Actor;
	FGuid SeenEpoch;
	uint8 bInitialized = 0;
};
