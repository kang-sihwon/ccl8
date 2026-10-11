#pragma once

#include "CoreMinimal.h"
#include "KSIInventoryComponent.h"
#include "CCLItemState.h"
#include "CCLFoodEnvironment.h"
#include "CCLInventoryComponent.generated.h"

class UKSIItemDefinition;

UCLASS(ClassGroup = Inventory, meta = (BlueprintSpawnableComponent))
class CCL_API UCCLInventoryComponent : public UKSIInventoryComponent
{
	GENERATED_BODY()

public:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual bool CanRestore(const TArray<FKSIInventoryEntry>& Entries) const override;
	static bool ValidateEntries(const TArray<FKSIInventoryEntry>& Entries, int32 Capacity);
	bool BindPersistence(FGuid ContainerId, FString& Error);
	FGuid GetPersistenceId() const { return PersistenceId; }
	// True when a world container identity or a registered player profile persists this inventory.
	bool HasPersistentOwner() const { return PersistenceId.IsValid() || bExternalPersistence != 0; }
	virtual FGuid Add(UKSIItemDefinition* Definition, int32 Quantity) override;
	virtual bool Remove(FGuid Id, int32 Quantity) override;
	const FCCLFoodEnvironmentBinding& GetFoodEnvironment() const { return FoodEnvironment; }

public:
	virtual bool CanAdd(const UKSIItemDefinition* Definition, int32 Quantity) const override;

protected:
    virtual bool CanStackEntry(const FKSIInventoryEntry& Entry) const override;
    virtual bool CanUsePlainTransfer(const FKSIInventoryEntry& Entry) const override;

private:
	friend class UCCLLoadoutComponent;
	friend class FCCLInventoryFood;
	friend class FCCLInventoryWeapon;
	friend class UCCLInventoryPersistenceSubsystem;
	friend class FCCLLoadoutRestoreBatch;
	friend class UCCLPlayerPersistenceSubsystem;
	FGuid PersistenceId;
	FCCLFoodEnvironmentBinding FoodEnvironment;
	uint8 bExternalPersistence = 0;
};
