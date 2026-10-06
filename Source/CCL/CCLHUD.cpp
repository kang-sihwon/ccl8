#include "CCLHUD.h"
#include "Campaign/CCLExpeditionComponent.h"
#include "Campaign/CCLVillageSteward.h"

#include "CCLCharacter.h"
#include "CCLPlayerState.h"
#include "CCLPlayerController.h"
#include "Items/CCLInventoryComponent.h"
#include "Items/CCLItemDefinition.h"
#include "Items/CCLLoadoutComponent.h"
#include "Items/CCLSkillDefinition.h"
#include "Items/CCLWorldPickup.h"
#include "AbilitySystem/CCLOffenseSet.h"
#include "Engine/Canvas.h"
#include "UI/CCLCombatViewModel.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystem/CCLStaminaSet.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "Combat/CCLFighterComponent.h"
#include "Combat/CCLEnemyCharacter.h"
#include "Campaign/CCLCampaignState.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

// 부모 인터페이스 함수

void ACCLHUD::DrawHUD()
{
	Super::DrawHUD();
	DrawText(TEXT("WASD Move  |  Mouse Look  |  Space Jump"), FLinearColor::White, 30.f, 30.f);
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	DrawText(TEXT("K Test Death"), FLinearColor::Yellow, 30.f, 55.f);
#endif
	const ACCLCharacter* Character = PlayerOwner ? Cast<ACCLCharacter>(PlayerOwner->GetPawn()) : nullptr;

	if (!ViewModel)
	{
		ViewModel = NewObject<UCCLCombatViewModel>(this);
	}

	ViewModel->Bind(Character ? Character->GetAbilitySystemComponent() : nullptr);
	DrawText(TEXT("LMB Attack | RMB Hold Guard | Q Parry | Shift Dodge"), FLinearColor::White, 30.f, 80.f);
	DrawText(FString::Printf(TEXT("HP %.0f / %.0f   Stamina %.0f / %.0f"),
	             ViewModel->GetValue(UCCLHealthSet::GetHealthAttribute()), ViewModel->GetValue(UCCLHealthSet::GetMaxHealthAttribute()),
	             ViewModel->GetValue(UCCLStaminaSet::GetStaminaAttribute()), ViewModel->GetValue(UCCLStaminaSet::GetMaxStaminaAttribute())),
	    FLinearColor::Green, 30.f, 110.f);

	if (Character)
	{
		if (auto* Fighter = Character->FindComponentByClass<UCCLFighterComponent>())
		{
			DrawText(Fighter->GetFeedback(), FLinearColor::Yellow, 30.f, 140.f, nullptr, 1.5f);
		}

		if (auto* ASC = Character->GetAbilitySystemComponent(); ASC && ASC->HasMatchingGameplayTag(CCLTags::State_Stagger))
		{
			DrawText(TEXT("STAGGERED"), FLinearColor::Red, 30.f, 170.f);
		}
	}

	float Y = 250.f;
	if (const auto* Campaign = GetWorld()->GetGameState<ACCLCampaignState>())
	{
		DrawText(Campaign->GetObjective(), FLinearColor(1.f, 0.8f, 0.3f), 30.f, 210.f, nullptr, 1.25f);
	}

	for (TActorIterator<ACCLEnemyCharacter> It(GetWorld()); It; ++It)
	{
		DrawText(FString::Printf(TEXT("%s HP %.0f%s"), *It->DisplayName, It->GetAbilitySystemComponent()->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()),
		             It->IsDead() ? TEXT(" (defeated)") : TEXT("")),
		    FLinearColor::Red, 30.f, Y);
		Y += 25.f;
		if (!It->IsDead() && It->FindComponentByClass<UCCLFighterComponent>()->GetAction() == ECCLCombatAction::Attack)
		{
			DrawText(It->PatternLabel, FLinearColor::Yellow, 30.f, Y);
			Y += 25.f;
		}
	}

	if (!Character || Character->IsDead())
	{
		DrawText(Character ? TEXT("You died. Press R to retry.") : TEXT("Waiting for spawn. Press R to retry."),
		    FLinearColor::Yellow, 30.f, 185.f, nullptr, 1.5f);
	}

	DrawText(TEXT("E Collect supplies | I Inventory / Training | Esc Exit"), FLinearColor::White, 30.f, Canvas->SizeY - 45.f);
	if (Character)
	{
		for (TActorIterator<ACCLWorldPickup> It(GetWorld()); It; ++It)
		{
			if (FVector::DistSquared(Character->GetActorLocation(), It->GetActorLocation()) < FMath::Square(225.f) && It->Definition)
			{
				DrawText(FString::Printf(TEXT("E: %s x%d"), *It->Definition->GetLabel().ToString(), It->Quantity), FLinearColor::Yellow, 30.f, Canvas->SizeY - 75.f);
				break;
			}
		}
	}
	const auto* Controller = Cast<ACCLPlayerController>(PlayerOwner);
	const auto* State = PlayerOwner ? PlayerOwner->GetPlayerState<ACCLPlayerState>() : nullptr;
	if (State && GetWorld()->GetGameState<ACCLCampaignState>())
	{
		const auto* Expedition = State->GetExpedition();
		DrawText(Expedition->GetTutorial(), FLinearColor::White, 30.f, Canvas->SizeY - 165.f);
		DrawText(FString::Printf(TEXT("Coins: %d  |  %s"), Expedition->GetCoins(), *Expedition->GetNotice()), FLinearColor::Yellow, 30.f, Canvas->SizeY - 140.f);
		for (TActorIterator<ACCLVillageSteward> It(GetWorld()); It; ++It)
		{
			if (It->CanReach(Character)) { DrawText(TEXT("Village Steward: T Talk / Quest | B Buy potion (10 coins)"), FLinearColor::Green, 30.f, Canvas->SizeY - 105.f); break; }
		}
	}
	if (Controller && Controller->IsInventoryOpen() && State)
	{
		const float X = FMath::Max(30.f, static_cast<float>(Canvas->SizeX) - 420.f);
		const auto* Inventory = State->GetInventory();
		const auto* Loadout = State->GetLoadout();
		DrawRect(FLinearColor(0.02f, 0.03f, 0.05f, 0.95f), X - 15.f, 25.f, 405.f, 610.f);
		DrawText(TEXT("INVENTORY & TRAINING"), FLinearColor::White, X, 40.f, nullptr, 1.3f);
		DrawText(TEXT("Up/Down Select | F Equip | G Unequip | H Use"), FLinearColor::Gray, X, 70.f);
		float Row = 105.f;
		for (int32 Index = 0; Index < Inventory->GetEntries().Num(); ++Index)
		{
			const auto& Entry = Inventory->GetEntries()[Index];
			DrawText(FString::Printf(TEXT("%s %s x%d%s"), Index == Controller->GetSelectedItem() ? TEXT(">") : TEXT(" "),
				Entry.Definition ? *Entry.Definition->GetLabel().ToString() : TEXT("Loading"), Entry.Quantity,
				Entry.Id == Loadout->GetEquippedId() ? TEXT(" [equipped]") : TEXT("")), FLinearColor::White, X, Row);
			Row += 23.f;
		}
		if (Inventory->GetEntries().IsEmpty())
		{
			DrawText(TEXT("Empty. Collect the village supplies with E."), FLinearColor::Gray, X, Row);
		}
		Row = FMath::Max(Row + 25.f, 220.f);
		DrawText(FString::Printf(TEXT("Training points: %d"), Loadout->GetPoints()), FLinearColor::Yellow, X, Row);
		Row += 30.f;
		for (int32 Index = 0; Index < Loadout->GetSkills().Num(); ++Index)
		{
			const auto* Skill = Loadout->GetSkills()[Index].Get();
			DrawText(FString::Printf(TEXT("%d: %s (%d pt)%s"), Index + 1, *Skill->Label.ToString(), Skill->PointCost,
				Loadout->IsLearned(Skill) ? TEXT(" [learned]") : TEXT("")), FLinearColor::White, X, Row);
			Row += 25.f;
		}
		DrawText(FString::Printf(TEXT("Attack bonus: +%.0f"), State->GetAbilitySystemComponent()->GetNumericAttribute(UCCLOffenseSet::GetAttackBonusAttribute())), FLinearColor::Green, X, Row + 15.f);
		DrawText(Loadout->GetResult(), FLinearColor::Yellow, X, Row + 50.f);
	}
}
