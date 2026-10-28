#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "CCLInventoryNetworkSmoke.generated.h"
class UCCLInventoryComponent;

UCLASS()
class ACCLInventoryNetworkProbe : public AActor
{
    GENERATED_BODY()

public:
    ACCLInventoryNetworkProbe();
    UPROPERTY()
    TObjectPtr<UCCLInventoryComponent> Inventory;
};

UCLASS()
class UCCLInventoryNetworkSmoke : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
private:
    bool Seed(ACCLInventoryNetworkProbe& Probe, bool bUpdated);
    void Finish(bool bSuccess, const TCHAR* Reason);
    TWeakObjectPtr<ACCLInventoryNetworkProbe> Observed;
    FString Directory;
    FString Role;
    float Elapsed = 0.f;
    int32 Notifications = 0;
    uint8 bUpdated = 0;
    uint8 bFinished = 0;
};
