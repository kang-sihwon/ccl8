#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CCLInventorySnapshot.h"
#include "CCLInventoryComponent.h"
#include "Environment/CCLWorldParticipant.h"
#include "CCLInventoryPersistenceSubsystem.generated.h"

struct FCCLWorldStep;
class ACCLWorldPickup;

UCLASS()
class CCL_API UCCLInventoryPersistenceSubsystem : public UWorldSubsystem, public ICCLWorldParticipant
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual FName GetWorldSection() const override;
	virtual int32 GetWorldOrder() const override { return 1; }
	virtual bool CaptureWorldSection(FCCLWorldCaptureContext& Context, TArray<uint8>& OutBytes, FString& Error) override;
	virtual bool PrepareWorldRestore(FCCLWorldRestoreContext& Context, const TArray<uint8>* Section, FString& Error) override;
	virtual bool CommitWorldRestore(TFunctionRef<bool()> CommitInner) override;
	virtual void PublishWorldRestore() override { PublishRestore(); }
	virtual void CancelWorldRestore() override { CancelRestore(); }
	virtual bool NeedsWorldAdvance() const override { return HasEnvironmentBindings(); }
	virtual bool PrepareWorldAdvance(const FCCLWorldStep& Step, const FCCLSurfaceSimulation& Surface, FString& Error) override
	{
		return PrepareAdvance(Step, Surface, Error);
	}
	virtual void CommitWorldAdvance() override { CommitRestore(false); }
	virtual void PublishWorldAdvance() override { PublishRestore(); }
	virtual void CancelWorldAdvance() override { CancelRestore(); }

	bool Bind(UCCLInventoryComponent& Inventory, FGuid Id, FString& Error);
	void Unbind(UCCLInventoryComponent& Inventory);
	bool BindPickup(ACCLWorldPickup& Pickup, FString& Error, bool bCreatingEmpty = false);
	void CancelEmptyPickup(ACCLWorldPickup& Pickup);
	bool IsPreparing() const { return bPreparing != 0; }
	bool Capture(FGuid WorldId, double WorldSeconds, TArray<FCCLInventorySnapshot>& Output, FString& Error,
				 const UCCLInventoryComponent* SessionInventory = nullptr,
				 TConstArrayView<UCCLInventoryComponent*> PlayerInventories = {}) const;

private:
	bool HasEnvironmentBindings() const;
	bool PrepareAdvance(const FCCLWorldStep& Step, const FCCLSurfaceSimulation& Surface, FString& Error);
	bool PrepareRestore(const TArray<FCCLInventorySnapshot>& Input, FGuid WorldId, double WorldSeconds, FString& Error,
						TConstArrayView<UCCLInventoryComponent*> PlayerInventories = {});
	void CancelRestore();
	void CommitRestore(bool bPublish = true);
	void PublishRestore();
	bool CheckUnboundFood(FString& Error, const UCCLInventoryComponent* SessionInventory = nullptr,
				 TConstArrayView<UCCLInventoryComponent*> PlayerInventories = {}) const;

private:
	TMap<FGuid, FCCLInventorySnapshot> Records;
	TMap<FGuid, TWeakObjectPtr<UCCLInventoryComponent>> Live;
	TMap<FGuid, FCCLInventorySnapshot> Prepared;
	TMap<TWeakObjectPtr<UCCLInventoryComponent>, TArray<FCCLInventoryEntry>> Pending;
	TArray<TWeakObjectPtr<UCCLInventoryComponent>> Locked;
	TArray<TWeakObjectPtr<ACCLWorldPickup>> RetiredPickups;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ACCLWorldPickup>> SpawnedPickups;

	uint8 bPreparing = 0;
	uint8 bCaptureFailed = 0;
};
