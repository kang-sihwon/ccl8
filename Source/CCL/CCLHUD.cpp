#include "CCLHUD.h"

#include "CCLCharacter.h"
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
	}

	if (!Character || Character->IsDead())
	{
		DrawText(Character ? TEXT("You died. Press R to retry.") : TEXT("Waiting for spawn. Press R to retry."),
		    FLinearColor::Yellow, 30.f, 185.f, nullptr, 1.5f);
	}
}
