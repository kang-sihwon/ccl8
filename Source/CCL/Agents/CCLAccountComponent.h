#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CCLAccountComponent.generated.h"

class UCCLInventoryComponent;
class UKSIItemDefinition;
class UCCLItemDefinition;
class FCCLLifeSimulation;

// What a seller offers for coins. Vendors own offers; the account only executes the trade.
USTRUCT(BlueprintType)
struct CCL_API FCCLShopOffer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Shop")
	TSoftObjectPtr<UKSIItemDefinition> Item;

	UPROPERTY(EditAnywhere, Category = "Shop", meta = (ClampMin = "1", ClampMax = "1000000"))
	int32 Price = 10;

	// Economy resource that represents this item in the seller's stock.
	UPROPERTY(EditAnywhere, Category = "Shop", meta = (Categories = "Agent.Resource"))
	FGameplayTag StockResource;

	UPROPERTY(EditAnywhere, Category = "Shop", meta = (ClampMin = "1", ClampMax = "1000"))
	int32 InitialStock = 100;
};

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
	// Opens this account in the running world economy if a restore removed it, keeping the current balance.
	bool EnsureOpen();
	bool Reward(int64 Amount);
	bool Purchase(UCCLInventoryComponent* Inventory, const FCCLShopOffer& Offer);
	int64 GetBalance() const;
	FGuid GetAccountId() const { return AccountId; }

private:
	FCCLLifeSimulation* Resolve() const;
	bool InitializeAccount();
	void RefreshView();

private:
	FGuid InitializedWorldId;

	UPROPERTY(Replicated)
	FGuid AccountId;

	// Owner-only display cache. Server reads the shared account ledger instead.
	UPROPERTY(Replicated)
	int64 BalanceView = 30;
};
