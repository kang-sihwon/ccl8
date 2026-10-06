#include "CCLExpeditionComponent.h"
#include "CCLVillageSteward.h"
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
	DOREPLIFETIME_CONDITION(UCCLExpeditionComponent, Coins, COND_OwnerOnly);
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
	if (Quest == ECCLQuestStatus::Available)
	{
		Quest = ECCLQuestStatus::Accepted;
		return Report(true, TEXT("Clear the east road. Defeat the warden, then return."));
	}
	const auto* Campaign = GetWorld()->GetGameState<ACCLCampaignState>();
	if (Quest != ECCLQuestStatus::Accepted) { return Report(false, TEXT("Reward already received. The road is open.")); }
	if (!Campaign || Campaign->GetPhase() != ECCLCampaignPhase::Victory) { return Report(false, TEXT("Defeat both guards and the warden, then return.")); }
	Quest = ECCLQuestStatus::Rewarded;
	Coins = FMath::Min(Coins + 60, 1000000);
	CastChecked<ACCLPlayerState>(GetOwner())->GetLoadout()->GrantPoints(1);
	return Report(true, TEXT("Road cleared! Reward: 60 coins and 1 training point."));
}
bool UCCLExpeditionComponent::Buy(ACCLVillageSteward* Steward)
{
	if (!CanInteract(Steward)) { return Report(false, TEXT("Approach the steward while out of combat.")); }
	if (Coins < 10) { return Report(false, TEXT("Need 10 coins for a recovery potion.")); }
	auto* Item = LoadObject<UCCLItemDefinition>(nullptr, TEXT("/Game/Progression/DA_RecoveryPotion.DA_RecoveryPotion"));
	auto* Player = CastChecked<ACCLPlayerState>(GetOwner());
	if (!Item || !Player->GetInventory()->Add(Item, 1).IsValid()) { return Report(false, TEXT("Purchase rejected: inventory full or supply unavailable.")); }
	Coins -= 10;
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
