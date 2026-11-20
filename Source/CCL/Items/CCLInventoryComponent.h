#pragma once

#include "CoreMinimal.h"
#include "KSIInventoryComponent.h"
#include "CCLItemState.h"
#include "CCLFoodEnvironment.h"
#include "CCLInventoryComponent.generated.h"

class FKSIInventoryEdit;
class UKSIItemDefinition;

UCLASS(ClassGroup = Inventory, meta = (BlueprintSpawnableComponent))
class CCL_API UCCLInventoryComponent : public UKSIInventoryComponent
{
	GENERATED_BODY()

	// 저장 식별자와 음식 환경 바인딩은 두 영속성 서브시스템만 정한다. 항목은 편집 객체로만 바꾼다.
	friend class UCCLInventoryPersistenceSubsystem;
	friend class UCCLPlayerPersistenceSubsystem;

	// 부모 인터페이스 함수
public:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool CanAdd(const UKSIItemDefinition* Definition, int32 Quantity) const override;
	virtual bool CanRestore(const TArray<FKSIInventoryEntry>& Entries) const override;

protected:
	virtual bool StageAdd(TArray<FKSIInventoryEntry>& Entries, UKSIItemDefinition* Definition, int32 Quantity, FGuid& OutId) const override;
	virtual bool StageRemove(TArray<FKSIInventoryEntry>& Entries, FGuid Id, int32 Quantity) const override;
	virtual bool CanStackEntry(const FKSIInventoryEntry& Entry) const override;
	virtual bool CanUsePlainTransfer(const FKSIInventoryEntry& Entry) const override;

	// 내 클래스 함수
public:
	bool BindPersistence(FGuid ContainerId, FString& Error);

	// 이 인벤토리의 편집을 확정하면서 음식 환경 바인딩을 함께 바꾼다. 확정에 실패하면 바인딩도 그대로다.
	bool CommitWithFoodEnvironment(FKSIInventoryEdit& Edit, const FCCLFoodEnvironmentBinding& Binding);

	static bool ValidateEntries(const TArray<FKSIInventoryEntry>& Entries, int32 Capacity);

	FGuid GetPersistenceId() const { return PersistenceId; }
	const FCCLFoodEnvironmentBinding& GetFoodEnvironment() const { return FoodEnvironment; }

	// True when a world container identity or a registered player profile persists this inventory.
	bool HasPersistentOwner() const { return PersistenceId.IsValid() || bExternalPersistence != 0; }

	// 프로퍼티
private:
	FGuid PersistenceId;
	FCCLFoodEnvironmentBinding FoodEnvironment;
	uint8 bExternalPersistence = 0;
};
