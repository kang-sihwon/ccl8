#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CCLAccountComponent.generated.h"

class UCCLInventoryComponent;
class UCCLItemDefinition;
class FCCLLifeSimulation;

UCLASS(ClassGroup = Economy, meta = (BlueprintSpawnableComponent))
class CCL_API UCCLAccountComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCCLAccountComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	bool ImportLegacy(int64 Coins);
	bool BindAccount(FGuid Id);
	bool Reward(int64 Amount);
	bool Purchase(UCCLInventoryComponent* Inventory, UCCLItemDefinition* Item, int64 Price);
	int64 GetBalance() const;
	FGuid GetAccountId() const { return AccountId; }

private:
	FCCLLifeSimulation* Resolve() const;
	void RefreshView();

private:
	UPROPERTY(Replicated)
	FGuid AccountId;

	// Owner-only display cache. Server reads the shared account ledger instead.
	UPROPERTY(Replicated)
	int64 BalanceView = 30;
};
