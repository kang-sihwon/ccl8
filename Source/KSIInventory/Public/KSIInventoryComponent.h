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

	template <class T>
	const T& GetState() const
	{
		const T* Value = State.GetPtr<T>();
		static const T Empty;
		return Value ? *Value : Empty;
	}

	template <class T>
	T& GetState()
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

	// 가방 칸 번호다. INDEX_NONE이면 소유하지만 가방 밖에 있다(장착 중 등).
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
	enum
	{
		WithNetDeltaSerializer = true
	};
};

// 항목은 FKSIInventoryEdit로만 바뀐다. Add, Remove 같은 단일 명령도 안에서 편집 객체를 연다.
UCLASS(ClassGroup = Inventory, meta = (BlueprintSpawnableComponent))
class KSIINVENTORY_API UKSIInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

	friend class FKSIInventoryEdit;

	// 부모 인터페이스 함수
public:
	UKSIInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 내 클래스 함수
public:
	FGuid Add(UKSIItemDefinition* Definition, int32 Quantity);
	bool Remove(FGuid Id, int32 Quantity);
	bool MoveToSlot(FGuid Id, int32 Slot);

	// 전체 항목을 검증한 뒤 한 번에 바꾼다.
	bool Restore(const TArray<FKSIInventoryEntry>& Entries);

	// 상태 없는 항목만 옮긴다. 상태가 있는 항목은 확장 모듈이 두 인벤토리의 편집 객체로 옮긴다.
	bool TransferTo(FGuid Id, int32 Quantity, UKSIInventoryComponent& Destination);

	bool Capture(TArray<uint8>& OutBytes) const;
	bool RestoreSnapshot(const TArray<uint8>& Bytes, TFunctionRef<UKSIItemDefinition*(const FString&)> ResolveDefinition);

	virtual bool CanAdd(const UKSIItemDefinition* Definition, int32 Quantity) const;
	virtual bool CanRestore(const TArray<FKSIInventoryEntry>& Entries) const;
	static bool ValidateEntries(const TArray<FKSIInventoryEntry>& Entries, int32 Capacity);

	// 서버 권위가 있고 열린 편집 객체가 없을 때만 참이다.
	bool CanMutate() const;

	// 편집 객체가 열려 있으면 다른 쓰기와 저장 캡처를 거절한다.
	bool IsEditOpen() const { return bEditOpen != 0; }

	const FKSIInventoryEntry* Find(FGuid Id) const;
	const FKSIInventoryEntry* FindSlot(int32 Slot) const;
	const TArray<FKSIInventoryEntry>& GetEntries() const { return List.Entries; }
	int32 GetSlotCount() const { return FMath::Clamp(Capacity, 1, 128); }

protected:
	// 편집 객체의 Add와 Remove가 후보 항목에 적용하는 규칙이다. 확장 모듈은 상태가 있는 항목의 규칙을 더한다.
	virtual bool StageAdd(TArray<FKSIInventoryEntry>& Entries, UKSIItemDefinition* Definition, int32 Quantity, FGuid& OutId) const;
	virtual bool StageRemove(TArray<FKSIInventoryEntry>& Entries, FGuid Id, int32 Quantity) const;

	// Save bytes are bounded and interpreted by the owning extension, never loaded as an arbitrary type path.
	virtual bool EncodeState(const FKSIInventoryEntry& Entry, TArray<uint8>& OutBytes) const;
	virtual bool DecodeState(const TArray<uint8>& Bytes, FKSIInventoryEntry& Entry) const;
	virtual bool CanStackEntry(const FKSIInventoryEntry& Entry) const;
	virtual bool CanUsePlainTransfer(const FKSIInventoryEntry& Entry) const;

private:
	UFUNCTION()
	void OnRep_List();

	// 검증된 후보를 복제 목록에 반영한다. 바뀐 항목만 더럽게 표시하고 기존 항목의 복제 ID를 유지한다.
	void ApplyEdit(const TArray<FKSIInventoryEntry>& Entries);

	// 프로퍼티
public:
	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (ClampMin = "1", ClampMax = "128"))
	int32 Capacity = 16;

	FKSIInventoryChanged OnChanged;

private:
	UPROPERTY(ReplicatedUsing = OnRep_List)
	FKSIInventoryList List;

	uint8 bEditOpen = 0;
};
