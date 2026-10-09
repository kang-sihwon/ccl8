#include "CCLGameInstance.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Agents/CCLAccountComponent.h"
#include "Misc/Base64.h"
#include "UI/CCLGameUI.h"
#include "UI/CCLSessionMenuScreen.h"
#include "UI/Core/CCLUISubsystem.h"
#include "Engine/LocalPlayer.h"
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
	ShowMenuForPlayer(GetFirstLocalPlayerController());
}

void UCCLGameInstance::ShowMenuForPlayer(APlayerController* PC)
{
	if (IsMenuVisibleForPlayer(PC))
	{
		return;
	}

	auto* UI = CCLGameUI::Get(PC);
	if (!UI)
	{
		return;
	}

	for (auto It = MenuHandles.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}

	if (auto* CCL = Cast<ACCLPlayerController>(PC))
	{
		CCL->CloseInventory();
		CCL->CloseDialogue();
	}

	PC->FlushPressedKeys();
	auto* Context = NewObject<UCCLSessionMenuContext>(this);
	Context->Session = this;
	Context->Controller = PC;
	Context->bInCampaign = GetWorld()->GetGameState<ACCLCampaignState>() != nullptr;
	MenuHandles.Add(PC->GetLocalPlayer(), UI->OpenView(CCLUITags::View_SessionMenu, Context, PC));
}

void UCCLGameInstance::HideMenu()
{
	const auto Closing = MoveTemp(MenuHandles);
	MenuHandles.Reset();
	for (const auto& Pair : Closing)
	{
		if (auto* UI = Pair.Key.IsValid() ? Pair.Key->GetSubsystem<UCCLUISubsystem>() : nullptr)
		{
			UI->CloseView(Pair.Value);
		}
	}
}

void UCCLGameInstance::HideMenuForPlayer(APlayerController* PC)
{
	const auto* Local = PC ? PC->GetLocalPlayer() : nullptr;
	if (auto* UI = Local ? Local->GetSubsystem<UCCLUISubsystem>() : nullptr)
	{
		FCCLUIViewHandle Handle;
		if (MenuHandles.RemoveAndCopyValue(PC->GetLocalPlayer(), Handle))
		{
			UI->CloseView(Handle);
		}
	}
}

void UCCLGameInstance::ToggleMenu()
{
	ToggleMenuForPlayer(GetFirstLocalPlayerController());
}

void UCCLGameInstance::ToggleMenuForPlayer(APlayerController* PC)
{
	if (IsMenuVisibleForPlayer(PC))
	{
		HideMenuForPlayer(PC);
	}
	else
	{
		ShowMenuForPlayer(PC);
	}
}

bool UCCLGameInstance::IsMenuVisible() const
{
	return IsMenuVisibleForPlayer(GetFirstLocalPlayerController());
}

