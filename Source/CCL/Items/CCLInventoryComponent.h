#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "CCLFoodState.h"
#include "CCLFoodEnvironment.h"
#include "CCLInventoryComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FCCLInventoryChanged);

class UCCLItemDefinition;

USTRUCT()
struct FCCLInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid Id;

	UPROPERTY()
	TObjectPtr<UCCLItemDefinition> Definition;

	UPROPERTY()
	int32 Quantity = 0;

	UPROPERTY()
	int32 Slot = INDEX_NONE;

	UPROPERTY()
	int32 LoadedAmmo = 0;

	UPROPERTY()
	FCCLFoodState Food;
};

USTRUCT()
struct FCCLInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& Info)
	{
		return FastArrayDeltaSerialize<FCCLInventoryEntry, FCCLInventoryList>(Entries, Info, *this);
	}

	UPROPERTY()
	TArray<FCCLInventoryEntry> Entries;
};

template <>
struct TStructOpsTypeTraits<FCCLInventoryList> : TStructOpsTypeTraitsBase2<FCCLInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};

UCLASS(ClassGroup = Inventory, meta = (BlueprintSpawnableComponent))
class CCL_API UCCLInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCCLInventoryComponent();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	bool Restore(const TArray<FCCLInventoryEntry>& Entries);
	bool CanRestore(const TArray<FCCLInventoryEntry>& Entries) const;
	static bool ValidateEntries(const TArray<FCCLInventoryEntry>& Entries, int32 Capacity);
	bool BindPersistence(FGuid ContainerId, FString& Error);
	FGuid GetPersistenceId() const { return PersistenceId; }
	bool IsTransactionOpen() const { return bTransactionOpen != 0; }
	FGuid Add(UCCLItemDefinition* Definition, int32 Quantity);
	bool Remove(FGuid Id, int32 Quantity);
	FGuid AddFood(UCCLItemDefinition* Definition, const FCCLFoodState& Food);
	bool MergeFood(FGuid SourceId, FGuid DestinationId);
	FGuid SplitFood(FGuid SourceId, int32 Quantity, int32 Slot);
	bool TransferFood(FGuid SourceId, int32 Quantity, UCCLInventoryComponent* Destination);
	bool ChangeFoodStorage(FGuid Id, const FCCLFoodStorage& Storage);
    bool ChangeFoodEnvironment(const FCCLFoodEnvironmentBinding& Binding, FString& Error);
    const FCCLFoodEnvironmentBinding& GetFoodEnvironment() const { return FoodEnvironment; }
	bool GetFoodQuality(FGuid Id, FCCLFoodQuality& Quality) const;
	bool MoveToSlot(FGuid Id, int32 Slot);
	bool ConsumeShot(FGuid Id);
	bool Reload(FGuid Id);
	bool CanFire(FGuid Id) const;
	bool CanReload(FGuid Id) const;
	bool BeginTransaction();
	void EndTransaction(bool bPublish);

private:
    void RestoreValidated(const TArray<FCCLInventoryEntry>& Entries);
	void NotifyChanged();
	bool FoodContext(FGuid& WorldId, double& WorldSeconds) const;
	bool BuildFoodAddition(TArray<FCCLInventoryEntry>& Entries, UCCLItemDefinition* Definition,
		const FCCLFoodState& Food, double WorldSeconds, FGuid& AddedId) const;

public:
	const FCCLInventoryEntry* FindSlot(int32 Slot) const;
	bool CanAdd(const UCCLItemDefinition* Definition, int32 Quantity) const;
	const FCCLInventoryEntry* Find(FGuid Id) const;
	const TArray<FCCLInventoryEntry>& GetEntries() const { return List.Entries; }

public:
	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (ClampMin = "1", ClampMax = "128"))
	int32 Capacity = 16;

	FCCLInventoryChanged OnChanged;

private:
	friend class UCCLInventoryPersistenceSubsystem;
    friend class FCCLLoadoutRestoreBatch;
    friend class UCCLPlayerPersistenceSubsystem;
	FGuid PersistenceId;
    FCCLFoodEnvironmentBinding FoodEnvironment;
	UPROPERTY(Replicated)
	FCCLInventoryList List;
	uint8 bTransactionOpen = 0;
	uint8 bPendingChanged = 0;
};
