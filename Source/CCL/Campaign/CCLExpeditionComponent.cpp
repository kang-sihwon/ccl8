#include "CCLExpeditionComponent.h"
#include "CCLVillageSteward.h"
#include "CCLLifeVillager.h"
#include "Agents/CCLAccountComponent.h"
#include "CCLCampaignState.h"
#include "CCLPlayerState.h"
#include "CCLCharacter.h"
#include "Items/CCLInventoryComponent.h"
#include "Items/CCLItemDefinition.h"
#include "Items/CCLLoadoutComponent.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "Net/UnrealNetwork.h"
UCCLExpeditionComponent::UCCLExpeditionComponent() { SetIsReplicatedByDefault(true); }
void UCCLExpeditionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UCCLExpeditionComponent, Quest, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UCCLExpeditionComponent, Notice, COND_OwnerOnly);
}
bool UCCLExpeditionComponent::CanInteract(ACCLVillageSteward* Steward) const
{
	const auto* Player = Cast<ACCLPlayerState>(GetOwner());
	const auto* Pawn = Player ? Cast<ACCLCharacter>(Player->GetPawn()) : nullptr;
	const auto* ASC = Player ? Player->GetAbilitySystemComponent() : nullptr;
	return GetOwner()->HasAuthority() && Pawn && !Pawn->IsDead() && ASC && !ASC->HasMatchingGameplayTag(CCLTags::State_Busy) &&
		!ASC->HasMatchingGameplayTag(CCLTags::State_Stagger) && IsValid(Steward) && Steward->CanReach(Pawn);
}
bool UCCLExpeditionComponent::Report(bool bSuccess, const TCHAR* Message)
{
	if (GetOwner()->HasAuthority()) { Notice = Message; }
	return bSuccess;
}
bool UCCLExpeditionComponent::Talk(ACCLVillageSteward* Steward)
{
	if (!CanInteract(Steward)) { return Report(false, TEXT("Approach the steward while out of combat.")); }
	if (const auto* Villager = Cast<ACCLLifeVillager>(Steward))
	{
		return Report(true, *Villager->DescribeLife());
	}

	if (Quest == ECCLQuestStatus::Available)
	{
		Quest = ECCLQuestStatus::Accepted;
		return Report(true, TEXT("Clear the east road. Defeat the warden, then return."));
	}
	const auto* Campaign = GetWorld()->GetGameState<ACCLCampaignState>();
	if (Quest != ECCLQuestStatus::Accepted) { return Report(false, TEXT("Reward already received. The road is open.")); }
	if (!Campaign || Campaign->GetPhase() != ECCLCampaignPhase::Victory) { return Report(false, TEXT("Defeat both guards and the warden, then return.")); }
	auto* Account = GetOwner()->FindComponentByClass<UCCLAccountComponent>();
	if (!Account || !Account->Reward(60))
	{
		return Report(false, TEXT("The reward account is unavailable."));
	}

	Quest = ECCLQuestStatus::Rewarded;
	CastChecked<ACCLPlayerState>(GetOwner())->GetLoadout()->GrantPoints(1);
	return Report(true, TEXT("Road cleared! Reward: 60 coins and 1 training point."));
}
bool UCCLExpeditionComponent::Buy(ACCLVillageSteward* Steward)
{
	if (!CanInteract(Steward)) { return Report(false, TEXT("Approach the steward while out of combat.")); }
	if (GetCoins() < 10) { return Report(false, TEXT("Need 10 coins for a recovery potion.")); }
	auto* Item = LoadObject<UCCLItemDefinition>(nullptr, TEXT("/Game/Progression/DA_RecoveryPotion.DA_RecoveryPotion"));
	auto* Player = CastChecked<ACCLPlayerState>(GetOwner());
	auto* Account = GetOwner()->FindComponentByClass<UCCLAccountComponent>();
	if (!Account || !Account->Purchase(Player->GetInventory(), Item, 10))
	{
		return Report(false, TEXT("Purchase rejected: inventory full or supply unavailable."));
	}
	return Report(true, TEXT("Purchased recovery potion. I: inventory, H: use selected item."));
}
FString UCCLExpeditionComponent::GetTutorial() const
{
	const auto* Campaign = GetWorld()->GetGameState<ACCLCampaignState>();
	if (Quest == ECCLQuestStatus::Rewarded) { return TEXT("Expedition complete. I: spend your new training point."); }
	if (Quest == ECCLQuestStatus::Available) { return TEXT("Preparation: find the village steward (T), collect supplies (E), equip/train (I)."); }
	if (Campaign && Campaign->GetPhase() == ECCLCampaignPhase::Victory) { return TEXT("Return to the village steward. T: claim your reward."); }
	return TEXT("Head east. Read enemy windups; guard or parry strikes, dodge the warden sweep.");
}

bool UCCLExpeditionComponent::Restore(int32 SavedCoins, ECCLQuestStatus SavedQuest, bool bImportCurrency)
{
	if (!GetOwner()->HasAuthority() || SavedCoins < 0 || SavedCoins > 1000000 || static_cast<uint8>(SavedQuest) > 2) { return false; }
	auto* Account = GetOwner()->FindComponentByClass<UCCLAccountComponent>();
	if (bImportCurrency && (!Account || !Account->ImportLegacy(SavedCoins)))
	{
		return false;
	}
	Quest = SavedQuest;
	Notice = TEXT("Expedition checkpoint restored.");
	return true;
}

int32 UCCLExpeditionComponent::GetCoins() const
{
	const auto* Account = GetOwner()->FindComponentByClass<UCCLAccountComponent>();
	return Account ? static_cast<int32>(FMath::Clamp<int64>(Account->GetBalance(), 0, 1000000)) : 0;
}