bool UCCLGameInstance::IsMenuVisibleForPlayer(const APlayerController* PC) const
{
	const auto* Local = PC ? PC->GetLocalPlayer() : nullptr;
	const auto* UI = Local ? Local->GetSubsystem<UCCLUISubsystem>() : nullptr;
	const auto* Handle = Local ? MenuHandles.Find(PC->GetLocalPlayer()) : nullptr;
	return UI && Handle && UI->IsViewOpen(*Handle);
}
void UCCLGameInstance::StartNew(bool bHost)
{
	GetSubsystem<UCCLAgentSessionStore>()->ResetSession();
	bPendingRestore = 0;
	Status = bHost ? TEXT("원정 방을 열었다.") : TEXT("혼자 하는 원정을 시작했다.");
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
	if (!ValidateAddress(Address)) { Status = TEXT("주소가 올바르지 않다. 호스트 이름 또는 IPv4 주소:포트 (1~65535)를 입력해 줘."); return false; }
	auto* PC = GetFirstLocalPlayerController();
	if (!PC) { Status = TEXT("플레이어가 아직 준비되지 않았다."); return false; }
	bPendingRestore = 0;
	Status = TEXT("접속 중: ") + Address + TEXT(" ...");
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
	{ Status = TEXT("저장할 수 없다. 원정 중 살아 있는 호스트가 행동을 멈춘 상태에서 저장해 줘."); return false; }
	FCCLSessionRecord Record;
	for (const auto& Entry : Player->GetInventory()->GetEntries())
	{
		FCCLSavedItem Value;
		Value.Id = Entry.Id;
		Value.Definition = Entry.Definition ? Entry.Definition->GetFName() : NAME_None;
		Value.Quantity = Entry.Quantity;
		Value.Slot = Entry.Slot;
		Value.LoadedAmmo = Entry.LoadedAmmo;
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
	if (auto* Agents = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>(); Agents && Agents->IsRunning())
	{
		TArray<uint8> Simulation;
		if (!Agents->Save(Simulation))
		{
			Status = TEXT("주민 상태를 수집하지 못해 저장할 수 없다.");
			return false;
		}

		Record.AgentSimulation = FBase64::Encode(Simulation);
		if (const auto* Account = Player->FindComponentByClass<UCCLAccountComponent>())
		{
			Record.AccountId = Account->GetAccountId();
			Record.Coins = 0;
		}
	}

	TArray<uint8> Bytes;
	const bool bSaved = FCCLSessionCodec::Encode(Record, Bytes) && UGameplayStatics::SaveDataToSlot(Bytes, SaveSlot(), 0);
	Status = bSaved ? TEXT("호스트의 진행 상황을 저장했다. 불러오면 마을에서 체력이 가득 찬 상태로 시작한다.") : TEXT("저장하지 못했다. 저장 공간과 콘텐츠 정보를 확인해 줘.");
	UE_LOG(LogTemp, Display, TEXT("CCL_SESSION save=%d"), bSaved);
	return bSaved;
}
bool UCCLGameInstance::LoadSession(bool bHost)
{
	TArray<uint8> Bytes;
	FCCLSessionRecord Record;
	if (!UGameplayStatics::LoadDataFromSlot(Bytes, SaveSlot(), 0) || !FCCLSessionCodec::Decode(Bytes, Record))
	{ Status = TEXT("불러올 수 없다. 저장 파일이 없거나 손상되었으며, 버전 또는 콘텐츠가 맞지 않을 수도 있다."); return false; }
	Pending = Record;
	GetSubsystem<UCCLAgentSessionStore>()->ResetSession();
	bPendingRestore = 1;
	Status = TEXT("저장한 진행 상황을 불러오는 중...");
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
	Status = bLoaded ? TEXT("진행 상황을 불러왔다. 살아남은 적은 초기화되고 호스트의 진행은 유지된다.") : TEXT("진행 상황을 적용하지 못했다. 시작 화면에서 다시 시도해 줘.");
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
		Entry.LoadedAmmo = Value.LoadedAmmo;
	}
	TArray<UCCLSkillDefinition*> Skills;
	for (FName Id : Record.Skills) { Skills.Add(FCCLSessionCodec::Skill(Id)); }
	if (!Player->GetInventory()->Restore(Entries) || !Player->GetLoadout()->Restore(Record.EquipmentSlots, Skills, Record.Points) ||
	    !Player->GetExpedition()->Restore(Record.Coins, static_cast<ECCLQuestStatus>(Record.Quest), !Record.AccountId.IsValid()))
	{
		return false;
	}

	if (!Record.AgentSimulation.IsEmpty())
	{
		TArray<uint8> Simulation;
		FString Error;
		auto* Agents = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
		if (!Agents || !FBase64::Decode(Record.AgentSimulation, Simulation) || !Agents->Restore(Simulation, Error))
		{
			return false;
		}
	}

	if (Record.AccountId.IsValid())
	{
		auto* Account = Player->FindComponentByClass<UCCLAccountComponent>();
		if (!Account || !Account->BindAccount(Record.AccountId))
		{
			return false;
		}
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
	Status = FString::Printf(TEXT("그래픽 품질 저장됨: %s"), Quality == 0 ? TEXT("낮음") : Quality == 1 ? TEXT("보통") : Quality == 2 ? TEXT("높음") : TEXT("최고"));
}
void UCCLGameInstance::ToggleWindowMode()
{
	auto* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings) { return; }
	const bool bWindowed = Settings->GetFullscreenMode() != EWindowMode::Windowed;
	Settings->SetFullscreenMode(bWindowed ? EWindowMode::Windowed : EWindowMode::WindowedFullscreen);
	Settings->SetScreenResolution(bWindowed ? FIntPoint(1280, 720) : Settings->GetDesktopResolution());
	Settings->ApplySettings(false);
	Status = bWindowed ? TEXT("1280×720 창 모드로 저장했다.") : TEXT("테두리 없는 창 모드로 저장했다.");
}
void UCCLGameInstance::NetworkFailed(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Error)
{
	if (World && World->GetGameInstance() != this) { return; }
	Status = TEXT("접속하지 못했다. 호스트 주소와 네트워크 연결을 확인해 줘.");
	UE_LOG(LogTemp, Warning, TEXT("CCL_SESSION network failure: %s"), *Error);
	bPendingRestore = 0;
	ShowMenu();
	UE_LOG(LogTemp, Display, TEXT("CCL_SESSION connection failed"));
}
void UCCLGameInstance::TravelFailed(UWorld* World, ETravelFailure::Type Type, const FString& Error)
{
	if (World && World->GetGameInstance() != this) { return; }
	Status = TEXT("맵으로 이동하지 못했다. 시작 화면에서 다시 시도해 줘.");
	UE_LOG(LogTemp, Warning, TEXT("CCL_SESSION travel failure: %s"), *Error);
	bPendingRestore = 0;
	ShowMenu();
}
