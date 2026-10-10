#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CCLPlayerSnapshot.h"
#include "Items/CCLLoadoutRestore.h"
#include "CCLPlayerPersistenceSubsystem.generated.h"

class ACCLPlayerState;
class FCCLSurfaceSimulation;
struct FCCLWorldStep;

UCLASS()
class CCL_API UCCLPlayerPersistenceSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    // Trusted server identity binding, never an RPC or client identity assertion.
    bool Bind(ACCLPlayerState& Player, const FString& IdentityKey, FString& Error);
    void Unbind(ACCLPlayerState& Player);
    bool OwnsInventory(const UCCLInventoryComponent& Inventory) const;
    bool Capture(FGuid WorldId, double WorldSeconds, TArray<FCCLPlayerSnapshot>& Output,
                 TArray<UCCLInventoryComponent*>& Participants, FString& Error,
                 const UCCLInventoryComponent* SessionInventory = nullptr) const;

private:
    friend class UCCLWorldSimulationSubsystem;
    bool CanInitialize() const;
    void InitializeRecords(const TArray<FCCLPlayerSnapshot>& Input);
    bool Restore(const TArray<FCCLPlayerSnapshot>& Input, FGuid WorldId, double WorldSeconds,
                 const FCCLLoadoutRestoreRequest* SessionRequest, TFunctionRef<bool()> CommitWorld,
                 TFunctionRef<void()> PublishWorld, FString& Error);
    bool HasEnvironmentBindings() const;
    bool PrepareAdvance(const FCCLWorldStep& Step, const FCCLSurfaceSimulation& Surface, FString& Error);
    void CommitAdvance();
    void CancelAdvance();
    void GetParticipants(TArray<UCCLInventoryComponent*>& Output) const;
    bool CapturePlayer(const FString& Key, ACCLPlayerState& Player, FCCLPlayerSnapshot& Output, FString& Error) const;
    bool Resolve(const FCCLPlayerSnapshot& Profile, ACCLPlayerState& Player, FCCLLoadoutRestoreRequest& Request,
                 FString& Error) const;
    void Finalize(ACCLPlayerState& Player, const FCCLPlayerSnapshot& Profile);

private:
    TMap<FString, FCCLPlayerSnapshot> Records;
    TMap<FString, TWeakObjectPtr<ACCLPlayerState>> Live;
    TMap<FString, FCCLPlayerSnapshot> Prepared;
    TMap<TWeakObjectPtr<UCCLInventoryComponent>, TArray<FCCLInventoryEntry>> Pending;
    TArray<TWeakObjectPtr<UCCLInventoryComponent>> Locked;
    uint8 bPreparing = 0;
    uint8 bCaptureFailed = 0;
};
