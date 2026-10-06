#include "CCLSessionSmokeSubsystem.h"
#include "Session/CCLGameInstance.h"
#include "Session/CCLSessionRecord.h"
#include "CCLCharacter.h"
#include "CCLPlayerController.h"
#include "CCLPlayerState.h"
#include "Items/CCLInventoryComponent.h"
#include "Items/CCLLoadoutComponent.h"
#include "Items/CCLWorldPickup.h"
#include "Campaign/CCLCampaignDirector.h"
#include "Combat/CCLEnemyCharacter.h"
#include "Campaign/CCLCampaignState.h"
#include "Campaign/CCLExpeditionComponent.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystem/CCLOffenseSet.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "UnrealClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
bool UCCLSessionSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	FString Role;
	return FParse::Value(FCommandLine::Get(), TEXT("CCLSessionSmoke="), Role);
#endif
}
TStatId UCCLSessionSmokeSubsystem::GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLSessionSmokeSubsystem, STATGROUP_Tickables); }
bool UCCLSessionSmokeSubsystem::Check(bool bCondition, const TCHAR* Message)
{
	UE_LOG(LogTemp, Display, TEXT("CCL_SESSION_TEST %s %s"), bCondition ? TEXT("CHECK") : TEXT("FAIL"), Message);
	if (!bCondition) { bComplete = 1; FPlatformMisc::RequestExitWithStatus(false, 1); }
	return bCondition;
}
void UCCLSessionSmokeSubsystem::Capture(const TCHAR* Name)
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("CCLSessionCapture"))) { return; }
	const FString Directory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Tests/SessionVisual"));
	IFileManager::Get().MakeDirectory(*Directory, true);
	FScreenshotRequest::RequestScreenshot(Directory / (FString(Name) + TEXT(".png")), true, false);
}
void UCCLSessionSmokeSubsystem::Finish()
{
	FString Role;
	FParse::Value(FCommandLine::Get(), TEXT("CCLSessionSmoke="), Role);
	UE_LOG(LogTemp, Display, TEXT("CCL_SESSION_TEST PASS %s"), *Role);
	bComplete = 1;
	if (Role != TEXT("host")) { CastChecked<UCCLGameInstance>(GetGameInstance())->Quit(); }
}
void UCCLSessionSmokeSubsystem::Tick(float DeltaTime)
{
	if (!Started) { Started = FPlatformTime::Seconds(); }
	if (FPlatformTime::Seconds() - Started > 150.) { Check(false, TEXT("watchdog")); return; }
	const double Now = FPlatformTime::Seconds();
	if (Now < Next) { return; }
	auto* Session = Cast<UCCLGameInstance>(GetGameInstance());
	UWorld* World = GetWorld();
	if (!Session || !World) { return; }
	auto* PC = Cast<ACCLPlayerController>(Session->GetFirstLocalPlayerController());
	if (!PC) { return; }
	auto* Player = PC->GetPlayerState<ACCLPlayerState>();
	FString Role;
	FParse::Value(FCommandLine::Get(), TEXT("CCLSessionSmoke="), Role);
	if (Step == 0)
	{
		if (!Session->IsMenuVisible()) { return; }
		if (!Check(UCCLGameInstance::ValidateAddress(TEXT("127.0.0.1:18791")) && !UCCLGameInstance::ValidateAddress(TEXT("127.0.0.1?listen")) &&
			!UCCLGameInstance::ValidateAddress(TEXT("host:65536")) && !Session->Join(TEXT("../Campaign")), TEXT("menu visible and unsafe addresses rejected"))) { return; }
		FCCLSessionRecord Good, Decoded;
		TArray<uint8> Bytes;
		if (!Check(FCCLSessionCodec::Encode(Good, Bytes) && FCCLSessionCodec::Decode(Bytes, Decoded), TEXT("record codec round trip"))) { return; }
		Bytes.Last() ^= 1;
		if (!Check(!FCCLSessionCodec::Decode(Bytes, Decoded), TEXT("damaged bytes rejected before parsing"))) { return; }
		Good.Version = 99;
		if (!Check(!FCCLSessionCodec::Validate(Good), TEXT("unknown version rejected"))) { return; }
		Good.Version = 1;
		FCCLSavedItem Invalid;
		Invalid.Id = FGuid::NewGuid(); Invalid.Definition = TEXT("UnknownItem"); Invalid.Quantity = 1;
		Good.Items.Add(Invalid);
		if (!Check(!FCCLSessionCodec::Validate(Good), TEXT("unknown definition rejected"))) { return; }

		Good.Items[0].Definition = TEXT("DA_RecoveryPotion"); Good.Items[0].Quantity = -1;
		if (!Check(!FCCLSessionCodec::Validate(Good), TEXT("negative saved quantity rejected"))) { return; }
		Good.Items[0].Quantity = 1; const FCCLSavedItem Duplicate = Good.Items[0]; Good.Items.Add(Duplicate);
		if (!Check(!FCCLSessionCodec::Validate(Good), TEXT("duplicate saved GUID rejected"))) { return; }
		Good.Items.Pop(); Good.Equipped = FGuid::NewGuid();
		if (!Check(!FCCLSessionCodec::Validate(Good), TEXT("missing equipment reference rejected"))) { return; }
		Capture(TEXT("start"));
		Next = Now + (FParse::Param(FCommandLine::Get(), TEXT("CCLSessionCapture")) ? 5. : 0.5);
		Step = 1;
		return;
	}
	if (Step == 1)
	{
		if (Role == TEXT("read")) { if (!Check(Session->LoadSession(false), TEXT("read checkpoint in fresh process"))) { return; } }
		else if (Role == TEXT("join")) { if (!Check(Session->Join(TEXT("127.0.0.1:18791")), TEXT("join through menu route"))) { return; } }
		else if (Role == TEXT("badjoin")) { if (!Check(Session->Join(TEXT("127.0.0.1:18792")), TEXT("start unreachable connection"))) { return; } }
		else if (Role == TEXT("write"))
		{
			FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::Enter, FModifierKeysState(), 0, false, 0, 0));
			FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(EKeys::Enter, FModifierKeysState(), 0, false, 0, 0));
		}
		else { Session->StartNew(true); }
		Step = 2;
		Next = Now + 1.;
		return;
	}
	if (Role == TEXT("badjoin"))
	{
		if (Session->GetStatus().StartsWith(TEXT("Connection failed:")))
		{
			if (!Check(Session->IsMenuVisible(), TEXT("connection failure visible in menu"))) { return; }
			Capture(TEXT("connection-failure"));
			if (Step == 2) { Step = 3; Next = Now + 2.; return; }
			Finish();
		}
		return;
	}
	if (Role == TEXT("read") && (Step == 3 || Step == 7) && Session->IsMenuVisible())
	{
		if (!Check(Session->LoadSession(false), TEXT("reload after returning to menu"))) { return; }
		Step = Step == 3 ? 4 : 8; Next = Now + 1.; return;
	}
	if (!Player || !Player->GetPawn() || !Player->GetAbilitySystemComponent()->GetAvatarActor() || !World->GetGameState<ACCLCampaignState>()) { return; }
	if (Role == TEXT("join"))
	{
		if (!Check(World->GetNetMode() == NM_Client && !Session->SaveSession(), TEXT("joined client cannot save host checkpoint"))) { return; }
		Finish(); return;
	}
	if (Role == TEXT("host"))
	{
		if (Step == 2) { UE_LOG(LogTemp, Display, TEXT("CCL_SESSION_TEST HOST READY")); Step = 3; }
		if (World->GetGameState()->PlayerArray.Num() >= 2) { Finish(); }
		return;
	}
	ACCLCampaignDirector* Director = nullptr;
	for (TActorIterator<ACCLCampaignDirector> It(World); It; ++It) { Director = *It; break; }
	if (!Check(Director != nullptr, TEXT("campaign director available"))) { return; }
	if (Role == TEXT("write"))
	{
		const FGuid Equipment = Player->GetInventory()->Add(FCCLSessionCodec::Item(TEXT("DA_IronGauntlets")), 1);
		Player->GetInventory()->Add(FCCLSessionCodec::Item(TEXT("DA_RecoveryPotion")), 7);
		Player->GetLoadout()->GrantPoints(1);
		if (!Check(Player->GetLoadout()->Equip(Equipment) && Player->GetLoadout()->Learn(FCCLSessionCodec::Skill(TEXT("DA_PowerTraining"))) &&
			Player->GetLoadout()->Learn(FCCLSessionCodec::Skill(TEXT("DA_VitalityTraining"))), TEXT("seed equipped and learned profile"))) { return; }
		Player->GetExpedition()->Restore(20, ECCLQuestStatus::Accepted);
		for (TActorIterator<ACCLWorldPickup> It(World); It; ++It) { It->Destroy(); }
		if (!Check(Director->RestoreCheckpoint(1, false) && Session->SaveSession(), TEXT("save partial road checkpoint"))) { return; }

		TArray<uint8> SavedBytes;
		if (!Check(UGameplayStatics::LoadDataFromSlot(SavedBytes, UCCLGameInstance::SaveSlot(), 0), TEXT("read saved bytes"))) { return; }
		TArray<uint8> BrokenBytes = SavedBytes;
		BrokenBytes.Last() ^= 1;
		if (!Check(UGameplayStatics::SaveDataToSlot(BrokenBytes, UCCLGameInstance::SaveSlot(), 0) && !Session->LoadSession(false), TEXT("load menu rejects damaged slot"))) { return; }
		if (!Check(UGameplayStatics::SaveDataToSlot(SavedBytes, UCCLGameInstance::SaveSlot(), 0), TEXT("restore validation slot"))) { return; }
		auto* Settings = GEngine->GetGameUserSettings();
		const int32 Original = Settings->GetOverallScalabilityLevel();
		Session->CycleQuality();
		if (!Check(Settings->GetOverallScalabilityLevel() != Original, TEXT("graphics control changes quality"))) { return; }

		const int32 AppliedQuality = Settings->GetOverallScalabilityLevel();
		Settings->LoadSettings(true);
		if (!Check(Settings->GetOverallScalabilityLevel() == AppliedQuality, TEXT("graphics quality persisted to settings file"))) { return; }
		Settings->SetOverallScalabilityLevel(Original < 0 ? 2 : Original);
		Settings->ApplySettings(false);
		Finish(); return;
	}
	if (Session->IsRestoring()) { return; }
	const auto* Inventory = Player->GetInventory();
	const auto* Loadout = Player->GetLoadout();
	if (Step == 2 || Step == 4)
	{
		TArray<uint8> Bytes;
		FCCLSessionRecord Record;
		if (!Check(UGameplayStatics::LoadDataFromSlot(Bytes, UCCLGameInstance::SaveSlot(), 0) && FCCLSessionCodec::Decode(Bytes, Record), TEXT("saved slot remains intact"))) { return; }
		if (!Check(Inventory->GetEntries().Num() == 2 && Loadout->GetEquippedId() == Record.Equipped && Loadout->GetPoints() == 0 &&
			Inventory->Find(Record.Items[1].Id) && Inventory->Find(Record.Items[1].Id)->Quantity == 7 &&
			Player->GetAbilitySystemComponent()->GetNumericAttribute(UCCLOffenseSet::GetAttackBonusAttribute()) == 15.f &&
			Player->GetAbilitySystemComponent()->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) == 125.f &&
			Player->GetExpedition()->GetCoins() == 20 && Player->GetExpedition()->GetQuest() == ECCLQuestStatus::Accepted &&
			Director->GetDefeatedMask() == 1 && World->GetGameState<ACCLCampaignState>()->GetRemainingGuards() == 1,
			TEXT("profile and partial world restored across processes"))) { return; }
		if (!Check(!TActorIterator<ACCLWorldPickup>(World), TEXT("collected supplies do not respawn on load"))) { return; }
		if (Step == 2) { Session->ReturnToMenu(); Step = 3; Next = Now + 1.; return; }
		PC->ToggleInventory();
		Capture(TEXT("loaded-profile"));
		Step = 5; Next = Now + 2.; return;
	}
	if (Step == 5) { Session->ShowMenu(); Capture(TEXT("in-game-menu")); Step = 6; Next = Now + 2.; return; }

	if (Step == 6)
	{
		for (const auto& Guard : Director->GetGuards()) { if (Guard.IsValid()) { Guard->GetAbilitySystemComponent()->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), 0.f); } }
		if (!Check(Director->GetBoss() && World->GetGameState<ACCLCampaignState>()->GetPhase() == ECCLCampaignPhase::Boss, TEXT("restored remaining guard still unlocks boss"))) { return; }
		Director->GetBoss()->GetAbilitySystemComponent()->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(), 0.f);
		if (!Check(World->GetGameState<ACCLCampaignState>()->GetPhase() == ECCLCampaignPhase::Victory && Session->SaveSession(), TEXT("save completed checkpoint"))) { return; }
		Session->ReturnToMenu(); Step = 7; Next = Now + 1.; return;
	}
	if (Step == 8)
	{
		if (!Check(World->GetGameState<ACCLCampaignState>()->GetPhase() == ECCLCampaignPhase::Victory && Director->GetDefeatedMask() == 3 && !TActorIterator<ACCLEnemyCharacter>(World), TEXT("completed checkpoint restores without respawning enemies"))) { return; }
		Finish();
	}
}
