#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CCLEconomy.generated.h"

USTRUCT()
struct CCL_API FCCLAccountRecord
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid Id;

	UPROPERTY()
	FGuid OwnerId;

	UPROPERTY()
	int64 Balance = 0;

	// Immutable opening allocation is audit metadata, not another current balance.
	UPROPERTY()
	int64 OpeningBalance = 0;

	UPROPERTY()
	uint8 bHasOpeningBalance = 0;
};

USTRUCT()
struct CCL_API FCCLResourceInventory
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid Id;

	UPROPERTY()
	FGuid OwnerId;

	UPROPERTY()
	int64 Capacity = 100;

	UPROPERTY()
	TMap<FGameplayTag, int64> Resources;
};

USTRUCT()
struct CCL_API FCCLObligationRecord
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid ObligationId;

	UPROPERTY()
	FGuid DebtorId;

	UPROPERTY()
	FGuid CreditorId;

	UPROPERTY()
	int64 RemainingAmount = 0;

	UPROPERTY()
	int64 OriginalAmount = 0;

	UPROPERTY()
	double DueTime = 0;

	UPROPERTY()
	FGameplayTag Status;
};

USTRUCT()
struct CCL_API FCCLOwnershipRecord
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid Id;

	UPROPERTY()
	FGuid PlaceId;

	UPROPERTY()
	FGuid OwnerId;

	UPROPERTY()
	TArray<FGuid> AuthorizedUsers;

	UPROPERTY()
	int32 ImprovementLevel = 0;
};

USTRUCT()
struct CCL_API FCCLItemTransfer
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid Source;

	UPROPERTY()
	FGuid Destination;

	// Adapter receipt for resources delivered into an existing item inventory.
	// These IDs describe a transfer; they are not another copy of inventory quantity.
	UPROPERTY()
	FGuid ExternalInventoryOwner;

	UPROPERTY()
	FGuid ExternalItem;

	UPROPERTY()
	FGameplayTag Resource;

	UPROPERTY()
	int64 Quantity = 0;
};

USTRUCT()
struct CCL_API FCCLTransactionRequest
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid RequestId;

	UPROPERTY()
	FGuid Buyer;

	UPROPERTY()
	FGuid Seller;

	UPROPERTY()
	TArray<FCCLItemTransfer> Items;

	UPROPERTY()
	int64 Price = 0;

	UPROPERTY()
	FGuid ObligationId;

	UPROPERTY()
	FGameplayTag Reason;

	UPROPERTY()
	double Time = 0;
};

USTRUCT()
struct CCL_API FCCLTransactionReceipt
{
	GENERATED_BODY()

	UPROPERTY()
	FCCLTransactionRequest Request;

	UPROPERTY()
	uint8 bSucceeded = 0;

	UPROPERTY()
	FString Failure;
};

USTRUCT()
struct CCL_API FCCLEconomyState
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<FGuid, FCCLAccountRecord> Accounts;

	UPROPERTY()
	TMap<FGuid, FCCLResourceInventory> Inventories;

	UPROPERTY()
	TMap<FGuid, FCCLObligationRecord> Obligations;

	UPROPERTY()
	TMap<FGuid, FCCLOwnershipRecord> Ownerships;

	UPROPERTY()
	TArray<FCCLTransactionReceipt> Journal;
};

namespace CCLEconomy
{
// Server-created requests only. Opportunity execution supplies authorization and current quoted terms.
CCL_API FCCLTransactionReceipt Execute(FCCLEconomyState& State, const FCCLTransactionRequest& Request);
CCL_API bool Validate(const FCCLEconomyState& State, FString& Error);
CCL_API int64 Quantity(const FCCLEconomyState& State, FGuid Inventory, FGameplayTag Resource);
CCL_API int64 UsedCapacity(const FCCLResourceInventory& Inventory);
}
