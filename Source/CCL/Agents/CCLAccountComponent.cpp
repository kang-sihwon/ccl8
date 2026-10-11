#include "CCLAccountComponent.h"

#include "CCLAgentWorldSubsystem.h"
#include "CCLAgentTags.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Items/CCLInventoryComponent.h"
#include "Items/CCLItemDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "NativeGameplayTags.h"

namespace
{
const FGuid Treasury(0xCC1800FF, 0, 1, 1);
const FGuid ShopInventory(0xCC1800FF, 0, 1, 2);
}

UCCLAccountComponent::UCCLAccountComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.2f;
}

void UCCLAccountComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()->HasAuthority() && !AccountId.IsValid())
	{
		AccountId = FGuid::NewGuid();
		ProvisionalAccountId = AccountId;
		ProvisionalOpeningBalance = BalanceView;
	}

	InitializeAccount();
	RefreshView();
}

void UCCLAccountComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function)
{
	Super::TickComponent(DeltaTime, TickType, Function);
	InitializeAccount();
	RefreshView();
}

void UCCLAccountComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UCCLAccountComponent, AccountId, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UCCLAccountComponent, BalanceView, COND_OwnerOnly);
}

bool UCCLAccountComponent::ImportLegacy(int64 Coins)
{
	if (!GetOwner()->HasAuthority() || Coins < 0 || Coins > 1000000)
	{
		return false;
	}

	auto* World = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	if (World && World->IsRunning())
	{
		if (!InitializeAccount())
		{
			return false;
		}

		auto* Simulation = Resolve();
		if (!Simulation || !Simulation->ImportAccountBalance(AccountId, Coins))
		{
			return false;
		}
	}

	BalanceView = Coins;
	return true;
}

bool UCCLAccountComponent::BindAccount(FGuid Id)
{
	auto* World = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	if (!GetOwner()->HasAuthority() || !World || !World->IsRunning() ||
		!World->GetSimulation().GetEconomy().Accounts.Contains(Id))
	{
		return false;
	}

	AccountId = Id;
	InitializedWorldId = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>()->GetIdentity().WorldId;
	RefreshView();
	return true;
}

bool UCCLAccountComponent::Reward(int64 Amount)
{
	if (!InitializeAccount())
	{
		return false;
	}

	auto* Simulation = Resolve();
	if (!Simulation || Amount <= 0 || Amount > 1000000 - GetBalance())
	{
		return false;
	}

	Simulation->OpenAccount(Treasury, 1000000);
	FCCLTransactionRequest Request;
	Request.RequestId = FGuid::NewGuid();
	Request.Buyer = Treasury;
	Request.Seller = AccountId;
	Request.Price = Amount;
	Request.Reason = CCLAgentTags::Help;
	Request.Time = Simulation->GetTime();
	const bool bSucceeded = CCLEconomy::Execute(Simulation->GetServerEconomy(), Request).bSucceeded != 0;
	RefreshView();
	return bSucceeded;
}

bool UCCLAccountComponent::Purchase(UCCLInventoryComponent* Inventory, const FCCLShopOffer& Offer)
{
	if (!InitializeAccount())
	{
		return false;
	}

	auto* Simulation = Resolve();
	auto* Item = Offer.Item.LoadSynchronous();
	const int64 Price = Offer.Price;
	if (!Simulation || !Inventory || Inventory->GetOwner() != GetOwner() || !Item || Price < 1 || Price > 1000000 ||
		!Offer.StockResource.IsValid() || Offer.InitialStock < 1 || Offer.InitialStock > 1000 || GetBalance() < Price ||
		!Inventory->CanAdd(Item, 1))
	{
		return false;
	}

	Simulation->OpenAccount(Treasury, 1000000);
	auto& Economy = Simulation->GetServerEconomy();
	if (!Economy.Inventories.Contains(ShopInventory))
	{
		FCCLResourceInventory Stock;
		Stock.Id = ShopInventory;
		Stock.OwnerId = Treasury;
		Stock.Capacity = 1000;
		Economy.Inventories.Add(Stock.Id, Stock);
	}

	// Each offered resource is stocked once, the first time a buyer asks for it.
	auto& Stock = Economy.Inventories[ShopInventory];
	if (!Stock.Resources.Contains(Offer.StockResource))
	{
		Stock.Resources.Add(Offer.StockResource, Offer.InitialStock);
	}

	if (CCLEconomy::Quantity(Economy, ShopInventory, Offer.StockResource) < 1 || !Inventory->BeginTransaction())
	{
		return false;
	}

	const auto Before = Inventory->GetEntries();
	const FGuid DeliveredItem = Inventory->Add(Item, 1);
	if (!DeliveredItem.IsValid())
	{
		Inventory->EndTransaction(false);
		return false;
	}

	FCCLTransactionRequest Request;
	Request.RequestId = FGuid::NewGuid();
	Request.Buyer = AccountId;
	Request.Seller = Treasury;
	Request.Price = Price;
	Request.Reason = CCLAgentTags::Trade;
	Request.Time = Simulation->GetTime();
	FCCLItemTransfer Transfer;
	Transfer.Source = ShopInventory;
	Transfer.ExternalInventoryOwner = AccountId;
	Transfer.ExternalItem = DeliveredItem;
	Transfer.Resource = Offer.StockResource;
	Transfer.Quantity = 1;
	Request.Items.Add(Transfer);
	const bool bSucceeded = CCLEconomy::Execute(Economy, Request).bSucceeded != 0;
	if (!bSucceeded)
	{
		Inventory->Restore(Before);
	}

	RefreshView();
	Inventory->EndTransaction(bSucceeded);
	return bSucceeded;
}

int64 UCCLAccountComponent::GetBalance() const
{
	if (auto* Simulation = Resolve())
	{
		const auto* Account = Simulation->GetEconomy().Accounts.Find(AccountId);
		return Account ? Account->Balance : BalanceView;
	}

	return BalanceView;
}

FCCLLifeSimulation* UCCLAccountComponent::Resolve() const
{
	if (!GetOwner()->HasAuthority())
	{
		return nullptr;
	}

	auto* World = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	if (!World || !World->IsRunning() || !AccountId.IsValid())
	{
		return nullptr;
	}

	return &World->GetSimulation();
}

bool UCCLAccountComponent::EnsureOpen()
{
	auto* Simulation = Resolve();
	auto* World = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	if (!GetOwner()->HasAuthority() || !Simulation || !World || !World->IsRunning())
	{
		return false;
	}

	// Only a restore that kept this player may reopen a missing account; reads and writes never do.
	if (!Simulation->GetEconomy().Accounts.Contains(AccountId) && !Simulation->OpenAccount(AccountId, BalanceView))
	{
		return false;
	}

	InitializedWorldId = World->GetIdentity().WorldId;
	return true;
}

bool UCCLAccountComponent::InitializeAccount()
{
	auto* Simulation = Resolve();
	auto* World = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	if (!Simulation || !World || !World->IsRunning())
	{
		return false;
	}

	const FGuid WorldId = World->GetIdentity().WorldId;
	if (InitializedWorldId == WorldId)
	{
		return Simulation->GetEconomy().Accounts.Contains(AccountId);
	}

	if (!Simulation->OpenAccount(AccountId, BalanceView))
	{
		return false;
	}

	InitializedWorldId = WorldId;
	return true;
}

void UCCLAccountComponent::RefreshView()
{
	if (GetOwner()->HasAuthority())
	{
		BalanceView = GetBalance();
	}
}
