#include "CCLHUD.h"

#include "AbilitySystem/CCLGameplayTags.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "CCLCharacter.h"
#include "CCLPlayerController.h"
#include "CCLPlayerState.h"
#include "Campaign/CCLCampaignState.h"
#include "Campaign/CCLExpeditionComponent.h"
#include "Campaign/CCLVillageSteward.h"
#include "Combat/CCLEnemyCharacter.h"
#include "Combat/CCLFighterComponent.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "Items/CCLItemDefinition.h"
#include "Items/CCLWorldPickup.h"
#include "UI/CCLCombatViewModel.h"
#include "UI/CCLGameUI.h"
#include "UI/CCLHUDScreens.h"
#include "UI/Core/CCLUISubsystem.h"

ACCLHUD::ACCLHUD()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
}

void ACCLHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	auto* PC = Cast<ACCLPlayerController>(PlayerOwner);
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	auto* UI = CCLGameUI::Get(PC);
	// Other genres / test profiles decide which feature registrations exist.
	if (!UI || !UI->FindRegistration(CCLUITags::View_Vitals).IsValid())
	{
		return;
	}

	if (!Context)
	{
		Context = NewObject<UCCLHUDContext>(this);
		Context->Vitals = NewObject<UCCLCombatViewModel>(Context);
	}

	const auto* Character = Cast<ACCLCharacter>(PC->GetPawn());
	Context->Vitals->Bind(Character ? Character->GetAbilitySystemComponent() : nullptr);
	RefreshContent();
	if (!UI->IsViewOpen(VitalsHandle))
	{
		VitalsHandle = UI->OpenView(CCLUITags::View_Vitals, Context, this);
	}

	if (!UI->IsViewOpen(FieldHandle))
	{
		FieldHandle = UI->OpenView(CCLUITags::View_FieldHUD, Context, this);
	}
}

void ACCLHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (const auto* Local = PlayerOwner ? PlayerOwner->GetLocalPlayer() : nullptr)
	{
		if (auto* UI = Local->GetSubsystem<UCCLUISubsystem>())
		{
			UI->CloseView(VitalsHandle);
			UI->CloseView(FieldHandle);
		}
	}

	if (Context && Context->Vitals)
	{
		Context->Vitals->Bind(nullptr);
	}

	Context = nullptr;
	Super::EndPlay(EndPlayReason);
}

void ACCLHUD::RefreshContent()
{
	const auto* PC = Cast<ACCLPlayerController>(PlayerOwner);
	const auto* Character = Cast<ACCLCharacter>(PC->GetPawn());
	const auto* State = PC->GetPlayerState<ACCLPlayerState>();
	const auto* Campaign = GetWorld()->GetGameState<ACCLCampaignState>();
	TArray<FString> Overview;
	TArray<FString> Prompts;
	if (Character)
	{
		if (const auto* Fighter = Character->FindComponentByClass<UCCLFighterComponent>(); Fighter && !Fighter->GetFeedback().IsEmpty())
		{
			Overview.Add(Fighter->GetFeedback());
		}

		if (const auto* ASC = Character->GetAbilitySystemComponent(); ASC && ASC->HasMatchingGameplayTag(CCLTags::State_Stagger))
		{
			Overview.Add(TEXT("STAGGERED"));
		}
	}

	if (!Character || Character->IsDead())
	{
		Overview.Add(Character ? TEXT("You died. Press R to retry.") : TEXT("Waiting for spawn. Press R to retry."));
	}

	if (Campaign)
	{
		Overview.Add(Campaign->GetObjective());
	}

	for (TActorIterator<ACCLEnemyCharacter> It(GetWorld()); It; ++It)
	{
		Overview.Add(FString::Printf(TEXT("%s HP %.0f%s"), *It->DisplayName,
			It->GetAbilitySystemComponent()->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()),
			It->IsDead() ? TEXT(" (defeated)") : TEXT("")));
		const auto* Fighter = It->FindComponentByClass<UCCLFighterComponent>();
		if (!It->IsDead() && Fighter && Fighter->GetAction() == ECCLCombatAction::Attack)
		{
			Overview.Add(It->PatternLabel);
		}
	}

	if (State && Campaign)
	{
		const auto* Expedition = State->GetExpedition();
		Prompts.Add(Expedition->GetTutorial());
		Prompts.Add(FString::Printf(TEXT("Coins: %d"), Expedition->GetCoins()));
		for (TActorIterator<ACCLVillageSteward> It(GetWorld()); It; ++It)
		{
			if (It->CanReach(Character))
			{
				Prompts.Add(TEXT("Village Steward: T Talk / Quest | B Buy potion (10 coins)"));
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
				Prompts.Add(FString::Printf(TEXT("E: %s x%d"), *It->Definition->GetLabel().ToString(), It->Quantity));
				break;
			}
		}
	}

	Prompts.Add(TEXT("E Collect | I Inventory / Training | Esc Menu"));
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	Prompts.Add(TEXT("K Test Death"));
#endif
	Context->Update(FString::Join(Overview, TEXT("\n")), PC->IsDialogueVisible() ? FString() : FString::Join(Prompts, TEXT("\n")));
}
