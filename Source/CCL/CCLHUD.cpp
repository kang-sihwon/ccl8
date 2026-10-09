#include "CCLHUD.h"

#include "UI/CCLUIInputData.h"
#include "Presentation/CCLCinematicSubsystem.h"
#include "UI/CCLMapScreen.h"

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

	if (!MapContext)
	{
		MapContext = NewObject<UCCLMapContext>(this);
	}
	if (!UI->IsViewOpen(MapHandle))
	{
		MapHandle = UI->OpenView(CCLUITags::View_Minimap, MapContext, this);
	}

	if (!Context)
	{
		Context = NewObject<UCCLHUDContext>(this);
		Context->Vitals = NewObject<UCCLCombatViewModel>(Context);
	}

	const auto* Character = Cast<ACCLCharacter>(PC->GetPawn());
	Context->Vitals->Bind(Character ? Character->GetAbilitySystemComponent() : nullptr);
	if (const auto* Campaign = GetWorld()->GetGameState<ACCLCampaignState>(); Campaign && Character && !Character->IsDead())
	{
		if (!bArrivalShown && GetWorld()->GetTimeSeconds() > 1)
		{
			bArrivalShown = 1;
			PC->GetLocalPlayer()->GetSubsystem<UCCLCinematicSubsystem>()->Play(PC, Character->GetActorLocation() + FVector(300, 0, 40), TEXT("사람들이 살아가는 마을"));
		}
		if (!bVictoryShown && Campaign->GetPhase() == ECCLCampaignPhase::Victory)
		{
			bVictoryShown = 1;
			PC->GetLocalPlayer()->GetSubsystem<UCCLCinematicSubsystem>()->Play(PC, Character->GetActorLocation() + FVector(0, 0, 50), TEXT("길이 열렸다"));
		}
	}
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
			UI->CloseView(MapHandle);
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
			Overview.Add(TEXT("경직"));
		}
	}

	if (!Character || Character->IsDead())
	{
		Overview.Add(Character ? TEXT("쓰러졌다. R 키를 눌러 다시 시작할 수 있다.") : TEXT("생성을 기다리는 중이다. R 키로 다시 시도할 수 있다."));
	}

	if (Campaign)
	{
		Overview.Add(Campaign->GetObjective());
	}

	for (TActorIterator<ACCLEnemyCharacter> It(GetWorld()); It; ++It)
	{
		Overview.Add(FString::Printf(TEXT("%s 체력 %.0f%s"), *It->DisplayName,
			It->GetAbilitySystemComponent()->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()),
			It->IsDead() ? TEXT(" (처치됨)") : TEXT("")));
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
		Prompts.Add(FString::Printf(TEXT("보유 동전: %d"), Expedition->GetCoins()));
		for (TActorIterator<ACCLVillageSteward> It(GetWorld()); It; ++It)
		{
			if (It->CanReach(Character))
			{
				Prompts.Add(TEXT("마을 관리인: T 대화 / 의뢰 · B 회복약 구입 (동전 10개)"));
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
				Prompts.Add(FString::Printf(TEXT("E: %s %d개 줍기"), *It->Definition->GetLabel().ToString(), It->Quantity));
				break;
			}
		}
	}

	Prompts.Add(FString::Printf(TEXT("E 줍기 · I 소지품 · M 지도 · %s 메뉴"), UCCLUIInputData::GetBackKeyLabel(this)));
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	Prompts.Add(TEXT("K 사망 테스트"));
#endif
	Context->Update(FString::Join(Overview, TEXT("\n")), PC->IsDialogueVisible() ? FString() : FString::Join(Prompts, TEXT("\n")));
}
