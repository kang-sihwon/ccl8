#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CCLBiologyExperiment.generated.h"

class ACCLLifeVillager;
struct FCCLWorldSnapshot;
enum class ECCLExperimentAction : uint8;

USTRUCT()
struct FCCLBiologyExperimentRow
{
    GENERATED_BODY()
    UPROPERTY() uint8 bRecorded = 0;
    UPROPERTY() uint8 bProtected = 0;
    UPROPERTY() uint8 DeathCause = 0;
    UPROPERTY() double Age = 0.;
    UPROPERTY() double Lifespan = 0.;
    UPROPERTY() float Health = 0.f;
};

USTRUCT()
struct FCCLBiologyExperimentView
{
    GENERATED_BODY()
    UPROPERTY() FGuid Epoch;
    UPROPERTY() double WorldSeconds = 0.;
    UPROPERTY() TArray<FCCLBiologyExperimentRow> Rows;
};

UCLASS()
class CCL_API ACCLBiologyExperiment : public AActor
{
    GENERATED_BODY()
public:
    ACCLBiologyExperiment();
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    bool Initialize(bool bSeed, FCCLWorldSnapshot& Baseline, FString& Error);
    bool Execute(ECCLExperimentAction Action, FString& Error);
    const FCCLBiologyExperimentView& GetView() const { return View; }
    ACCLLifeVillager* FindActor(int32 Index) const;
    static FGuid AgentId(int32 Index) { return FGuid(7077, 7, 7, Index + 1); }
    static ACCLBiologyExperiment* Find(const UWorld* World);

private:
    void Reconcile();
    void UpdateView();
    UPROPERTY(Replicated) FCCLBiologyExperimentView View;
    FGuid ActorEpoch;
    uint8 bInitialized = 0;
};
