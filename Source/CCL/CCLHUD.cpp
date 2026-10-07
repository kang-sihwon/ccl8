#include "CCLHUD.h"
#include "Campaign/CCLExpeditionComponent.h"
#include "Campaign/CCLVillageSteward.h"

#include "AbilitySystem/CCLGameplayTags.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystem/CCLOffenseSet.h"
#include "AbilitySystem/CCLStaminaSet.h"
#include "CCLCharacter.h"
#include "CCLPlayerController.h"
#include "CCLPlayerState.h"
#include "Campaign/CCLCampaignState.h"
#include "Combat/CCLEnemyCharacter.h"
#include "Combat/CCLFighterComponent.h"
#include "Engine/Canvas.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Items/CCLInventoryComponent.h"
#include "Items/CCLItemDefinition.h"
#include "Items/CCLLoadoutComponent.h"
#include "Items/CCLSkillDefinition.h"
#include "Items/CCLWorldPickup.h"
#include "UI/CCLCombatViewModel.h"

// 부모 인터페이스 함수

void ACCLHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas)
	{
		return;
	}

	// Use a 720p layout and scale text and spacing together on larger displays.
	const float UiScale = FMath::Max(0.5f, FMath::Min(Canvas->SizeX / 1280.f, Canvas->SizeY / 720.f));
	const float Width = Canvas->SizeX / UiScale;
	const float Height = Canvas->SizeY / UiScale;
	constexpr float Margin = 24.f;
	constexpr float PanelWidth = 560.f;
	const float LeftWidth = FMath::Min(650.f, Width - PanelWidth - Margin * 3.f);

	// Measure with the same font and scale used for drawing. Return logical height
	// so wrapped feedback and item names cannot overlap the following row.
	auto Text = [this, UiScale](const FString& Value, FLinearColor Color, float X, float Y, float MaxWidth, float Emphasis = 1.f,
	                            bool bDraw = true) {
		if (Value.IsEmpty())
		{
			return 0.f;
		}

		const float FontScale = 2.f * UiScale * Emphasis;
		float SampleWidth = 0.f;
		float SampleHeight = 0.f;
		GetTextSize(TEXT("Ag"), SampleWidth, SampleHeight, nullptr, FontScale);
		const float LineHeight = FMath::Max(24.f * Emphasis, SampleHeight / UiScale + 4.f);
		TArray<FString> Lines;
		FString Line;
		TArray<FString> Words;
		Value.ParseIntoArrayWS(Words);
		for (const FString& Word : Words)
		{
			const FString Candidate = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
			float TextWidth = 0.f;
			float TextHeight = 0.f;
			GetTextSize(Candidate, TextWidth, TextHeight, nullptr, FontScale);
			if (!Line.IsEmpty() && TextWidth > MaxWidth * UiScale)
			{
				Lines.Add(Line);
				Line = Word;
			}
			else
			{
				Line = Candidate;
			}
		}

		Lines.Add(Line);
		if (bDraw)
		{
			for (int32 Index = 0; Index < Lines.Num(); ++Index)
			{
				const float ScreenY = (Y + Index * LineHeight) * UiScale;
				DrawText(Lines[Index], FLinearColor::Black, X * UiScale + UiScale, ScreenY + UiScale, nullptr, FontScale);
				DrawText(Lines[Index], Color, X * UiScale, ScreenY, nullptr, FontScale);
			}
		}

		return Lines.Num() * LineHeight;
	};
	auto Panel = [this, UiScale](float X, float Y, float W, float H) {
		DrawRect(FLinearColor(0.015f, 0.02f, 0.03f, 0.82f), X * UiScale, Y * UiScale, W * UiScale, H * UiScale);
	};

	const ACCLCharacter* Character = PlayerOwner ? Cast<ACCLCharacter>(PlayerOwner->GetPawn()) : nullptr;
	auto* Controller = Cast<ACCLPlayerController>(PlayerOwner);
	if (Controller && !Controller->IsDialogueVisible())
	{
		Controller->CloseDialogue();
	}

	const auto* State = PlayerOwner ? PlayerOwner->GetPlayerState<ACCLPlayerState>() : nullptr;
	const auto* Campaign = GetWorld()->GetGameState<ACCLCampaignState>();
	if (!ViewModel)
	{
		ViewModel = NewObject<UCCLCombatViewModel>(this);
	}

	ViewModel->Bind(Character ? Character->GetAbilitySystemComponent() : nullptr);
	float Y = Margin;
	Y += Text(TEXT("WASD Move | Mouse Look | Space Jump"), FLinearColor::White, Margin, Y, LeftWidth);
	Y += Text(TEXT("LMB Left hand | RMB Right hand | Q Parry | Shift Dodge"), FLinearColor::White, Margin, Y, LeftWidth);
	Y += 12.f;
	Y += Text(FString::Printf(TEXT("HP %.0f / %.0f   Stamina %.0f / %.0f"), ViewModel->GetValue(UCCLHealthSet::GetHealthAttribute()),
	                          ViewModel->GetValue(UCCLHealthSet::GetMaxHealthAttribute()),
	                          ViewModel->GetValue(UCCLStaminaSet::GetStaminaAttribute()),
	                          ViewModel->GetValue(UCCLStaminaSet::GetMaxStaminaAttribute())),
	          FLinearColor::Green, Margin, Y, LeftWidth, 1.2f);

	if (Character)
	{
		if (const auto* Fighter = Character->FindComponentByClass<UCCLFighterComponent>())
		{
			Y += Text(Fighter->GetFeedback(), FLinearColor::Yellow, Margin, Y, LeftWidth);
		}

		if (const auto* ASC = Character->GetAbilitySystemComponent(); ASC && ASC->HasMatchingGameplayTag(CCLTags::State_Stagger))
		{
			Y += Text(TEXT("STAGGERED"), FLinearColor::Red, Margin, Y, LeftWidth);
		}
	}

	if (!Character || Character->IsDead())
	{
		Y += Text(Character ? TEXT("You died. Press R to retry.") : TEXT("Waiting for spawn. Press R to retry."), FLinearColor::Yellow,
		          Margin, Y, LeftWidth, 1.2f);
	}

	Y += 12.f;
	if (Campaign)
	{
		Y += Text(Campaign->GetObjective(), FLinearColor(1.f, 0.8f, 0.3f), Margin, Y, LeftWidth);
	}

	for (TActorIterator<ACCLEnemyCharacter> It(GetWorld()); It; ++It)
	{
		Y += Text(FString::Printf(TEXT("%s HP %.0f%s"), *It->DisplayName,
		                          It->GetAbilitySystemComponent()->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()),
		                          It->IsDead() ? TEXT(" (defeated)") : TEXT("")),
		          FLinearColor(1.f, 0.45f, 0.4f), Margin, Y, LeftWidth);
		const auto* Fighter = It->FindComponentByClass<UCCLFighterComponent>();
		if (!It->IsDead() && Fighter && Fighter->GetAction() == ECCLCombatAction::Attack)
		{
			Y += Text(It->PatternLabel, FLinearColor::Yellow, Margin, Y, LeftWidth);
		}
	}

	TArray<TPair<FString, FLinearColor>> Prompts;
	if (State && Campaign)
	{
		const auto* Expedition = State->GetExpedition();
		Prompts.Emplace(Expedition->GetTutorial(), FLinearColor::White);
		Prompts.Emplace(FString::Printf(TEXT("Coins: %d"), Expedition->GetCoins()), FLinearColor::Yellow);
		for (TActorIterator<ACCLVillageSteward> It(GetWorld()); It; ++It)
		{
			if (It->CanReach(Character))
			{
				Prompts.Emplace(TEXT("Village Steward: T Talk / Quest | B Buy potion (10 coins)"), FLinearColor::Green);
				break;
			}
		}
	}

	if (Character)
	{
		for (TActorIterator<ACCLWorldPickup> It(GetWorld()); It; ++It)
		{
			if (FVector::DistSquared(Character->GetActorLocation(), It->GetActorLocation()) < FMath::Square(225.f) && It->Definition)
			{
				Prompts.Emplace(FString::Printf(TEXT("E: %s x%d"), *It->Definition->GetLabel().ToString(), It->Quantity),
				                FLinearColor::Yellow);
				break;
			}
		}
	}

	Prompts.Emplace(TEXT("E Collect | I Inventory / Training | Esc Menu"), FLinearColor::White);
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	Prompts.Emplace(TEXT("K Test Death"), FLinearColor::Yellow);
#endif
	float PromptHeight = 0.f;
	for (const auto& Prompt : Prompts)
	{
		PromptHeight += Text(Prompt.Key, Prompt.Value, Margin, 0.f, LeftWidth, 1.f, false);
	}
	float PromptY = Height - Margin - PromptHeight;
	if (Controller && Controller->IsDialogueVisible())
	{
		Prompts.Reset();
		PromptHeight = 0.f;
	}

	if (!Prompts.IsEmpty())
	{
		Panel(Margin - 8.f, PromptY - 8.f, LeftWidth + 16.f, PromptHeight + 16.f);
	}

	for (const auto& Prompt : Prompts)
	{
		PromptY += Text(Prompt.Key, Prompt.Value, Margin, PromptY, LeftWidth);
	}

	if (Controller && Controller->IsDialogueVisible())
	{
		constexpr float DialogueWidth = 880.f;
		const float X = (Width - DialogueWidth) * 0.5f;
		const float InnerWidth = DialogueWidth - 40.f;
		const float NameHeight = Text(Controller->GetDialogueName(), FLinearColor::Yellow, 0.f, 0.f, InnerWidth, 1.1f, false);
		const float BodyHeight = Text(Controller->GetDialogueText(), FLinearColor::White, 0.f, 0.f, InnerWidth, 1.f, false);
		const float BoxHeight = NameHeight + BodyHeight + 76.f;
		float Row = Height - Margin - BoxHeight;
		Panel(X - 2.f, Row - 2.f, DialogueWidth + 4.f, BoxHeight + 4.f);
		Panel(X, Row, DialogueWidth, BoxHeight);
		Row += 16.f;
		Row += Text(Controller->GetDialogueName(), FLinearColor(1.f, 0.82f, 0.4f), X + 20.f, Row, InnerWidth, 1.1f);
		Row += 8.f;
		Row += Text(Controller->GetDialogueText(), FLinearColor::White, X + 20.f, Row, InnerWidth);
		Text(TEXT("T Talk / Quest | B Buy potion | Esc Close"), FLinearColor(0.8f, 0.85f, 0.9f), X + 20.f, Row + 12.f, InnerWidth);
	}
}
