#include "CCLEconomy.h"

#include "CCLAgentTags.h"

namespace
{
constexpr int64 Limit = 1000000000000LL;

bool SameRequest(const FCCLTransactionRequest& A, const FCCLTransactionRequest& B)
{
	if (A.Buyer != B.Buyer || A.Seller != B.Seller || A.Price != B.Price ||
		A.ObligationId != B.ObligationId || A.Reason != B.Reason || A.Time != B.Time || A.Items.Num() != B.Items.Num())
	{
		return false;
	}

	for (int32 Index = 0; Index < A.Items.Num(); ++Index)
	{
		const auto& X = A.Items[Index];
		const auto& Y = B.Items[Index];
		if (X.Source != Y.Source || X.Destination != Y.Destination || X.Resource != Y.Resource || X.Quantity != Y.Quantity ||
            X.ExternalInventoryOwner != Y.ExternalInventoryOwner || X.ExternalItem != Y.ExternalItem)
		{
			return false;
		}
	}

	return true;
}
}

FCCLTransactionReceipt CCLEconomy::Execute(FCCLEconomyState& State, const FCCLTransactionRequest& Request)
{
	FCCLTransactionReceipt Receipt;
	Receipt.Request = Request;
	const auto* Existing = State.Journal.FindByPredicate([&](const FCCLTransactionReceipt& R)
	{
		return R.Request.RequestId == Request.RequestId;
	});
	if (Existing)
	{
		if (SameRequest(Existing->Request, Request))
		{
			return *Existing;
		}

		Receipt.Failure = TEXT("Request ID reused with different terms.");
		return Receipt;
	}

	auto Fail = [&](const TCHAR* Reason)
	{
		Receipt.Failure = Reason;
		if (Request.RequestId.IsValid())
		{
			State.Journal.Add(Receipt);
		}

		return Receipt;
	};
	if (!Request.RequestId.IsValid() || !Request.Reason.IsValid() || !FMath::IsFinite(Request.Time) || Request.Time < 0 ||
		Request.Price < 0 || Request.Price > Limit || Request.Items.Num() > 64)
	{
		return Fail(TEXT("Invalid transaction terms."));
	}

	// Stage every affected balance and inventory before publishing any mutation or callback.
	TMap<FGuid, FCCLAccountRecord> Accounts;
	TMap<FGuid, FCCLResourceInventory> Inventories;
	FCCLObligationRecord Obligation;
	if (Request.Price > 0)
	{
		const auto* Buyer = State.Accounts.Find(Request.Buyer);
		const auto* Seller = State.Accounts.Find(Request.Seller);
		if (!Buyer || !Seller || Buyer->Id == Seller->Id || Buyer->Balance < Request.Price || Seller->Balance > Limit - Request.Price)
		{
			return Fail(TEXT("Insufficient balance or invalid accounts."));
		}

		Accounts.Add(Buyer->Id, *Buyer).Balance -= Request.Price;
		Accounts.Add(Seller->Id, *Seller).Balance += Request.Price;
	}

	if (Request.ObligationId.IsValid())
	{
		const auto* Debt = State.Obligations.Find(Request.ObligationId);
		const auto* Buyer = State.Accounts.Find(Request.Buyer);
		const auto* Seller = State.Accounts.Find(Request.Seller);
		if (!Debt || !Buyer || !Seller || Request.Price <= 0 || Request.Price > Debt->RemainingAmount ||
			Buyer->OwnerId != Debt->DebtorId || Seller->OwnerId != Debt->CreditorId || Debt->Status != CCLAgentTags::Active)
		{
			return Fail(TEXT("Invalid obligation payment."));
		}

		Obligation = *Debt;
		Obligation.RemainingAmount -= Request.Price;
		if (Obligation.RemainingAmount == 0)
		{
			Obligation.Status = CCLAgentTags::Completed;
		}
	}

	for (const auto& Item : Request.Items)
	{
		if (!Item.Resource.IsValid() || Item.Quantity <= 0 || Item.Quantity > Limit || Item.Source == Item.Destination ||
			(!Item.Source.IsValid() && !Item.Destination.IsValid()) ||
            (Item.ExternalItem.IsValid() != Item.ExternalInventoryOwner.IsValid()) ||
            (Item.ExternalItem.IsValid() && (Item.Destination.IsValid() || !Item.Source.IsValid())))
		{
			return Fail(TEXT("Invalid item transfer."));
		}

		for (const FGuid Id : {Item.Source, Item.Destination})
		{
			if (Id.IsValid() && !Inventories.Contains(Id))
			{
				const auto* Inventory = State.Inventories.Find(Id);
				if (!Inventory)
				{
					return Fail(TEXT("Unknown inventory."));
				}

				Inventories.Add(Id, *Inventory);
			}
		}

		if (Item.Source.IsValid())
		{
			auto& Quantity = Inventories[Item.Source].Resources.FindOrAdd(Item.Resource);
			if (Quantity < Item.Quantity)
			{
				return Fail(TEXT("Insufficient stock."));
			}

			Quantity -= Item.Quantity;
		}

		if (Item.Destination.IsValid())
		{
			auto& Quantity = Inventories[Item.Destination].Resources.FindOrAdd(Item.Resource);
			if (Quantity > Limit - Item.Quantity)
			{
				return Fail(TEXT("Resource quantity overflow."));
			}

			Quantity += Item.Quantity;
		}
	}

	for (const auto& Pair : Inventories)
	{
		if (UsedCapacity(Pair.Value) > Pair.Value.Capacity)
		{
			return Fail(TEXT("Insufficient inventory capacity."));
		}
	}

	for (auto& Pair : Accounts)
	{
		State.Accounts[Pair.Key] = MoveTemp(Pair.Value);
	}

	for (auto& Pair : Inventories)
	{
		State.Inventories[Pair.Key] = MoveTemp(Pair.Value);
	}

	if (Request.ObligationId.IsValid())
	{
		State.Obligations[Request.ObligationId] = Obligation;
	}

	Receipt.bSucceeded = 1;
	State.Journal.Add(Receipt);
	return Receipt;
}

