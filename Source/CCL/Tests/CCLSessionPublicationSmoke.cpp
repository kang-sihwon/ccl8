#include "CCLSessionSmokeSubsystem.h"

#include "Session/CCLGameInstance.h"
#include "CCLPlayerState.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Agents/CCLAccountComponent.h"
#include "Campaign/CCLExpeditionComponent.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "Items/CCLInventoryComponent.h"
#include "Items/CCLLoadoutComponent.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Base64.h"
#include "Misc/ScopeExit.h"

bool UCCLSessionSmokeSubsystem::CheckPublication(const FCCLSessionRecord& Record)
{
	auto* Session = CastChecked<UCCLGameInstance>(GetGameInstance());
	auto* Player = Session->GetFirstLocalPlayerController()->GetPlayerState<ACCLPlayerState>();
	auto* Account = Player->FindComponentByClass<UCCLAccountComponent>();
	auto* ASC = Player->GetCCLAbilitySystem();
	auto* Agents = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	auto* Clock = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	auto* Chest = GetWorld()->SpawnActor<AActor>();
	if (!Check(Chest && Agents && Clock && Account, TEXT("publication participants available")))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		Chest->Destroy();
	};
	auto* Box = NewObject<UCCLInventoryComponent>(Chest);
	Box->RegisterComponent();
	FString Error;
	if (!Check(Box->BindPersistence(FGuid::NewGuid(), Error), TEXT("register publication container")))
	{
		return false;
	}
	const auto BoxItem = Box->Add(FCCLSessionCodec::Item(TEXT("DA_RecoveryPotion")), 4);
	FCCLSessionRecord Candidate = Record;
	TArray<uint8> WorldBytes;
	if (!Check(BoxItem.IsValid() && Agents->Save(WorldBytes, Error, Player->GetInventory()),
			   TEXT("capture session with live container")))
	{
		return false;
	}
	Candidate.AgentSimulation = FBase64::Encode(WorldBytes);
	if (!Check(FCCLSessionCodec::Validate(Candidate), TEXT("publication fixture validates")))
	{
		return false;
	}
	const int32 SavedAccounts = Agents->GetSimulation().GetEconomy().Accounts.Num();
	auto& Ledger = Agents->GetSimulation().GetServerEconomy();
	const auto SavedAccount = Ledger.Accounts.FindChecked(Account->GetAccountId());
	Ledger.Accounts.Remove(Account->GetAccountId());
	const int64 DisplayBefore = Account->GetBalance();
	Account->TickComponent(.2f, LEVELTICK_All, nullptr);
	const bool bMissingPreserved = Account->GetBalance() == DisplayBefore && !Account->Reward(1) &&
								   !Account->ImportLegacy(999) && Ledger.Accounts.Num() == SavedAccounts - 1;
	Ledger.Accounts.Add(Account->GetAccountId(), SavedAccount);
	if (!Check(bMissingPreserved, TEXT("read, tick and currency writes never reopen missing initialized account")))
	{
		return false;
	}
	const FGuid PreviousAccount = FGuid::NewGuid();
	Agents->GetSimulation().OpenAccount(PreviousAccount, 67);
	if (!Check(Account->BindAccount(PreviousAccount) &&
				   Player->GetExpedition()->Restore(0, ECCLQuestStatus::Rewarded, false) &&
				   Player->GetLoadout()->RestoreState({}, {}, {}, 17, false) && Box->Remove(BoxItem, 3),
			   TEXT("mutate host account, quest, loadout and container")))
	{
		return false;
	}
	ASC->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), 23.f);
	const auto Epoch = Clock->GetEpoch();
	int32 Notifications = 0;
	bool bConsistent = true;
	bool bLegacy = false;
	auto Listen = [&]
	{
		++Notifications;
		// Reading the balance must not recreate an account absent from the restored ledger.
		const int64 Balance = Account->GetBalance();
		const auto& Accounts = Agents->GetSimulation().GetEconomy().Accounts;
		bConsistent &=
			Account->GetAccountId() == Candidate.AccountId && Balance == (bLegacy ? 41 : 20) &&
			Player->GetExpedition()->GetQuest() == (bLegacy ? ECCLQuestStatus::Available : ECCLQuestStatus::Accepted) &&
			Player->GetLoadout()->GetPoints() == (bLegacy ? 4 : Candidate.Points) &&
			ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) == (bLegacy ? 100.f : 125.f) &&
			Box->Find(BoxItem) && Box->Find(BoxItem)->Quantity == 4 && Accounts.Num() == SavedAccounts &&
			!Accounts.Contains(PreviousAccount);
	};
	const auto HostChanged = Player->GetInventory()->OnChanged.AddLambda(Listen);
	const auto BoxChanged = Box->OnChanged.AddLambda(Listen);
	ON_SCOPE_EXIT
	{
		Player->GetInventory()->OnChanged.Remove(HostChanged);
		Box->OnChanged.Remove(BoxChanged);
	};
	auto Preserved = [&]
	{
		return Notifications == 0 && Clock->GetEpoch() == Epoch && Account->GetAccountId() == PreviousAccount &&
			   Account->GetBalance() == 67 && Player->GetExpedition()->GetQuest() == ECCLQuestStatus::Rewarded &&
			   Player->GetLoadout()->GetPoints() == 17 && Player->GetInventory()->GetEntries().IsEmpty() &&
			   Box->Find(BoxItem) && Box->Find(BoxItem)->Quantity == 1 &&
			   ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) == 23.f &&
			   Agents->GetSimulation().GetEconomy().Accounts.Num() == SavedAccounts + 1;
	};
	FCCLSessionRecord Invalid = Candidate;
	Invalid.AccountId = FGuid::NewGuid();
	if (!Check(!Session->ApplyRecord(Invalid) && Preserved(),
			   TEXT("missing saved account refuses before profile publication")))
	{
		return false;
	}
	{
		int32 Attempts = 0;
		const auto Query = FGameplayEffectApplicationQuery::CreateLambda(
			[&](const FActiveGameplayEffectsContainer&, const FGameplayEffectSpec&)
			{
				++Attempts;
				return false;
			});
		ASC->GameplayEffectApplicationQueries.Add(Query);
		ON_SCOPE_EXIT
		{
			ASC->GameplayEffectApplicationQueries.RemoveAll([&](const auto& Value)
															{ return Value.GetHandle() == Query.GetHandle(); });
		};
		if (!Check(!Session->ApplyRecord(Candidate) && Attempts == 1 && Preserved(),
				   TEXT("session GAS refusal preserves account, quest, world and container without notification")))
		{
			return false;
		}
	}
	if (!Check(Session->ApplyRecord(Candidate) && Notifications == 2 && bConsistent,
			   TEXT("host and container callbacks see final account and quest without reopening old account")))
	{
		return false;
	}
	FCCLSessionRecord Legacy;
	Legacy.Coins = 41;
	Legacy.Quest = static_cast<uint8>(ECCLQuestStatus::Available);
	Legacy.Points = 4;
	Legacy.DefeatedGuards = 1;
	bLegacy = true;
	Notifications = 0;
	if (!Check(Session->ApplyRecord(Legacy) && Notifications == 1 && bConsistent,
			   TEXT("legacy currency import and quest finalize before personal inventory notification")))
	{
		return false;
	}
	bLegacy = false;
	Notifications = 0;
	return Check(Session->ApplyRecord(Candidate) && Notifications == 2 && bConsistent,
				 TEXT("saved ledger replaces legacy balance before all restore listeners"));
}
