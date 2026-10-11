#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "KSIItemDefinition.h"
#include "KSIInventoryComponent.generated.h"
DECLARE_MULTICAST_DELEGATE(FKSIInventoryChanged);

USTRUCT()
struct KSIINVENTORY_API FKSIInventoryEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()


    template <class T> const T& GetState() const
    {
        const T* Value = State.GetPtr<T>();
        static const T Empty;
        return Value ? *Value : Empty;
    }
    template <class T> T& GetState()
    {
        if (!State.IsValid())
        {
            State.InitializeAs<T>();
        }
        return State.GetMutable<T>();
    }

    UPROPERTY()
    FGuid Id;

    UPROPERTY()
    TObjectPtr<UKSIItemDefinition> Definition;

    UPROPERTY()
    int32 Quantity = 0;

    UPROPERTY()
    int32 Slot = INDEX_NONE;

    UPROPERTY()
    FInstancedStruct State;


};

USTRUCT()
struct KSIINVENTORY_API FKSIInventoryList : public FFastArraySerializer
{
    GENERATED_BODY()

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& Info)
    {
        return FastArrayDeltaSerialize<FKSIInventoryEntry, FKSIInventoryList>(Entries, Info, *this);
    }

    UPROPERTY()
    TArray<FKSIInventoryEntry> Entries;
};

template <>
struct TStructOpsTypeTraits<FKSIInventoryList> : TStructOpsTypeTraitsBase2<FKSIInventoryList>
{
    enum { WithNetDeltaSerializer = true };
};

UCLASS(ClassGroup = Inventory, meta = (BlueprintSpawnableComponent))
class KSIINVENTORY_API UKSIInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UKSIInventoryComponent();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
public:
    bool Restore(const TArray<FKSIInventoryEntry>& Entries);
    virtual bool CanRestore(const TArray<FKSIInventoryEntry>& Entries) const;
    static bool ValidateEntries(const TArray<FKSIInventoryEntry>& Entries, int32 Capacity);
    virtual FGuid Add(UKSIItemDefinition* Definition, int32 Quantity);
    virtual bool Remove(FGuid Id, int32 Quantity);
    virtual bool CanAdd(const UKSIItemDefinition* Definition, int32 Quantity) const;
    bool MoveToSlot(FGuid Id, int32 Slot);
    // Transfers plain entries. Extensions must supply their own state-aware transfer operation.
    bool TransferTo(FGuid Id, int32 Quantity, UKSIInventoryComponent& Destination);
    bool Capture(TArray<uint8>& OutBytes) const;
    bool RestoreSnapshot(const TArray<uint8>& Bytes, TFunctionRef<UKSIItemDefinition*(const FString&)> ResolveDefinition);
    bool CanMutate() const;
    bool BeginTransaction();
    void EndTransaction(bool bPublish);
    bool IsTransactionOpen() const { return bTransactionOpen != 0; }
    const FKSIInventoryEntry* FindSlot(int32 Slot) const;
    const FKSIInventoryEntry* Find(FGuid Id) const;
    const TArray<FKSIInventoryEntry>& GetEntries() const { return List.Entries; }
protected:
    void RestoreValidated(const TArray<FKSIInventoryEntry>& Entries);
    void NotifyChanged();
    void FinishTransaction(bool bPublish);
    // Save bytes are bounded and interpreted by the owning extension, never loaded as an arbitrary type path.
    virtual bool EncodeState(const FKSIInventoryEntry& Entry, TArray<uint8>& OutBytes) const;
    virtual bool DecodeState(const TArray<uint8>& Bytes, FKSIInventoryEntry& Entry) const;
    virtual bool CanStackEntry(const FKSIInventoryEntry& Entry) const;
    virtual bool CanUsePlainTransfer(const FKSIInventoryEntry& Entry) const;
private:
    UFUNCTION()
    void OnRep_List();

public:
    UPROPERTY(EditAnywhere, Category = "Inventory", meta = (ClampMin = "1", ClampMax = "128"))
    int32 Capacity = 16;
    FKSIInventoryChanged OnChanged;
protected:
    UPROPERTY(ReplicatedUsing = OnRep_List)
    FKSIInventoryList List;
    uint8 bTransactionOpen = 0;
    uint8 bMutationLocked = 0;
    uint8 bPendingChanged = 0;
};
