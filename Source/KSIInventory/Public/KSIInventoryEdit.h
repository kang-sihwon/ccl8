#pragma once

#include "CoreMinimal.h"
#include "KSIInventoryComponent.h"

// 인벤토리를 바꾸는 유일한 경로다.
// 여는 동안 대상 인벤토리의 다른 쓰기를 막고, 후보 항목을 바꾼 뒤 Commit이 검증하고 한 번에 반영한다.
// 알림은 Publish나 소멸 때 한 번 보내고 잠금은 소멸 때 푼다. Commit하지 않고 소멸하면 아무것도 바뀌지 않는다.
// 여러 인벤토리를 함께 바꿀 때는 모두 Commit한 뒤 모두 Publish해서, 알림을 받는 쪽이 중간 상태를 보지 않게 한다.
class KSIINVENTORY_API FKSIInventoryEdit
{
	// 내 클래스 함수
public:
	explicit FKSIInventoryEdit(UKSIInventoryComponent& Inventory);
	~FKSIInventoryEdit();

	FKSIInventoryEdit(const FKSIInventoryEdit&) = delete;
	FKSIInventoryEdit& operator=(const FKSIInventoryEdit&) = delete;

	bool Add(UKSIItemDefinition* Definition, int32 Quantity, FGuid& OutId);
	bool Remove(FGuid Id, int32 Quantity);

	// 항목을 가방 칸으로 옮긴다. INDEX_NONE이면 가방 밖에 둔다. 다른 항목이 있는 칸이면 서로 바꾼다.
	bool Place(FGuid Id, int32 Slot);

	// 후보 전체를 바꾼다. 저장 복원처럼 결과 목록을 이미 가진 호출자가 쓴다.
	void Replace(TArray<FKSIInventoryEntry> Entries);

	// 후보를 대상 인벤토리의 규칙으로 검사한다. 편집이 열린 동안에는 다른 쓰기가 없으므로 Commit 결과와 같다.
	bool Validate() const;

	bool Commit();

	// Commit한 변경을 지금 알린다. 소멸할 때도 아직 알리지 않았으면 알린다.
	void Publish();

	FKSIInventoryEntry* Find(FGuid Id);

	// 비어 있는 가장 작은 가방 칸이다. 없으면 INDEX_NONE이다.
	int32 FindFreeSlot() const;

	bool IsOpen() const { return bOpen != 0; }
	bool IsCommitted() const { return bCommitted != 0; }
	TArray<FKSIInventoryEntry>& GetEntries() { return Staged; }
	const TArray<FKSIInventoryEntry>& GetEntries() const { return Staged; }
	UKSIInventoryComponent* GetInventory() const { return Inventory.Get(); }

	static int32 FindFreeSlot(const TArray<FKSIInventoryEntry>& Entries, int32 SlotCount);

	// 여러 호출에 걸쳐 편집을 붙잡는 코디네이터용이다. 모두 Commit한 뒤 하나씩 알리고 잠금을 푼다.
	static FKSIInventoryEdit* FindIn(TArrayView<const TUniquePtr<FKSIInventoryEdit>> Edits, const UKSIInventoryComponent* Inventory);
	static void PublishAndRelease(TArray<TUniquePtr<FKSIInventoryEdit>>& Edits);

private:
	bool CanStage() const;

	// 프로퍼티
private:
	TWeakObjectPtr<UKSIInventoryComponent> Inventory;
	TArray<FKSIInventoryEntry> Staged;
	uint8 bOpen = 0;
	uint8 bCommitted = 0;
	uint8 bPublished = 0;
};
