#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/CCLInventorySnapshot.h"
#include "CCLFoodExperiment.generated.h"

class ACCLWorldPickup;
class UCCLInventoryComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UCCLItemDefinition;
struct FCCLWorldSnapshot;
enum class ECCLExperimentAction : uint8;

USTRUCT()
struct FCCLFoodExperimentRow
{
	GENERATED_BODY()
	UPROPERTY() uint8 EnvironmentSource = 0;
	UPROPERTY() int32 Quantity = 0;
	UPROPERTY() int32 Lots = 0;
	UPROPERTY() int32 Spoiled = 0;
	UPROPERTY() int32 Moldy = 0;
	UPROPERTY() double Freshness = 1.;
	UPROPERTY() double Mold = 0.;
	UPROPERTY() double AgeSeconds = 0.;
	UPROPERTY() double Temperature = 20.;
	UPROPERTY() double Humidity = 1.;
	UPROPERTY() double SpoilageMultiplier = 1.;
	UPROPERTY() double MoldExposure = 1.;
};

USTRUCT()
struct FCCLFoodExperimentView
{
	GENERATED_BODY()
	UPROPERTY() FGuid WorldId;
	UPROPERTY() FGuid Epoch;
	UPROPERTY() double WorldSeconds = 0.;
	UPROPERTY() TArray<FCCLFoodExperimentRow> Rows;
	UPROPERTY() int32 GroundQuantity = 0;
};

UCLASS()
class CCL_API ACCLFoodExperiment : public AActor
{
	GENERATED_BODY()
public:
	ACCLFoodExperiment();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetActorEyesViewPoint(FVector& Location, FRotator& Rotation) const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
public:
	bool Initialize(bool bSeed, FCCLWorldSnapshot& Baseline, FString& Error);
	bool Execute(ECCLExperimentAction Action, FString& Error);
	const FCCLFoodExperimentView& GetView() const { return View; }
	ACCLWorldPickup* FindGroundSample() const;
	FVector GetGroundPosition() const { return GetActorLocation() + FVector(130., -150., 25.); }
	UCCLInventoryComponent* GetInventory(int32 Index) const;
	static FGuid ContainerId(int32 Index) { return FGuid(7071, 7072, 7073, Index + 1); }
	static ACCLFoodExperiment* Find(const UWorld* World);
	static FString StorageName(int32 Index);
	int32 GetRenderedCount() const;
private:
	bool MakeInitial(FGuid WorldId, double WorldSeconds, TArray<FCCLInventorySnapshot>& Records, FString& Error) const;
	void UpdateView();
	void UpdatePresentation();
	UFUNCTION() void OnRep_View();
	UCCLItemDefinition* SampleDefinition() const;
private:
	UPROPERTY() TArray<TObjectPtr<UCCLInventoryComponent>> Inventories;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Samples;
	UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> Labels;
	UPROPERTY(ReplicatedUsing = OnRep_View) FCCLFoodExperimentView View;
	uint8 bInitialized = 0;
};
