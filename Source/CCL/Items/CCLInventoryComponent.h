#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	FGuid Add(UCCLItemDefinition* Definition, int32 Quantity);
	bool Remove(FGuid Id, int32 Quantity);
	bool CanAdd(const UCCLItemDefinition* Definition, int32 Quantity) const;
	const FCCLInventoryEntry* Find(FGuid Id) const;
	const TArray<FCCLInventoryEntry>& GetEntries() const { return List.Entries; }

public:
	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (ClampMin = "1", ClampMax = "128"))
	int32 Capacity = 16;

	FCCLInventoryChanged OnChanged;

private:
	UPROPERTY(Replicated)
	FCCLInventoryList List;
};
