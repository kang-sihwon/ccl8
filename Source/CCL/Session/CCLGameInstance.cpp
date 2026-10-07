#include "CCLGameInstance.h"
#include "CCLCharacter.h"
#include "CCLPlayerController.h"
#include "CCLPlayerState.h"
#include "Items/CCLInventoryComponent.h"
#include "Items/CCLItemDefinition.h"
#include "Items/CCLLoadoutComponent.h"
#include "Items/CCLSkillDefinition.h"
#include "Items/CCLWorldPickup.h"
#include "Campaign/CCLCampaignDirector.h"
#include "Campaign/CCLCampaignState.h"
#include "Campaign/CCLExpeditionComponent.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Framework/Application/SlateApplication.h"

void UCCLGameInstance::Init()
{
	Super::Init();
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &ThisClass::BeforeMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::AfterMap);
	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &ThisClass::NetworkFailed);
		GEngine->OnTravelFailure().AddUObject(this, &ThisClass::TravelFailed);
	}
}
void UCCLGameInstance::Shutdown()
{
	HideMenu();
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	if (GEngine) { GEngine->OnNetworkFailure().RemoveAll(this); GEngine->OnTravelFailure().RemoveAll(this); }
	Super::Shutdown();
}
void UCCLGameInstance::BeforeMap(const FString& MapName) { HideMenu(); }
void UCCLGameInstance::AfterMap(UWorld* World)
{
	if (!World || World->GetGameInstance() != this || World->GetNetMode() == NM_DedicatedServer) { return; }
	if (bPendingRestore && World->GetGameState<ACCLCampaignState>())
	{
		RestoreStarted = FPlatformTime::Seconds();
		World->GetTimerManager().SetTimer(RestoreTimer, this, &ThisClass::TryRestore, 0.1f, true);
	}
	else if (World->GetMapName().Contains(TEXT("FrontEnd")))
	{
		FTimerHandle MenuTimer;
		World->GetTimerManager().SetTimer(MenuTimer, this, &ThisClass::ShowMenu, 0.2f, false);
	}
}
void UCCLGameInstance::ShowMenu()
{
	if (Menu.IsValid() || !GetGameViewportClient() || !GetFirstLocalPlayerController()) { return; }
	auto* PC = GetFirstLocalPlayerController();
	if (auto* CCL = Cast<ACCLPlayerController>(PC))
	{
		CCL->CloseInventory();
		CCL->CloseDialogue();
	}

	PC->FlushPressedKeys();
	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);
	PC->bShowMouseCursor = true;
	TSharedRef<SVerticalBox> Choices = SNew(SVerticalBox);
	TSharedPtr<SButton> FirstButton;
	auto AddButton = [&Choices, &FirstButton](const TCHAR* Label, TFunction<void()> Action)
	{
		TSharedPtr<SButton> Button;
		Choices->AddSlot().AutoHeight().Padding(0.f, 3.f)
		[ SAssignNew(Button, SButton).ContentPadding(FMargin(12.f, 7.f))
			.OnClicked_Lambda([Action]() { Action(); return FReply::Handled(); })
			[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).Text(FText::FromString(Label))] ];
		if (!FirstButton.IsValid()) { FirstButton = Button; }
	};
	Choices->AddSlot().AutoHeight().Padding(0.f, 4.f)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).Text(FText::FromString(TEXT("CCL | EAST GATE EXPEDITION"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 32))];
	Choices->AddSlot().AutoHeight().Padding(0.f, 6.f)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).Text(FText::FromString(TEXT("Prototype campaign - solo or direct multiplayer")))];
	const bool bInCampaign = GetWorld()->GetGameState<ACCLCampaignState>() != nullptr;
	if (bInCampaign)
	{
		AddButton(TEXT("Resume"), [this]() { HideMenu(); });
		AddButton(TEXT("Save host checkpoint"), [this]() { SaveSession(); });
		AddButton(TEXT("Return to start screen (unsaved changes are lost)"), [this]() { ReturnToMenu(); });
	}
	else
	{
		AddButton(TEXT("New solo expedition"), [this]() { StartNew(false); });
		AddButton(TEXT("Host new expedition"), [this]() { StartNew(true); });
		AddButton(TEXT("Load solo checkpoint"), [this]() { LoadSession(false); });
		AddButton(TEXT("Host saved checkpoint"), [this]() { LoadSession(true); });
		Choices->AddSlot().AutoHeight().Padding(0.f, 6.f)[SAssignNew(AddressBox, SEditableTextBox).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).Text(FText::FromString(TEXT("127.0.0.1:7777"))).HintText(FText::FromString(TEXT("Host name or IPv4:port")))];
		AddButton(TEXT("Join address"), [this]() { if (AddressBox.IsValid()) { Join(AddressBox->GetText().ToString()); } });
	}
	AddButton(TEXT("Cycle graphics quality (Low / Medium / High / Epic)"), [this]() { CycleQuality(); });
	AddButton(TEXT("Toggle 1280x720 window / borderless desktop"), [this]() { ToggleWindowMode(); });
	AddButton(TEXT("Quit game"), [this]() { Quit(); });
	Choices->AddSlot().AutoHeight().Padding(0.f, 12.f)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).WrapTextAt(780.f).Text_Lambda([this]() { return FText::FromString(Status); })];
	Choices->AddSlot().AutoHeight()[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 18)).WrapTextAt(780.f).Text(FText::FromString(TEXT("Host checkpoint only; guests are session-only. Opening this menu does not pause the world.")))];
	Menu = SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0.035f, 0.06f, 0.09f, 0.97f)).HAlign(HAlign_Center).VAlign(VAlign_Center)
	[ SNew(SBox).WidthOverride(800.f).MaxDesiredHeight(940.f)[SNew(SScrollBox)+SScrollBox::Slot()[Choices]] ];
	GetGameViewportClient()->AddViewportWidgetContent(Menu.ToSharedRef(), 100);
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(FirstButton);
	PC->SetInputMode(InputMode);
	FSlateApplication::Get().SetKeyboardFocus(FirstButton, EFocusCause::SetDirectly);
}
void UCCLGameInstance::HideMenu()
{
	if (!Menu.IsValid()) { return; }
	if (GetGameViewportClient()) { GetGameViewportClient()->RemoveViewportWidgetContent(Menu.ToSharedRef()); }
	Menu.Reset();
	AddressBox.Reset();
	if (auto* PC = GetFirstLocalPlayerController())
	{
		PC->ResetIgnoreMoveInput();
		PC->ResetIgnoreLookInput();
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
}
void UCCLGameInstance::ToggleMenu() { if (Menu.IsValid()) { HideMenu(); } else { ShowMenu(); } }
void UCCLGameInstance::StartNew(bool bHost)
{
	bPendingRestore = 0;
	Status = bHost ? TEXT("Hosting an expedition.") : TEXT("Solo expedition started.");
	HideMenu();
	UGameplayStatics::OpenLevel(this, TEXT("/Game/Maps/Campaign"), true, bHost ? TEXT("listen") : TEXT(""));
}
bool UCCLGameInstance::ValidateAddress(const FString& Address)
{
	if (Address.IsEmpty() || Address.Len() > 253) { return false; }
	FString Host = Address, Port;
	if (Address.Split(TEXT(":"), &Host, &Port))
	{
		if (Port.IsEmpty() || Port.Len() > 5) { return false; }
		for (TCHAR C : Port) { if (C < '0' || C > '9') { return false; } }
		const int32 Number = FCString::Atoi(*Port);
		if (Number < 1 || Number > 65535) { return false; }
	}
	if (Host.IsEmpty() || !FChar::IsAlnum(Host[0]) || !FChar::IsAlnum(Host[Host.Len() - 1])) { return false; }
	for (TCHAR C : Host) { if (!((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9') || C == '.' || C == '-')) { return false; } }
	return true;
}
bool UCCLGameInstance::Join(const FString& Address)
{
	if (!ValidateAddress(Address)) { Status = TEXT("Invalid address. Use host name or IPv4:port (1-65535)."); return false; }
	auto* PC = GetFirstLocalPlayerController();
	if (!PC) { Status = TEXT("Local player is not ready."); return false; }
	bPendingRestore = 0;
	Status = TEXT("Connecting to ") + Address + TEXT(" ...");
	PC->ClientTravel(Address, TRAVEL_Absolute);
	return true;
}
FString UCCLGameInstance::SaveSlot()
{
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	FString Role;
	if (FParse::Value(FCommandLine::Get(), TEXT("CCLSessionSmoke="), Role)) { return TEXT("CCL_Validation"); }
#endif
	return TEXT("CCL_Checkpoint");
}
bool UCCLGameInstance::SaveSession()
{
	auto* PC = GetFirstLocalPlayerController();
	auto* Player = PC ? PC->GetPlayerState<ACCLPlayerState>() : nullptr;
	auto* Pawn = PC ? Cast<ACCLCharacter>(PC->GetPawn()) : nullptr;
	auto* Campaign = GetWorld()->GetGameState<ACCLCampaignState>();
	ACCLCampaignDirector* Director = nullptr;
	for (TActorIterator<ACCLCampaignDirector> It(GetWorld()); It; ++It) { Director = *It; break; }
	if (!Player || !Pawn || !PC->HasAuthority() || Pawn->IsDead() || !Campaign || !Director ||
		(Player->GetAbilitySystemComponent()->HasMatchingGameplayTag(CCLTags::State_Busy) || Player->GetAbilitySystemComponent()->HasMatchingGameplayTag(CCLTags::State_Stagger)) ||
		Campaign->GetPhase() == ECCLCampaignPhase::Error)
	{ Status = TEXT("Save rejected. A living, idle host in a valid expedition is required."); return false; }
	FCCLSessionRecord Record;
	for (const auto& Entry : Player->GetInventory()->GetEntries())
	{
		FCCLSavedItem Value;
		Value.Id = Entry.Id;
		Value.Definition = Entry.Definition ? Entry.Definition->GetFName() : NAME_None;
		Value.Quantity = Entry.Quantity;
		Value.Slot = Entry.Slot;
		Record.Items.Add(Value);
	}
	Record.Equipped = Player->GetLoadout()->GetEquippedId();
	Record.EquipmentSlots = Player->GetLoadout()->GetEquipment();
	Record.Points = Player->GetLoadout()->GetPoints();
	for (auto Skill : Player->GetLoadout()->GetSkills()) { if (Player->GetLoadout()->IsLearned(Skill)) { Record.Skills.Add(Skill->GetFName()); } }
	Record.Coins = Player->GetExpedition()->GetCoins();
	Record.Quest = static_cast<uint8>(Player->GetExpedition()->GetQuest());
	Record.DefeatedGuards = Director->GetDefeatedMask();
	Record.Victory = Campaign->GetPhase() == ECCLCampaignPhase::Victory;
	for (TActorIterator<ACCLWorldPickup> It(GetWorld()); It; ++It) { if (It->Definition) { Record.RemainingSupplies.Add(It->Definition->GetFName()); } }
	TArray<uint8> Bytes;
	const bool bSaved = FCCLSessionCodec::Encode(Record, Bytes) && UGameplayStatics::SaveDataToSlot(Bytes, SaveSlot(), 0);
	Status = bSaved ? TEXT("Host checkpoint saved. Resume from the village at full health.") : TEXT("Save failed. Check storage and definitions.");
	UE_LOG(LogTemp, Display, TEXT("CCL_SESSION save=%d"), bSaved);
	return bSaved;
}
bool UCCLGameInstance::LoadSession(bool bHost)
{
	TArray<uint8> Bytes;
	FCCLSessionRecord Record;
	if (!UGameplayStatics::LoadDataFromSlot(Bytes, SaveSlot(), 0) || !FCCLSessionCodec::Decode(Bytes, Record))
	{ Status = TEXT("Cannot load: checkpoint missing, damaged, incompatible or references unavailable content."); return false; }
	Pending = Record;
	bPendingRestore = 1;
	Status = TEXT("Loading checkpoint ...");
	HideMenu();
	UGameplayStatics::OpenLevel(this, TEXT("/Game/Maps/Campaign"), true, bHost ? TEXT("listen") : TEXT(""));
	return true;
}
void UCCLGameInstance::TryRestore()
{
	auto* PC = GetFirstLocalPlayerController();
	auto* Player = PC ? PC->GetPlayerState<ACCLPlayerState>() : nullptr;
	if (!bPendingRestore) { GetWorld()->GetTimerManager().ClearTimer(RestoreTimer); return; }
	if (!Player || !Player->GetPawn() || !Player->GetAbilitySystemComponent()->GetAvatarActor())
	{
		if (FPlatformTime::Seconds() - RestoreStarted < 20.) { return; }
	}
	const bool bLoaded = Player && ApplyRecord(Pending);
	bPendingRestore = 0;
	GetWorld()->GetTimerManager().ClearTimer(RestoreTimer);
	Status = bLoaded ? TEXT("Checkpoint restored. Surviving enemies reset; host progress retained.") : TEXT("Checkpoint application failed. Return to start and retry.");
	UE_LOG(LogTemp, Display, TEXT("CCL_SESSION restore=%d"), bLoaded);
	if (!bLoaded) { ReturnToMenu(); }
}
bool UCCLGameInstance::ApplyRecord(const FCCLSessionRecord& Record)
{
	if (!FCCLSessionCodec::Validate(Record)) { return false; }
	auto* PC = GetFirstLocalPlayerController();
	auto* Player = PC ? PC->GetPlayerState<ACCLPlayerState>() : nullptr;
	if (!Player || !PC->HasAuthority()) { return false; }
	TArray<FCCLInventoryEntry> Entries;
	for (const auto& Value : Record.Items)
	{
		auto& Entry = Entries.AddDefaulted_GetRef();
		Entry.Id = Value.Id;
		Entry.Definition = FCCLSessionCodec::Item(Value.Definition);
		Entry.Quantity = Value.Quantity;
		Entry.Slot = Value.Slot;
	}
	TArray<UCCLSkillDefinition*> Skills;
	for (FName Id : Record.Skills) { Skills.Add(FCCLSessionCodec::Skill(Id)); }
	if (!Player->GetInventory()->Restore(Entries) || !Player->GetLoadout()->Restore(Record.EquipmentSlots, Skills, Record.Points) ||
	    !Player->GetExpedition()->Restore(Record.Coins, static_cast<ECCLQuestStatus>(Record.Quest)))
	{
		return false;
	}

	for (TActorIterator<ACCLWorldPickup> It(GetWorld()); It; ++It) { if (It->Definition && !Record.RemainingSupplies.Contains(It->Definition->GetFName())) { It->Destroy(); } }
	for (TActorIterator<ACCLCampaignDirector> It(GetWorld()); It; ++It) { return It->RestoreCheckpoint(Record.DefeatedGuards, Record.Victory != 0); }
	return false;
}
void UCCLGameInstance::ReturnToMenu()
{
	bPendingRestore = 0;
	HideMenu();
	UGameplayStatics::OpenLevel(this, TEXT("/Game/Maps/FrontEnd"), true);
}
void UCCLGameInstance::Quit()
{
	if (auto* PC = GetFirstLocalPlayerController()) { PC->ConsoleCommand(TEXT("quit")); }
}
void UCCLGameInstance::CycleQuality()
{
	auto* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings) { return; }
	const int32 Quality = (FMath::Max(0, Settings->GetOverallScalabilityLevel()) + 1) % 4;
	Settings->SetOverallScalabilityLevel(Quality);
	Settings->ApplySettings(false);
	Status = FString::Printf(TEXT("Graphics quality saved: %s"), Quality == 0 ? TEXT("Low") : Quality == 1 ? TEXT("Medium") : Quality == 2 ? TEXT("High") : TEXT("Epic"));
}
void UCCLGameInstance::ToggleWindowMode()
{
	auto* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings) { return; }
	const bool bWindowed = Settings->GetFullscreenMode() != EWindowMode::Windowed;
	Settings->SetFullscreenMode(bWindowed ? EWindowMode::Windowed : EWindowMode::WindowedFullscreen);
	Settings->SetScreenResolution(bWindowed ? FIntPoint(1280, 720) : Settings->GetDesktopResolution());
	Settings->ApplySettings(false);
	Status = bWindowed ? TEXT("Windowed 1280x720 saved.") : TEXT("Borderless desktop mode saved.");
}
void UCCLGameInstance::NetworkFailed(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Error)
{
	if (World && World->GetGameInstance() != this) { return; }
	Status = TEXT("Connection failed: ") + Error.Left(300);
	bPendingRestore = 0;
	ShowMenu();
	UE_LOG(LogTemp, Display, TEXT("CCL_SESSION connection failed"));
}
void UCCLGameInstance::TravelFailed(UWorld* World, ETravelFailure::Type Type, const FString& Error)
{
	if (World && World->GetGameInstance() != this) { return; }
	Status = TEXT("Travel failed: ") + Error.Left(300);
	bPendingRestore = 0;
	ShowMenu();
}
