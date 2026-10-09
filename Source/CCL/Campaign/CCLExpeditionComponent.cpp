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
	if (!CanInteract(Steward)) { return Report(false, TEXT("전투가 끝나면 마을 관리인에게 다가가자.")); }
	if (const auto* Villager = Cast<ACCLLifeVillager>(Steward))
	{
		return Report(true, *Villager->DescribeLife());
	}

	if (Quest == ECCLQuestStatus::Available)
	{
		Quest = ECCLQuestStatus::Accepted;
		return Report(true, TEXT("동쪽 길을 열어 줘. 성문 수호자를 쓰러뜨리고 돌아오면 돼."));
	}
	const auto* Campaign = GetWorld()->GetGameState<ACCLCampaignState>();
	if (Quest != ECCLQuestStatus::Accepted) { return Report(false, TEXT("보상은 이미 받았어. 이제 길이 열렸어.")); }
	if (!Campaign || Campaign->GetPhase() != ECCLCampaignPhase::Victory) { return Report(false, TEXT("경비병 두 명과 성문 수호자를 쓰러뜨리고 돌아와 줘.")); }
	auto* Account = GetOwner()->FindComponentByClass<UCCLAccountComponent>();
	if (!Account || !Account->Reward(60))
	{
		return Report(false, TEXT("보상을 지급할 계정을 찾을 수 없어."));
	}

	Quest = ECCLQuestStatus::Rewarded;
	CastChecked<ACCLPlayerState>(GetOwner())->GetLoadout()->GrantPoints(1);
	return Report(true, TEXT("길을 열었구나! 보상으로 동전 60개와 훈련 점수 1점을 줄게."));
}
bool UCCLExpeditionComponent::Buy(ACCLVillageSteward* Steward)
{
	if (!CanInteract(Steward)) { return Report(false, TEXT("전투가 끝나면 마을 관리인에게 다가가자.")); }
	if (GetCoins() < 10) { return Report(false, TEXT("회복약을 사려면 동전 10개가 필요해.")); }
	auto* Item = LoadObject<UCCLItemDefinition>(nullptr, TEXT("/Game/Progression/DA_RecoveryPotion.DA_RecoveryPotion"));
	auto* Player = CastChecked<ACCLPlayerState>(GetOwner());
	auto* Account = GetOwner()->FindComponentByClass<UCCLAccountComponent>();
	if (!Account || !Account->Purchase(Player->GetInventory(), Item, 10))
	{
		return Report(false, TEXT("구입 불가: 소지품이 가득 찼거나 재고가 없어."));
	}
	return Report(true, TEXT("회복약을 구입했다. I: 소지품, H: 선택한 아이템 사용"));
}
FString UCCLExpeditionComponent::GetTutorial() const
{
	const auto* Campaign = GetWorld()->GetGameState<ACCLCampaignState>();
	if (Quest == ECCLQuestStatus::Rewarded) { return TEXT("원정을 완료했다. I 키로 새 훈련 점수를 사용하자."); }
	if (Quest == ECCLQuestStatus::Available) { return TEXT("준비: 관리인과 대화 (T), 보급품 줍기 (E), 장비 / 훈련 (I)"); }
	if (Campaign && Campaign->GetPhase() == ECCLCampaignPhase::Victory) { return TEXT("마을 관리인에게 돌아가자. T: 보상 받기"); }
	return TEXT("동쪽으로 향하자. 적의 공격을 방어하거나 패링하고, 수호자의 휩쓸기는 회피하자.");
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
	Notice = TEXT("원정의 저장 지점을 불러왔다.");
	return true;
}

int32 UCCLExpeditionComponent::GetCoins() const
{
	const auto* Account = GetOwner()->FindComponentByClass<UCCLAccountComponent>();
	return Account ? static_cast<int32>(FMath::Clamp<int64>(Account->GetBalance(), 0, 1000000)) : 0;
}