bool CCLEconomy::Validate(const FCCLEconomyState& State, FString& Error)
{
	if (State.Accounts.Num() > 10000 || State.Inventories.Num() > 10000 || State.Journal.Num() > 1000000)
	{
		Error = TEXT("Economy snapshot exceeds capacity.");
		return false;
	}

	for (const auto& Pair : State.Accounts)
	{
		const auto& A = Pair.Value;
		if (!A.Id.IsValid() || Pair.Key != A.Id || !A.OwnerId.IsValid() || A.Balance < 0 || A.Balance > Limit)
		{
			Error = TEXT("Invalid account.");
			return false;
		}
	}

	for (const auto& Pair : State.Inventories)
	{
		const auto& I = Pair.Value;
		if (!I.Id.IsValid() || Pair.Key != I.Id || !I.OwnerId.IsValid() || I.Capacity < 0 || I.Capacity > Limit ||
			I.Resources.Num() > 128 || UsedCapacity(I) > I.Capacity)
		{
			Error = TEXT("Invalid resource inventory.");
			return false;
		}

		for (const auto& Resource : I.Resources)
		{
			if (!Resource.Key.IsValid() || Resource.Value < 0 || Resource.Value > Limit)
			{
				Error = TEXT("Invalid resource quantity.");
				return false;
			}
		}
	}

	for (const auto& Pair : State.Obligations)
	{
		const auto& O = Pair.Value;
		if (!O.ObligationId.IsValid() || Pair.Key != O.ObligationId || !O.DebtorId.IsValid() || !O.CreditorId.IsValid() ||
			O.DebtorId == O.CreditorId || O.RemainingAmount < 0 || O.RemainingAmount > O.OriginalAmount ||
			O.OriginalAmount > Limit || !FMath::IsFinite(O.DueTime) || O.DueTime < 0)
		{
			Error = TEXT("Invalid obligation.");
			return false;
		}
	}

	TSet<FGuid> Requests;
	for (const auto& Receipt : State.Journal)
	{
		if (!Receipt.Request.RequestId.IsValid() || Requests.Contains(Receipt.Request.RequestId))
		{
			Error = TEXT("Invalid transaction journal.");
			return false;
		}

		Requests.Add(Receipt.Request.RequestId);
	}

	return true;
}

int64 CCLEconomy::Quantity(const FCCLEconomyState& State, FGuid Inventory, FGameplayTag Resource)
{
	const auto* I = State.Inventories.Find(Inventory);
	return I ? I->Resources.FindRef(Resource) : 0;
}

int64 CCLEconomy::UsedCapacity(const FCCLResourceInventory& Inventory)
{
	int64 Total = 0;
	for (const auto& Pair : Inventory.Resources)
	{
		if (Pair.Value < 0 || Pair.Value > Limit || Total > Limit - Pair.Value)
		{
			return MAX_int64;
		}

		Total += Pair.Value;
	}

	return Total;
}
